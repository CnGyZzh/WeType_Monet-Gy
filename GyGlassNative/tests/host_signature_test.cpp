#include "../native/host_signature.hpp"
#include <cassert>
#include <string_view>
using namespace gyglass;
int main(){
 auto first=[](std::string_view c,std::string_view f){
  return c=="com.tencent.wetype.plugin.hld.s" &&
   (f=="ime_color_btn_white_bg"||f=="ime_color_btn_grey_bg"||f=="ime_color_btn_white_border");
 };
 auto found=match_host_signature(first);
 assert(found && found->class_name=="com.tencent.wetype.plugin.hld.s");
 auto fallback=[](std::string_view c,std::string_view f){
  return c=="com.tencent.wetype.plugin.hld.o" &&
   (f=="ime_key_white_color"||f=="ime_key_grey_color"||f=="ime_key_white_border_color");
 };
 found=match_host_signature(fallback);
 assert(found && found->class_name=="com.tencent.wetype.plugin.hld.o");
 assert(match_host_signature([](auto,auto){return false;})==nullptr);
}
