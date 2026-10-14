#pragma once
#include <cstdint>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
namespace gyglass {
enum class Mode:uint8_t {Seamless=0,Independent=1};
struct PresetConfig {Mode mode=Mode::Seamless;uint8_t key_opacity_percent=16;};
struct PresetReply {char magic[4];uint8_t protocol,mode,renderer,key_opacity_percent;};
static_assert(sizeof(PresetReply)==8,"protocol size");
inline PresetReply make_reply(PresetConfig p) noexcept {
 return {{'G','Y','G','L'},2,static_cast<uint8_t>(p.mode),0,p.key_opacity_percent};
}
inline bool valid_reply(const PresetReply& r) noexcept {
 return std::memcmp(r.magic,"GYGL",4)==0&&r.protocol==2&&r.mode<2&&r.renderer==0&&
        r.key_opacity_percent>=1&&r.key_opacity_percent<=100;
}
inline PresetConfig decode_preset(const char* s,size_t n) noexcept {
 PresetConfig out{};if(!s||!n||n>96)return out;
 bool mode=false,opacity=false;
 for(size_t off=0;off<n;) {
  size_t end=off;while(end<n&&s[end]!='\n')++end;
  const auto len=end-off;const char* line=s+off;off=end<n?end+1:end;
  if(len==0)continue;
  if(len>=5&&std::memcmp(line,"MODE=",5)==0&&!mode) {
   if(len==13&&std::memcmp(line+5,"seamless",8)==0)out.mode=Mode::Seamless;
   else if(len==16&&std::memcmp(line+5,"independent",11)==0)out.mode=Mode::Independent;
   else return {};
   mode=true;
  } else if(len>=12&&std::memcmp(line,"KEY_OPACITY=",12)==0&&!opacity) {
   unsigned v=0;if(len==12||len>15)return {};
   for(size_t i=12;i<len;++i){if(line[i]<'0'||line[i]>'9')return {};v=v*10u+unsigned(line[i]-'0');}
   if(v<1||v>100)return {};
   out.key_opacity_percent=static_cast<uint8_t>(v);opacity=true;
  } else return {};
 }
 return mode?out:PresetConfig{};
}
inline PresetConfig read_preset_at(int dir) noexcept {
 if(dir<0)return {};
 int fd=openat(dir,"preset.conf",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
 if(fd<0)return {};
 struct stat st{};
 if(fstat(fd,&st)!=0||!S_ISREG(st.st_mode)||st.st_size>96){close(fd);return {};}
 char buf[97]{};ssize_t n=read(fd,buf,sizeof(buf));close(fd);
 return n>0&&n<=96?decode_preset(buf,static_cast<size_t>(n)):PresetConfig{};
}
}
