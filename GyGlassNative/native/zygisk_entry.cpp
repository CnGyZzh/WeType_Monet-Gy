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

void companion_handler(int fd) {
 unsigned char q=0;
 while(recv(fd,&q,1,0)==1 && q==kQuery) {
  int dir=open(kFolder,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
  auto config=gyglass::read_preset_at(dir);
  if(dir>=0)close(dir);
  auto reply=gyglass::make_reply(config);
  if(send(fd,&reply,sizeof(reply),MSG_NOSIGNAL)!=static_cast<ssize_t>(sizeof(reply)))break;
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
  if(active_)__android_log_print(ANDROID_LOG_INFO,"GyGlass",
   "native_entry=loaded poll_hook=%d renderer=experimental",enabled_?1:0);
 }
 void onPoll(JNIEnv* env) noexcept {
  if(!enabled_||!env||env->ExceptionCheck()||
     static_cast<pid_t>(syscall(SYS_gettid))!=getpid())return;
  timespec ts{};if(clock_gettime(CLOCK_MONOTONIC,&ts))return;
  int64_t now=static_cast<int64_t>(ts.tv_sec)*1000000000LL+ts.tv_nsec;
  if(busy_||now<next_ns_)return;
  busy_=true;next_ns_=now+1500000000LL;
  refresh_config();
  jobject kb=gyglass::discover_keyboard(env);
  if(kb){
   jclass vc=env->FindClass("android/view/View");
   jmethodID shown=vc?env->GetMethodID(vc,"isShown","()Z"):nullptr;
   bool visible=shown&&!env->ExceptionCheck()&&env->CallBooleanMethod(kb,shown)==JNI_TRUE;
   if(env->ExceptionCheck())env->ExceptionClear();
   if(vc)env->DeleteLocalRef(vc);
   if(visible){
    int status=gyglass::apply_keyboard(env,kb,config_);
    report(status);next_ns_=now+3000000000LL;
   }
   env->DeleteLocalRef(kb);
  }else report(-1);
  if(env->ExceptionCheck())env->ExceptionClear();
  busy_=false;
 }
 private:
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
   "keyboard_map_status=%d mode=%u opacity=%u",status,
   static_cast<unsigned>(config_.mode),static_cast<unsigned>(config_.key_opacity_percent));
 }
 zygisk::Api* api_=nullptr;
 JNIEnv* env_=nullptr;
 gyglass::PresetConfig config_{};
 int companion_fd_=-1;
 bool active_=false,enabled_=false,busy_=false;
 int last_status_=-99;
 int64_t next_ns_=0;
};
void replacement(JNIEnv* env,jobject queue,jlong ptr,jint timeout) {
 if(original)original(env,queue,ptr,timeout);
 if(active_instance)active_instance->onPoll(env);
}
}
REGISTER_ZYGISK_MODULE(GyGlassModule)
REGISTER_ZYGISK_COMPANION(companion_handler)
