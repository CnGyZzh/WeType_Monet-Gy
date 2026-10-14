#include "../native/style.hpp"
#include <cassert>
using namespace gyglass;
unsigned alpha(uint32_t color){return color>>24;}
int main(){
 constexpr uint32_t purple=0xB5A8CA,blue=0xA2BEDA,accent=0x8874B9;
 assert(alpha(resolve(false,false,purple,accent,100).key)==0);
 assert(alpha(resolve(true,false,purple,accent,1).key)==3);
 assert(alpha(resolve(true,false,purple,accent,16).key)==41);
 assert(alpha(resolve(true,true,purple,accent,16).key)==41);
 assert(alpha(resolve(true,false,purple,accent,100).key)==255);
 assert(alpha(resolve(true,false,purple,accent,0).key)==41);
 assert(alpha(resolve(true,false,purple,accent,101).key)==41);
 assert((resolve(true,false,purple,accent,16).key&0xffffffu)==purple);
 assert((resolve(true,false,blue,accent,16).key&0xffffffu)==blue);
 assert(alpha(resolve(true,false,purple,accent,16).function_key)>41);
}