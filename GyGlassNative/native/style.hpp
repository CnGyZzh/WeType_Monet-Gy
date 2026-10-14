#pragma once
#include <cstdint>
namespace gyglass {
struct Palette {uint32_t panel,key,function_key,toolbar,border,accent;};
constexpr uint32_t argb(uint8_t alpha,uint32_t rgb) noexcept {
 return (static_cast<uint32_t>(alpha)<<24u)|(rgb&0x00FFFFFFu);
}
constexpr uint8_t alpha_from_percent(unsigned n) noexcept {
 if(n<1||n>100)n=16;
 return static_cast<uint8_t>((n*255u+50u)/100u);
}
constexpr Palette resolve(bool independent,bool dark,uint32_t neutral,uint32_t accent,unsigned opacity=16) noexcept {
 const unsigned a=independent?alpha_from_percent(opacity):0u;
 const unsigned fk=independent?(a+16u>255u?255u:a+16u):(dark?0x45u:0x39u);
 return {argb(dark?0x28u:0x2Du,neutral),
         argb(static_cast<uint8_t>(a),neutral),
         argb(static_cast<uint8_t>(fk),neutral),
         argb(dark?0x2Bu:0x22u,neutral),
         argb(static_cast<uint8_t>(independent?(a/4u+5u):8u),neutral),
         argb(0x9Au,accent)};
}
}