#pragma once
#include <cstdint>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace gyglass {
enum class Mode : uint8_t { Seamless=0, Independent=1 };
struct PresetReply {
 char magic[4];
 uint8_t protocol;
 uint8_t mode;
 uint8_t renderer;
 uint8_t reserved;
};
static_assert(sizeof(PresetReply)==8);
inline PresetReply make_reply(Mode m) noexcept {
 return {{'G','Y','G','L'},1,static_cast<uint8_t>(m),0,0};
}
inline bool valid_reply(const PresetReply& r) noexcept {
 return r.magic[0]=='G' && r.magic[1]=='Y' && r.magic[2]=='G' &&
 r.magic[3]=='L' && r.protocol==1 && r.mode<=1 && r.renderer==0;
}
inline Mode decode_preset(const char* value,size_t n) noexcept {
 // The file contains one atomic key=value line written by preset.sh.
 constexpr char s[]="MODE=seamless";
 constexpr char i[]="MODE=independent";
 if(n && value[n-1]=='\n') --n;
 if(n==sizeof(i)-1 && std::memcmp(value,i,n)==0) return Mode::Independent;
 if(n==sizeof(s)-1 && std::memcmp(value,s,n)==0) return Mode::Seamless;
 return Mode::Seamless; // unknown/malformed settings fail closed
}
inline Mode read_preset_at(int dirfd) noexcept {
 if(dirfd<0) return Mode::Seamless;
 const int fd=openat(dirfd,"preset.conf",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
 if(fd<0) return Mode::Seamless;
 struct stat st{};
 if(fstat(fd,&st)!=0 || !S_ISREG(st.st_mode) || st.st_size>64) {
  close(fd);return Mode::Seamless;
 }
 char data[65]{};
 const ssize_t n=read(fd,data,sizeof(data));
 close(fd);
 if(n<0 || n>64) return Mode::Seamless;
 return decode_preset(data,static_cast<size_t>(n));
}
}
