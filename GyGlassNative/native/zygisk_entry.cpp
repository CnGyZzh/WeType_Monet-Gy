#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstdio>
#include <ctime>
#include <unistd.h>
#include <android/log.h>
#include <cstdint>
#include <cstring>
#include "zygisk.hpp"
#include "preset_bridge.hpp"
#include "keyboard_patch.hpp"

namespace {
constexpr char kTarget[]="com.tencent.wetype";
constexpr char kFolder[]="/data/adb/modules/gyglass";
constexpr unsigned char kQuery='P';
constexpr unsigned char kEvent='E';
using PollOnce=void(*)(JNIEnv*,jobject,jlong,jint);
PollOnce original=nullptr;
class GyGlassModule;
GyGlassModule* active_instance=nullptr;
void replacement(JNIEnv*,jobject,jlong,jint);

// On-demand messages are delivered by Zygisk's root companion. No resident
// logger, no chat contents, and no background polling.
void companion_handler(int fd) {
 unsigned char q=0;
 for(;;) {
  if(recv(fd,&q,1,0)!=1)break;
  if(q=='I') { // One-time pre-specialization injection proof.
   struct Proof { int32_t pid; uint8_t registered; } proof{};
   if(recv(fd,&proof,sizeof(proof),MSG_WAITALL)!=static_cast<ssize_t>(sizeof(proof)))break;
   if(proof.pid<=0 || proof.registered>1)break;
   int dir=open(kFolder,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
   if(dir>=0) {
    int out=openat(dir,"injection_state.txt",O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC|O_NOFOLLOW,0600);
    if(out>=0) {
     char data[120]{};
     const int n=std::snprintf(data,sizeof(data),
       "NATIVE_PRE=ok\\nNATIVE_PID=%ld\\nNATIVE_QUEUE_REGISTERED=%u\\n",
       static_cast<long>(proof.pid),static_cast<unsigned>(proof.registered));
     if(n>0 && static_cast<size_t>(n)<sizeof(data)) (void)write(out,data,static_cast<size_t>(n));
     close(out);
    }
    close(dir);
   }
  } else if(q==kQuery) {
   int dir=open(kFolder,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
   auto config=gyglass::read_preset_at(dir);
   if(dir>=0) {
    int state=openat(dir,"companion_state.txt",O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC|O_NOFOLLOW,0600);
    if(state>=0) {
     static constexpr char marker[]="COMPANION_REQUEST=ok\\n";
     (void)write(state,marker,sizeof(marker)-1);
     close(state);
    }
    close(dir);
   }
   auto reply=gyglass::make_reply(config);
   if(send(fd,&reply,sizeof(reply),MSG_NOSIGNAL)!=static_cast<ssize_t>(sizeof(reply)))break;
  } else break;
 }
 close(fd);
}

class GyGlassModule final:public zygisk::ModuleBase {
 public:
 void onLoad(zygisk::Api* api,JNIEnv* env) override {
  api_=api;env_=env;active_instance=this;
 }
 void preAppSpecialize(zygisk::AppSpecializeArgs* args) override {
  if(!args||!args->nice_name) {api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);return;}
  const char* name=env_->GetStringUTFChars(args->nice_name,nullptr);
  if(name) {
   active_=std::strcmp(name,kTarget)==0 ||
           std::strncmp(name,"com.tencent.wetype:",sizeof(kTarget))==0;
   env_->ReleaseStringUTFChars(args->nice_name,name);
  }
  if(!active_){api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);return;}
  __android_log_print(ANDROID_LOG_INFO,"GyGlass",
   "GY_EVT=PRE_APP value=ok pid=%ld",static_cast<long>(getpid()));
  int dir=api_->getModuleDir();
  config_=gyglass::read_preset_at(dir);
  if(dir>=0)close(dir);
  companion_fd_=api_->connectCompanion();
  if(companion_fd_>=0&&!api_->exemptFd(companion_fd_)) {
   close(companion_fd_);companion_fd_=-1;
  }
  __android_log_print(ANDROID_LOG_INFO,"GyGlass",
   "GY_EVT=COMPANION value=%s",companion_fd_>=0?"ok":"fail");
  emit_event('I');
  JNINativeMethod m[]={{const_cast<char*>("nativePollOnce"),
                        const_cast<char*>("(JI)V"),
                        reinterpret_cast<void*>(replacement)}};
  api_->hookJniNativeMethods(env_,"android/os/MessageQueue",m,1);
  if(!env_->ExceptionCheck()&&m[0].fnPtr){
   original=reinterpret_cast<PollOnce>(m[0].fnPtr);enabled_=true;
  }else {
   if(env_->ExceptionCheck())env_->ExceptionClear();
   __android_log_print(ANDROID_LOG_WARN,"GyGlass","poll_hook=not_available");
  }
  emit_event(enabled_?'R':'F');
  // This proof is a process-specific signal independent of logcat.
  // No wait on startup: one small socket message, no service daemon.
  if(companion_fd_>=0) {
   struct Proof { int32_t pid; uint8_t registered; } proof{
     static_cast<int32_t>(getpid()), static_cast<uint8_t>(enabled_?1:0)};
   const unsigned char query='I';
   if(send(companion_fd_,&query,1,MSG_NOSIGNAL)==1)
     (void)send(companion_fd_,&proof,sizeof(proof),MSG_NOSIGNAL);
  }
 }
 void postAppSpecialize(const zygisk::AppSpecializeArgs*) override {
  if(active_) {
   __android_log_print(ANDROID_LOG_INFO,"GyGlass",
    "GY_EVT=POST_APP value=ok pid=%ld",static_cast<long>(getpid()));
   emit_event('T');
   __android_log_print(ANDROID_LOG_INFO,"GyGlass",
    "native_entry=loaded poll_hook=%d renderer=experimental",enabled_?1:0);
   __android_log_print(ANDROID_LOG_INFO,"GyGlass",
    "GY_EVT=QUEUE value=%s",enabled_?"ok":"fail");
  }
 }
 void onPoll(JNIEnv* env) noexcept {
  if(!enabled_||!env||env->ExceptionCheck()||
     static_cast<pid_t>(syscall(SYS_gettid))!=getpid())return;
  timespec ts{};if(clock_gettime(CLOCK_MONOTONIC,&ts))return;
  int64_t now=static_cast<int64_t>(ts.tv_sec)*1000000000LL+ts.tv_nsec;
  if(busy_||now<next_ns_)return;
  busy_=true;next_ns_=now+1500000000LL;
  if(!main_thread_logged_) {
   main_thread_logged_=true;
   emit_event('U');
   __android_log_print(ANDROID_LOG_INFO,"GyGlass","GY_EVT=MAIN_THREAD value=ok");
  }
  // Root IPC is bounded but never run on every native poll; cache the preset.
  if(now>=next_config_ns_){
   refresh_config();
   next_config_ns_=now+8000000000LL;
  }
  jobject kb=gyglass::discover_keyboard(env);
  if(kb){
   jclass vc=env->FindClass("android/view/View");
   jmethodID shown=vc?env->GetMethodID(vc,"isShown","()Z"):nullptr;
   bool visible=shown&&!env->ExceptionCheck()&&env->CallBooleanMethod(kb,shown)==JNI_TRUE;
   if(env->ExceptionCheck())env->ExceptionClear();
   if(vc)env->DeleteLocalRef(vc);
   if(visible){
    if(!keyboard_logged_) {
     keyboard_logged_=true;
     emit_event('K');
     __android_log_print(ANDROID_LOG_INFO,"GyGlass","GY_EVT=KEYBOARD value=ok");
    }
    gyglass::PatchEvidence evidence{};
    int status=gyglass::apply_keyboard(env,kb,config_,&evidence);
    report(status);
    report_points(status,evidence);
    if((status==4||status==5)&&!map_logged_){map_logged_=true;emit_event('M');}
    else if((status==1||status==2)&&!error_logged_){error_logged_=true;emit_event('X');}
    next_ns_=now+3000000000LL;
   }
   env->DeleteLocalRef(kb);
  }else report(-1);
  if(env->ExceptionCheck())env->ExceptionClear();
  busy_=false;
 }
 private:
 void emit_event(char event) noexcept {
  if(companion_fd_<0) return;
  unsigned char payload[6]{kEvent,static_cast<unsigned char>(event),0,0,0,0};
  const int32_t pid=static_cast<int32_t>(getpid());
  std::memcpy(payload+2,&pid,sizeof(pid));
  // No response or wait: never block the IME main thread for diagnostics.
  (void)send(companion_fd_,payload,sizeof(payload),MSG_NOSIGNAL|MSG_DONTWAIT);
 }
 void refresh_config() noexcept {
  if(companion_fd_<0)return;
  timeval lim{};lim.tv_usec=30000;
  setsockopt(companion_fd_,SOL_SOCKET,SO_RCVTIMEO,&lim,sizeof(lim));
  setsockopt(companion_fd_,SOL_SOCKET,SO_SNDTIMEO,&lim,sizeof(lim));
  const unsigned char q=kQuery;
  if(send(companion_fd_,&q,1,MSG_NOSIGNAL)!=1)return;
  gyglass::PresetReply r{};
  if(recv(companion_fd_,&r,sizeof(r),MSG_WAITALL)!=static_cast<ssize_t>(sizeof(r))||
     !gyglass::valid_reply(r))return;
  config_.mode=static_cast<gyglass::Mode>(r.mode);
  config_.key_opacity_percent=r.key_opacity_percent;
 }
 void report(int status) noexcept {
  if(status==last_status_)return;
  last_status_=status;
  __android_log_print(ANDROID_LOG_INFO,"GyGlass",
   "keyboard_map_result=%d mode=%u opacity=%u",status,
   static_cast<unsigned>(config_.mode),static_cast<unsigned>(config_.key_opacity_percent));
 }
 void report_points(int status,const gyglass::PatchEvidence& ev) noexcept {
  // Reports evidence for each actual resource. A successful map write is
  // NOT proof that Android composed translucent pixels on screen.
  const char* keys[]={"SIGNATURE","MONET","COLORMAP",
                     "NORMAL_KEY","SPECIAL_KEY","NORMAL_BORDER","SPECIAL_BORDER"};
  int values[7]{
   ev.signature_ok?1:(status==1?2:0),
   ev.monet_ok?1:(ev.map_ok?2:0),
   status==4||status==5?1:(status>=1&&status<=3?2:0),
   ev.key_status[0]?1:(ev.map_ok&&ev.monet_ok?2:0),
   ev.key_status[1]?1:(ev.map_ok&&ev.monet_ok?2:0),
   ev.key_status[2]?1:(ev.map_ok&&ev.monet_ok?2:0),
   ev.key_status[3]?1:(ev.map_ok&&ev.monet_ok?2:0)
  };
  for(int i=0;i<7;++i) {
   if(last_points_[i]==values[i])continue;
   last_points_[i]=values[i];
   __android_log_print(ANDROID_LOG_INFO,"GyGlass",
    "GY_EVT=%s value=%s",keys[i],values[i]==1?"ok":values[i]==2?"fail":"wait");
  }
 }
 zygisk::Api* api_=nullptr;
 JNIEnv* env_=nullptr;
 gyglass::PresetConfig config_{};
 int companion_fd_=-1;
 bool active_=false,enabled_=false,busy_=false;
 bool main_thread_logged_=false,keyboard_logged_=false;
 bool map_logged_=false,error_logged_=false;
 int last_points_[7]{-1,-1,-1,-1,-1,-1,-1};
 int last_status_=-99;
 int64_t next_ns_=0;
 int64_t next_config_ns_=0;
};
void replacement(JNIEnv* env,jobject queue,jlong ptr,jint timeout) {
 if(original)original(env,queue,ptr,timeout);
 if(active_instance)active_instance->onPoll(env);
}
}
REGISTER_ZYGISK_MODULE(GyGlassModule)
REGISTER_ZYGISK_COMPANION(companion_handler)
