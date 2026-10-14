#pragma once
#include <jni.h>
#include <cstring>
#include <cstdint>
#include "style.hpp"
#include "preset_bridge.hpp"
#include "monet_source.hpp"
namespace gyglass {
struct PatchEvidence {
 bool signature_ok=false;
 bool map_ok=false;
 bool monet_ok=false;
 unsigned char key_status[4]{};  // 0=not found, 1=already correct, 2=written
};
// Experimental, strictly limited to WeType's selfdraw keyboard View. No Window blur.
inline bool clear_error(JNIEnv* e) {if(!e->ExceptionCheck())return true;e->ExceptionClear();return false;}
inline jobject locate_view(JNIEnv* e,jobject node,int& visited,int depth) {
 if(!node||visited++>=512||depth>20)return nullptr;
 jclass cls=e->GetObjectClass(node);
 jclass classcls=e->FindClass("java/lang/Class");
 jmethodID getname=classcls?e->GetMethodID(classcls,"getName","()Ljava/lang/String;"):nullptr;
 jstring name=getname?static_cast<jstring>(e->CallObjectMethod(cls,getname)):nullptr;
 bool found=false;
 if(name&&clear_error(e)) {
  const char* n=e->GetStringUTFChars(name,nullptr);
  found=n && std::strcmp(n,"com.tencent.wetype.plugin.hld.keyboard.selfdraw.o")==0;
  if(n)e->ReleaseStringUTFChars(name,n);
 }
 clear_error(e);
 if(name)e->DeleteLocalRef(name);
 if(cls)e->DeleteLocalRef(cls);
 if(classcls)e->DeleteLocalRef(classcls);
 if(found)return e->NewLocalRef(node);
 jclass grp=e->FindClass("android/view/ViewGroup");
 if(!grp||!clear_error(e))return nullptr;
 if(!e->IsInstanceOf(node,grp)){e->DeleteLocalRef(grp);return nullptr;}
 jmethodID count=e->GetMethodID(grp,"getChildCount","()I");
 jmethodID at=e->GetMethodID(grp,"getChildAt","(I)Landroid/view/View;");
 int children=count&&at?e->CallIntMethod(node,count):0;
 if(!clear_error(e)){e->DeleteLocalRef(grp);return nullptr;}
 for(int i=0;i<children&&i<256&&visited<512;++i){
  jobject child=e->CallObjectMethod(node,at,i);
  if(!clear_error(e))break;
  if(!child)continue;
  jobject hit=locate_view(e,child,visited,depth+1);
  e->DeleteLocalRef(child);
  if(hit){e->DeleteLocalRef(grp);return hit;}
 }
 e->DeleteLocalRef(grp);
 return nullptr;
}
inline jobject discover_keyboard(JNIEnv* e) {
 jclass thread=e->FindClass("android/app/ActivityThread");
 jmethodID current=thread?e->GetStaticMethodID(thread,"currentActivityThread","()Landroid/app/ActivityThread;"):nullptr;
 jfieldID services=thread?e->GetFieldID(thread,"mServices","Landroid/util/ArrayMap;"):nullptr;
 if(!current||!services||!clear_error(e)) {if(thread)e->DeleteLocalRef(thread);return nullptr;}
 jobject t=e->CallStaticObjectMethod(thread,current);
 jobject map=t?e->GetObjectField(t,services):nullptr;
 if(t)e->DeleteLocalRef(t);
 e->DeleteLocalRef(thread);
 if(!map||!clear_error(e)) {if(map)e->DeleteLocalRef(map);return nullptr;}
 jclass mc=e->FindClass("android/util/ArrayMap");
 jmethodID size=mc?e->GetMethodID(mc,"size","()I"):nullptr;
 jmethodID value=mc?e->GetMethodID(mc,"valueAt","(I)Ljava/lang/Object;"):nullptr;
 jclass ime=e->FindClass("android/inputmethodservice/InputMethodService");
 if(!mc||!size||!value||!ime||!clear_error(e)){
  clear_error(e);e->DeleteLocalRef(map);
  if(mc)e->DeleteLocalRef(mc);if(ime)e->DeleteLocalRef(ime);return nullptr;
 }
 int num=e->CallIntMethod(map,size);
 if(!clear_error(e))num=0;
 jobject result=nullptr;
 for(int i=0;i<num&&i<32&&!result;++i) {
  jobject svc=e->CallObjectMethod(map,value,i);
  if(!svc||!clear_error(e)){clear_error(e);if(svc)e->DeleteLocalRef(svc);break;}
  if(e->IsInstanceOf(svc,ime)) {
   jmethodID getwindow=e->GetMethodID(ime,"getWindow","()Landroid/app/Dialog;");
   jobject dlg=getwindow?e->CallObjectMethod(svc,getwindow):nullptr;
   if(dlg&&clear_error(e)) {
    jclass dialog=e->FindClass("android/app/Dialog");
    jmethodID dw=dialog?e->GetMethodID(dialog,"getWindow","()Landroid/view/Window;"):nullptr;
    jobject window=dw?e->CallObjectMethod(dlg,dw):nullptr;
    if(window&&clear_error(e)){
     jclass wc=e->FindClass("android/view/Window");
     jmethodID decor=wc?e->GetMethodID(wc,"peekDecorView","()Landroid/view/View;"):nullptr;
     jobject root=decor?e->CallObjectMethod(window,decor):nullptr;
     if(root&&clear_error(e)) {int visited=0;result=locate_view(e,root,visited,0);}
     clear_error(e);if(root)e->DeleteLocalRef(root);
     if(wc)e->DeleteLocalRef(wc);
     e->DeleteLocalRef(window);
    }
    clear_error(e);if(dialog)e->DeleteLocalRef(dialog);
    e->DeleteLocalRef(dlg);
   }
   clear_error(e);
  }
  e->DeleteLocalRef(svc);
 }
 e->DeleteLocalRef(map);e->DeleteLocalRef(mc);e->DeleteLocalRef(ime);
 return result;
}
inline int apply_keyboard(JNIEnv* e,jobject keyboard,const PresetConfig& preset,PatchEvidence* evidence=nullptr) {
 if(evidence)*evidence=PatchEvidence{};
 if(!keyboard)return 0;
 jclass vc=e->GetObjectClass(keyboard);
 jclass cc=e->FindClass("java/lang/Class");
 jmethodID loader=cc?e->GetMethodID(cc,"getClassLoader","()Ljava/lang/ClassLoader;"):nullptr;
 jobject ld=loader?e->CallObjectMethod(vc,loader):nullptr;
 jclass lc=e->FindClass("java/lang/ClassLoader");
 jmethodID load=lc?e->GetMethodID(lc,"loadClass","(Ljava/lang/String;)Ljava/lang/Class;"):nullptr;
 if(!ld||!load||!clear_error(e)) {clear_error(e);if(vc)e->DeleteLocalRef(vc);if(cc)e->DeleteLocalRef(cc);if(lc)e->DeleteLocalRef(lc);if(ld)e->DeleteLocalRef(ld);return 1;}
 jstring n=e->NewStringUTF("com.tencent.wetype.plugin.hld.o");
 jclass attrs=static_cast<jclass>(e->CallObjectMethod(ld,load,n));
 e->DeleteLocalRef(n);e->DeleteLocalRef(ld);e->DeleteLocalRef(lc);e->DeleteLocalRef(cc);
 if(!attrs||!clear_error(e)){clear_error(e);if(attrs)e->DeleteLocalRef(attrs);e->DeleteLocalRef(vc);return 1;}
 const char* names[]={"ime_color_btn_white_bg","ime_color_btn_grey_bg","ime_color_btn_white_border","ime_color_btn_grey_border"};
 jint ids[4]{};bool ok=true;
 for(int i=0;i<4;++i){
  jfieldID f=e->GetStaticFieldID(attrs,names[i],"I");
  if(!f||!clear_error(e)){ok=false;break;}
  ids[i]=e->GetStaticIntField(attrs,f);
  if(!clear_error(e)||ids[i]<=0){ok=false;break;}
 }
 e->DeleteLocalRef(attrs);
 if(!ok){e->DeleteLocalRef(vc);return 1;}
 if(evidence)evidence->signature_ok=true;
 jmethodID getmap=e->GetMethodID(vc,"getColorMap","()Ljava/util/HashMap;");
 jobject m=getmap?e->CallObjectMethod(keyboard,getmap):nullptr;
 if(!m||!clear_error(e)){clear_error(e);if(m)e->DeleteLocalRef(m);e->DeleteLocalRef(vc);return 2;}
 if(evidence)evidence->map_ok=true;
 const MonetSource colors=query_monet(e);
 if(!colors.available){e->DeleteLocalRef(m);e->DeleteLocalRef(vc);return 2;}
 if(evidence)evidence->monet_ok=true;
 const Palette p=resolve(preset.mode==Mode::Independent,colors.dark,
                         colors.neutral,colors.accent,preset.key_opacity_percent);
 const uint32_t colors_out[]={p.key,p.function_key,p.border,p.border};
 jclass mapc=e->FindClass("java/util/Map");
 jclass integer=e->FindClass("java/lang/Integer");
 jmethodID contains=mapc?e->GetMethodID(mapc,"containsKey","(Ljava/lang/Object;)Z"):nullptr;
 jmethodID get=mapc?e->GetMethodID(mapc,"get","(Ljava/lang/Object;)Ljava/lang/Object;"):nullptr;
 jmethodID put=mapc?e->GetMethodID(mapc,"put","(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;"):nullptr;
 jmethodID box=integer?e->GetStaticMethodID(integer,"valueOf","(I)Ljava/lang/Integer;"):nullptr;
 jmethodID unbox=integer?e->GetMethodID(integer,"intValue","()I"):nullptr;
 if(!contains||!get||!put||!box||!unbox||!clear_error(e)){
  clear_error(e);if(mapc)e->DeleteLocalRef(mapc);if(integer)e->DeleteLocalRef(integer);
  e->DeleteLocalRef(m);e->DeleteLocalRef(vc);return 2;
 }
 int updates=0,matched=0;
 for(int i=0;i<4;++i) {
  jobject k=e->CallStaticObjectMethod(integer,box,ids[i]);
  if(!k||!clear_error(e)){clear_error(e);if(k)e->DeleteLocalRef(k);continue;}
  if(e->CallBooleanMethod(m,contains,k)==JNI_TRUE&&clear_error(e)) {
   jobject old=e->CallObjectMethod(m,get,k);
   if(old&&clear_error(e)&&e->IsInstanceOf(old,integer)) {
    ++matched;
    if(evidence)evidence->key_status[i]=1;
    jint before=e->CallIntMethod(old,unbox);
    if(clear_error(e)&&static_cast<uint32_t>(before)!=colors_out[i]) {
     jobject val=e->CallStaticObjectMethod(integer,box,static_cast<jint>(colors_out[i]));
     if(val&&clear_error(e)){jobject prev=e->CallObjectMethod(m,put,k,val);
      if(clear_error(e)){++updates;if(evidence)evidence->key_status[i]=2;}
      if(prev)e->DeleteLocalRef(prev);
     }
     clear_error(e);if(val)e->DeleteLocalRef(val);
    }
   }
   clear_error(e);if(old)e->DeleteLocalRef(old);
  }
  clear_error(e);e->DeleteLocalRef(k);
 }
 if(updates){
  jmethodID invalidate=e->GetMethodID(vc,"postInvalidate","()V");
  if(invalidate&&clear_error(e))e->CallVoidMethod(keyboard,invalidate);
  clear_error(e);
 }
 e->DeleteLocalRef(mapc);e->DeleteLocalRef(integer);e->DeleteLocalRef(m);e->DeleteLocalRef(vc);
 return updates?4:(matched?5:3); // 5 means already has desired colors
}
}
