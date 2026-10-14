#include "../native/host_signature.hpp"
#include <cassert>
#include <string_view>
using namespace gyglass;
int main() {
 static_assert(host_draw_contract.size()==4);
 // A verified 57206-like runtime class/method table passes.
 auto all=[](std::string_view c,std::string_view m,std::string_view d) {
  for(const auto& x:host_draw_contract) {
   if(x.owner==c && x.name==m && x.descriptor==d) return true;
  }
  return false;
 };
 assert(match_host_draw_contract(all));
 // A future update can retain names but change the draw method's prototype.
 auto updated=[](std::string_view c,std::string_view m,std::string_view d) {
  if(m=="d" && c=="com.tencent.wetype.plugin.hld.keyboard.selfdraw.e") return false;
  for(const auto& x:host_draw_contract)
   if(x.owner==c && x.name==m && x.descriptor==d)return true;
  return false;
 };
 assert(!match_host_draw_contract(updated));
 assert(!match_host_draw_contract([](auto,auto,auto){return false;}));
}
