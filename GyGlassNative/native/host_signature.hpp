#pragma once
#include <array>
#include <string_view>
namespace gyglass {
struct RequiredHostMethod {
 std::string_view owner;
 std::string_view name;
 std::string_view descriptor;
};
// Verified from the user-provided WeType APK (manifest versionCode=57206),
// using DEX source_file_idx + method_id + proto_id. These are only candidates
// until the app's ClassLoader has resolved all signatures at runtime.
inline constexpr std::array<RequiredHostMethod,4> host_draw_contract{{
 {"com.tencent.wetype.plugin.hld.keyboard.selfdraw.o","onDraw","(Landroid/graphics/Canvas;)V"},
 {"com.tencent.wetype.plugin.hld.keyboard.selfdraw.o","getButtonDrawer","()Lcom/tencent/wetype/plugin/hld/keyboard/selfdraw/e;"},
 {"com.tencent.wetype.plugin.hld.keyboard.selfdraw.o","getColorMap","()Ljava/util/HashMap;"},
 {"com.tencent.wetype.plugin.hld.keyboard.selfdraw.e","d","(Landroid/graphics/Canvas;)V"}
}};
// Runtime callers must match class + method + descriptor for EVERY member.
// Never use obfuscated class name alone; never hook an unmatched version.
template<class Probe>
bool match_host_draw_contract(Probe&& has_method){
 for(const auto& m:host_draw_contract){
  if(!has_method(m.owner,m.name,m.descriptor)) return false;
 }
 return true;
}
}
