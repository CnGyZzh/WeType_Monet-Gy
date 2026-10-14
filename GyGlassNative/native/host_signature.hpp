#pragma once
#include <array>
#include <string_view>
namespace gyglass {
// Candidate layouts observed in prior WeType HLD versions.
// They are NOT trusted until the host ClassLoader resolves all required fields.
struct HostSignature {
 std::string_view class_name;
 std::string_view normal_key;
 std::string_view special_key;
 std::string_view border;
};
inline constexpr std::array<HostSignature,4> known_signatures{{
 {"com.tencent.wetype.plugin.hld.s","ime_color_btn_white_bg","ime_color_btn_grey_bg","ime_color_btn_white_border"},
 {"com.tencent.wetype.plugin.hld.o","ime_color_btn_white_bg","ime_color_btn_grey_bg","ime_color_btn_white_border"},
 {"com.tencent.wetype.plugin.hld.s","ime_key_white_color","ime_key_grey_color","ime_key_white_border_color"},
 {"com.tencent.wetype.plugin.hld.o","ime_key_white_color","ime_key_grey_color","ime_key_white_border_color"}
}};
template<class Probe>
const HostSignature* match_host_signature(Probe&& field_present) {
 for(const auto& s:known_signatures){
  if(field_present(s.class_name,s.normal_key)&&
     field_present(s.class_name,s.special_key)&&
     field_present(s.class_name,s.border)) return &s;
 }
 return nullptr; // Fail closed on unknown/renamed host resources.
}
}
