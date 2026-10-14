#pragma once
#include <cstdint>
#include <string_view>
namespace gyglass {
struct HostVersion {
 uint32_t major=0,minor=0,patch=0,build=0;
 bool valid=false;
};
constexpr HostVersion parse_host_version(std::string_view value) noexcept {
 HostVersion r;
 uint32_t* fields[]={&r.major,&r.minor,&r.patch,&r.build};
 for(int i=0;i<4;++i) {
   if(value.empty() || value.front()<'0' || value.front()>'9') return r;
   uint32_t number=0;
   int digits=0;
   while(!value.empty() && value.front()>='0' && value.front()<='9') {
     if(++digits>9) return r;
     number=number*10u+static_cast<uint32_t>(value.front()-'0');
     value.remove_prefix(1);
   }
   *fields[i]=number;
   if(i!=3) { if(value.empty() || value.front()!='.') return r; value.remove_prefix(1); }
 }
 r.valid=value.empty();
 return r;
}
enum class HostProfile { KnownVersionNeedsRuntimeCheck, SameFamilyNeedsProbe, Unknown };
constexpr HostProfile classify_host(const HostVersion& v) noexcept {
 if(!v.valid || v.major!=4 || v.minor!=0 || v.patch!=0) return HostProfile::Unknown;
 if(v.build==58107 || v.build==58111) return HostProfile::KnownVersionNeedsRuntimeCheck;
 return HostProfile::SameFamilyNeedsProbe;
}
}
