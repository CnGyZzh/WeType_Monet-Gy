#include "zygisk.hpp"
#include "style.hpp"
#include <android/log.h>
#include <cstring>

namespace {
constexpr char kTarget[]="com.tencent.wetype";
class GyGlassModule final:public zygisk::ModuleBase {
public:
 void onLoad(zygisk::Api* api,JNIEnv* env) override {api_=api;env_=env;}
 void preAppSpecialize(zygisk::AppSpecializeArgs* args) override {
   if(args==nullptr || args->nice_name==nullptr) return;
   const char* p=env_->GetStringUTFChars(args->nice_name,nullptr);
   if(p!=nullptr) {
     active_=std::strcmp(p,kTarget)==0 ||
       std::strncmp(p,"com.tencent.wetype:",sizeof(kTarget))==0;
     env_->ReleaseStringUTFChars(args->nice_name,p);
   }
   if(!active_) api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
 }
 void postAppSpecialize(const zygisk::AppSpecializeArgs*) override {
   if(!active_) return;
   // ART method hooks pending. Never blur the entire IME window.
   __android_log_print(ANDROID_LOG_INFO,"GyGlass",
                       "native stage loaded; renderer not enabled");
 }
private:
 zygisk::Api* api_=nullptr;
 JNIEnv* env_=nullptr;
 bool active_=false;
};
}
REGISTER_ZYGISK_MODULE(GyGlassModule)
