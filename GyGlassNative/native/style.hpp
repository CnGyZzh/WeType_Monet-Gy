#pragma once
#include <cstdint>
namespace gyglass {
struct Palette {uint32_t panel,key,function_key,toolbar,border,accent;};
constexpr uint32_t argb(uint8_t a,uint32_t rgb) noexcept {
  return (static_cast<uint32_t>(a)<<24)|(rgb&0x00FFFFFFu);
}
// Runtime renderer must provide system Monet RGB; never hardcode white keycaps.
constexpr Palette resolve(bool independent,bool dark,uint32_t neutral,uint32_t accent) noexcept {
  return {argb(dark?0x28:0x2D,neutral),
          argb(independent?(dark?0x39:0x29):0x00,neutral),
          argb(dark?0x45:0x39,neutral),
          argb(dark?0x2B:0x22,neutral),
          argb(independent?0x14:0x08,neutral),
          argb(0x9A,accent)};
}
}
