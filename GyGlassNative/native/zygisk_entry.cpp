#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <android/log.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <sys/stat.h>
#include "zygisk.hpp"
#include "preset_bridge.hpp"
#include "keyboard_patch.hpp"

namespace {
constexpr char kTarget[]="com.tencent.wetype";
constexpr char kFolder[]="/data/adb/modules/gyglass";
constexpr unsigned char kQuery='P';
using PollOnce=void(*)(JNIEnv*,jobject,jlong,jint);
PollOnce original=nullptr;
class GyGlassModule;
GyGlassModule* active_instance=nullptr;
void replacement(JNIEnv*,jobject,jlong,jint);

// A tiny second evidence channel independent of logcat (no user text).
// Each record: Unix time, target PID, one event code. Root-only 0600 file.
void write_stage(uint32_t pid,char stage) {
 if(pid<=1 || pid>4194304 || !std::strchr("IZQqTVCAFX",stage))return;
 const char* directory="/data/adb/modules/gyglass/logs";
 (void)mkdir(directory,0700);
 int fd=open("/data/adb/modules/gyglass/logs/native.events",
             O_WRONLY|O_APPEND|O_CREAT|O_CLOEXEC|O_NOFOLLOW,0600);
 if(fd<0)return;
 struct stat st{};
 if(fstat(fd,&st)!=0 || !S_ISREG(st.st_mode) || st.st_size>=32768) {
  // Rotate bounded diagnostic data. No content is ever captured.
  close(fd);
  fd=open("/data/adb/modules/gyglass/logs/native.events",
          O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC|O_NOFOLLOW,0600);
  if(fd<0)return;
 }
 char row[80]{};
 int len=std::snprintf(row,sizeof(row),"%lld %u %c\n",
                       static_cast<long long>(time(nullptr)),pid,stage);
 if(len>0 && len<static_cast<int>(sizeof(row))) (void)write(fd,row,static_cast<size_t>(len));
 close(fd);
}
void companion_handler(int fd) {
 unsigned char q=0;
 while(recv(fd,&q,1,MSG_WAITALL)==1) {
  if(q==kQuery){
   int dir=open(kFolder,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
   auto config=gyglass::read_preset_at(dir);
   if(dir>=0)close(dir);
   auto reply=gyglass::make_reply(config);
   if(send(fd,&reply,sizeof(reply),MSG_NOSIGNAL)!=static_cast<ssize_t>(sizeof(reply)))break;
  } else if(q=='E'){
   unsigned char payload[5]{};
   if(recv(fd,payload,sizeof(payload),MSG_WAITALL)!=static_cast<ssize_t>(sizeof(payload)))break;
   uint32_t pid=0;
   std::memcpy(&pid,payload,sizeof(pid));
   write_stage(pid,static_cast<char>(payload[4]));
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
  int dir=api_->getModuleDir();
  config_=gyglass::read_preset_at(dir);
  if(dir>=0)close(dir);
  companion_fd_=api_->connectCompanion();
  if(companion_fd_>=0&&!api_->exemptFd(companion_fd_)) {
   close(companion_fd_);companion_fd_=-1;
  }
  send_stage('I');
  __android_log_print(ANDROID_LOG_INFO,"GyGlass",
   "GY_EVT=COMPANION value=%s",companion_fd_>=0?"ok":"fail");
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
 }
 void postAppSpecialize(const zygisk::AppSpecializeArgs*) override {
  if(active_) {
   send_stage('Z');
   send_stage(enabled_?'Q':'q');
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
   send_stage('T');
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
     send_stage('V');
     __android_log_print(ANDROID_LOG_INFO,"GyGlass","GY_EVT=KEYBOARD value=ok");
    }
    gyglass::PatchEvidence evidence{};
    int status=gyglass::apply_keyboard(env,kb,config_,&evidence);
    report(status);
    if(status!=last_event_status_){
     last_event_status_=status;
     if(status==4)send_stage('C');
     else if(status==5)send_stage('A');
     else if(status>0 && status<4)send_stage('F');
    }
    report_points(status,evidence);
    next_ns_=now+3000000000LL;
   }
   env->DeleteLocalRef(kb);
  }else report(-1);
  if(env->ExceptionCheck())env->ExceptionClear();
  busy_=false;
 }
 private:
 void send_stage(char stage) noexcept {
  if(companion_fd_<0)return;
  unsigned char event[6]{'E',0,0,0,0,static_cast<unsigned char>(stage)};
  const uint32_t pid=static_cast<uint32_t>(getpid());
  std::memcpy(event+1,&pid,sizeof(pid));
  (void)send(companion_fd_,event,sizeof(event),MSG_NOSIGNAL|MSG_DONTWAIT);
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
 int last_points_[7]{-1,-1,-1,-1,-1,-1,-1};
 int last_status_=-99;
 int last_event_status_=-99;
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
