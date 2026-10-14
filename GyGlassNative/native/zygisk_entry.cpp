#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <android/log.h>
#include <cstring>
#include "zygisk.hpp"
#include "style.hpp"
#include "preset_bridge.hpp"
#include "monet_source.hpp"

namespace {
constexpr char kTarget[] = "com.tencent.wetype";
constexpr char kModuleDirectory[] = "/data/adb/modules/gyglass";
constexpr unsigned char kQueryPreset = 'P';

// Root Companion is a request handler, not a resident polling loop.
// It exposes only the preset mode and never reads chats/clipboard/input text.
void companion_handler(int socket) {
 unsigned char query=0;
 // Blocking receive: no timer, no polling, no access to keyboard text.
 while(recv(socket,&query,sizeof(query),0)==1 && query==kQueryPreset) {
  const int root=open(kModuleDirectory,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
  const gyglass::Mode mode=gyglass::read_preset_at(root);
  if(root>=0) close(root);
  const gyglass::PresetReply reply=gyglass::make_reply(mode);
  if(send(socket,&reply,sizeof(reply),MSG_NOSIGNAL)!=static_cast<ssize_t>(sizeof(reply)))
   break;
 }
 close(socket);
}

class GyGlassModule final : public zygisk::ModuleBase {
 public:
 void onLoad(zygisk::Api* api, JNIEnv* env) override {
  api_=api;env_=env;
 }
 void preAppSpecialize(zygisk::AppSpecializeArgs* args) override {
  if(!args || !args->nice_name) {
   api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
   return;
  }
  const char* name=env_->GetStringUTFChars(args->nice_name,nullptr);
  if(name) {
   active_=(std::strcmp(name,kTarget)==0 ||
            std::strncmp(name,"com.tencent.wetype:",sizeof(kTarget))==0);
   env_->ReleaseStringUTFChars(args->nice_name,name);
  }
  if(!active_) {
   api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
   return;
  }
  // Zygisk's module directory fd is only accessible before app specialization.
  const int module_fd=api_->getModuleDir();
  mode_=gyglass::read_preset_at(module_fd);
  if(module_fd>=0) close(module_fd);
  // Zygisk API calls are only valid before/within specialization.
  // Preserve the socket fd so future keyboard-show callbacks can refresh the
  // preset through Root Companion without invoking Zygisk API again.
  companion_fd_=api_->connectCompanion();
  if(companion_fd_>=0 && !api_->exemptFd(companion_fd_)) {
   close(companion_fd_);
   companion_fd_=-1;
  }
 }
 void postAppSpecialize(const zygisk::AppSpecializeArgs*) override {
  if(!active_) return;
  // Until an actual IME-scoped ART renderer is installed, report only
  // native injection and preset discovery, not hook/render success.
  (void)refresh_preset();
  const gyglass::MonetSource monet=gyglass::query_monet(env_);
  if(monet.available){
   // The palette is sampled once for diagnostic readiness.
   // A future keyboard-show hook will refresh it on demand.
   const auto style=gyglass::resolve(mode_==gyglass::Mode::Independent,
                                      monet.dark,monet.neutral,monet.accent);
   __android_log_print(ANDROID_LOG_INFO,"GyGlass",
       "native_entry=loaded preset=%s monet=available key_alpha=%u renderer=not_implemented",
       mode_==gyglass::Mode::Independent?"independent":"seamless",
       static_cast<unsigned>(style.key>>24));
  }else{
   __android_log_print(ANDROID_LOG_INFO,"GyGlass",
       "native_entry=loaded preset=%s monet=unavailable renderer=not_implemented",
       mode_==gyglass::Mode::Independent?"independent":"seamless");
  }
 }
 private:
 bool refresh_preset() noexcept {
  if(companion_fd_<0) return false;
  struct timeval limit{};
  limit.tv_sec=0;
  limit.tv_usec=200000;
  (void)setsockopt(companion_fd_,SOL_SOCKET,SO_RCVTIMEO,&limit,sizeof(limit));
  (void)setsockopt(companion_fd_,SOL_SOCKET,SO_SNDTIMEO,&limit,sizeof(limit));
  const unsigned char q=kQueryPreset;
  if(send(companion_fd_,&q,1,MSG_NOSIGNAL)!=1) return false;
  gyglass::PresetReply reply{};
  if(recv(companion_fd_,&reply,sizeof(reply),MSG_WAITALL)!=static_cast<ssize_t>(sizeof(reply)) ||
     !gyglass::valid_reply(reply)) return false;
  mode_=static_cast<gyglass::Mode>(reply.mode);
  return true;
 }
 zygisk::Api* api_=nullptr;
 JNIEnv* env_=nullptr;
 bool active_=false;
 gyglass::Mode mode_=gyglass::Mode::Seamless;
 int companion_fd_=-1;
};
}
REGISTER_ZYGISK_MODULE(GyGlassModule)
REGISTER_ZYGISK_COMPANION(companion_handler)
