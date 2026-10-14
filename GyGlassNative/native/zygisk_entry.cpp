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

namespace {
constexpr char kTarget[] = "com.tencent.wetype";
constexpr char kModuleDirectory[] = "/data/adb/modules/gyglass";
constexpr unsigned char kQueryPreset = 'P';

// Root Companion is a request handler, not a resident polling loop.
// It exposes only the preset mode and never reads chats/clipboard/input text.
void companion_handler(int socket) {
 unsigned char query=0;
 const ssize_t received=recv(socket,&query,sizeof(query),0);
 if(received==1 && query==kQueryPreset) {
  const int root=open(kModuleDirectory,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
  const gyglass::Mode mode=gyglass::read_preset_at(root);
  if(root>=0) close(root);
  const gyglass::PresetReply reply=gyglass::make_reply(mode);
  (void)send(socket,&reply,sizeof(reply),MSG_NOSIGNAL);
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
 }
 void postAppSpecialize(const zygisk::AppSpecializeArgs*) override {
  if(!active_) return;
  // Until an actual IME-scoped ART renderer is installed, report only
  // native injection and preset discovery, not hook/render success.
  __android_log_print(ANDROID_LOG_INFO,"GyGlass",
      "native_entry=loaded preset=%s renderer=not_implemented",
      mode_==gyglass::Mode::Independent?"independent":"seamless");
 }
 private:
 zygisk::Api* api_=nullptr;
 JNIEnv* env_=nullptr;
 bool active_=false;
 gyglass::Mode mode_=gyglass::Mode::Seamless;
};
}
REGISTER_ZYGISK_MODULE(GyGlassModule)
REGISTER_ZYGISK_COMPANION(companion_handler)
