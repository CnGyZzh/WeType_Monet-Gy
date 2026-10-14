#pragma once
#include <jni.h>
#include <cstdint>

namespace gyglass {
struct MonetSource {
 uint32_t neutral=0;
 uint32_t accent=0;
 bool available=false;
 bool dark=false;
};
// Query current system resource overlays, never poll, never access wallpaper images.
inline MonetSource query_monet(JNIEnv* env) noexcept {
 MonetSource result;
 if(!env) return result;
 auto fail=[&](){if(env->ExceptionCheck()) env->ExceptionClear();return result;};
 jclass resourceClass=env->FindClass("android/content/res/Resources");
 if(!resourceClass) return fail();
 jmethodID getSystem=env->GetStaticMethodID(resourceClass,"getSystem","()Landroid/content/res/Resources;");
 jmethodID getIdentifier=env->GetMethodID(resourceClass,"getIdentifier","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I");
 jmethodID getColor=env->GetMethodID(resourceClass,"getColor","(I)I");
 jmethodID getConfig=env->GetMethodID(resourceClass,"getConfiguration","()Landroid/content/res/Configuration;");
 if(!getSystem || !getIdentifier || !getColor || !getConfig) {
  env->DeleteLocalRef(resourceClass); return fail();
 }
 jobject resources=env->CallStaticObjectMethod(resourceClass,getSystem);
 if(env->ExceptionCheck() || !resources) {
  env->DeleteLocalRef(resourceClass);return fail();
 }
 jobject config=env->CallObjectMethod(resources,getConfig);
 if(config && !env->ExceptionCheck()){
  jclass configClass=env->GetObjectClass(config);
  jfieldID uiMode=env->GetFieldID(configClass,"uiMode","I");
  if(uiMode && !env->ExceptionCheck()) result.dark=(env->GetIntField(config,uiMode)&0x30)==0x20;
  env->DeleteLocalRef(configClass);
  env->DeleteLocalRef(config);
 }
 if(env->ExceptionCheck()) env->ExceptionClear();
 auto lookup=[&](const char* name)->uint32_t {
  jstring jname=env->NewStringUTF(name);
  jstring jtype=env->NewStringUTF("color");
  jstring jpkg=env->NewStringUTF("android");
  if(!jname || !jtype || !jpkg || env->ExceptionCheck()){
   if(jname)env->DeleteLocalRef(jname);
   if(jtype)env->DeleteLocalRef(jtype);
   if(jpkg)env->DeleteLocalRef(jpkg);
   env->ExceptionClear();
   return 0;
  }
  jint rid=env->CallIntMethod(resources,getIdentifier,jname,jtype,jpkg);
  env->DeleteLocalRef(jname);
  env->DeleteLocalRef(jtype);
  env->DeleteLocalRef(jpkg);
  if(env->ExceptionCheck()){env->ExceptionClear();return 0;}
  if(rid<=0)return 0;
  const jint argb=env->CallIntMethod(resources,getColor,rid);
  if(env->ExceptionCheck()){env->ExceptionClear();return 0;}
  return static_cast<uint32_t>(argb)&0x00FFFFFFu;
 };
 result.neutral=lookup(result.dark?"system_neutral1_800":"system_neutral1_100");
 result.accent=lookup(result.dark?"system_accent1_300":"system_accent1_400");
 result.available=result.neutral!=0 && result.accent!=0;
 env->DeleteLocalRef(resources);
 env->DeleteLocalRef(resourceClass);
 return result;
}
}
