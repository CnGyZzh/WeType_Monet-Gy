#include "../native/preset_bridge.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
using namespace gyglass;
int main(){
 auto old=decode_preset("MODE=independent\n",17);
 assert(old.mode==Mode::Independent && old.key_opacity_percent==16);
 auto updated=decode_preset("MODE=independent\nKEY_OPACITY=100\n",33);
 assert(updated.mode==Mode::Independent && updated.key_opacity_percent==100);
 auto min=decode_preset("MODE=independent\nKEY_OPACITY=1\n",31);
 assert(min.key_opacity_percent==1);
 auto bad=decode_preset("MODE=independent\nKEY_OPACITY=101\n",33);
 assert(bad.mode==Mode::Seamless && bad.key_opacity_percent==16);
 assert(decode_preset("MODE=independently",18).mode==Mode::Seamless);
 auto reply=make_reply(updated);
 assert(valid_reply(reply)&&reply.mode==1&&reply.key_opacity_percent==100);
 reply.protocol=1; assert(!valid_reply(reply));
 char dirname[]="/tmp/gyglass-preset-XXXXXX";
 const char* dir=mkdtemp(dirname); assert(dir);
 const int folder=open(dir,O_RDONLY|O_DIRECTORY);assert(folder>=0);
 assert(read_preset_at(folder).mode==Mode::Seamless);
 int fd=openat(folder,"preset.conf",O_WRONLY|O_CREAT|O_TRUNC,0600);
 assert(fd>=0);
 const char text[]="MODE=independent\nKEY_OPACITY=16\n";
 assert(write(fd,text,sizeof(text)-1)==static_cast<ssize_t>(sizeof(text)-1));
 close(fd);
 auto readback=read_preset_at(folder);
 assert(readback.mode==Mode::Independent&&readback.key_opacity_percent==16);
 unlinkat(folder,"preset.conf",0);close(folder);rmdir(dirname);
 std::puts("GyGlass preset bridge tests passed");
}