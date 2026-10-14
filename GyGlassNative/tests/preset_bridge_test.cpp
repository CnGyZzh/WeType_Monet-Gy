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
 assert(decode_preset("MODE=seamless\n",14)==Mode::Seamless);
 assert(decode_preset("MODE=independent\n",17)==Mode::Independent);
 assert(decode_preset("MODE=independent\nEXTRA=1",24)==Mode::Seamless);
 assert(decode_preset("MODE=independently",18)==Mode::Seamless);
 const auto reply=make_reply(Mode::Independent);
 assert(valid_reply(reply) && reply.mode==1 && reply.renderer==0);
 auto broken=reply; broken.protocol=99;
 assert(!valid_reply(broken));
 char dirname[]="/tmp/gyglass-preset-XXXXXX";
 const char* dir=mkdtemp(dirname);assert(dir);
 const int folder=open(dir,O_RDONLY|O_DIRECTORY);
 assert(folder>=0);
 assert(read_preset_at(folder)==Mode::Seamless);
 int fd=openat(folder,"preset.conf",O_WRONLY|O_CREAT|O_TRUNC,0600);
 assert(fd>=0);
 const char text[]="MODE=independent\n";
 assert(write(fd,text,sizeof(text)-1)==static_cast<ssize_t>(sizeof(text)-1));
 close(fd);
 assert(read_preset_at(folder)==Mode::Independent);
 unlinkat(folder,"preset.conf",0);
 close(folder);
 rmdir(dirname);
 std::puts("GyGlass preset bridge tests passed");
}
