#include "../native/style.hpp"
#include <cassert>
using namespace gyglass;
unsigned a(uint32_t c) {return c>>24;}
int main(){
 constexpr uint32_t purple=0xB5A8CA,blue=0xA2BEDA,accent=0x8874B9;
 auto seamless=resolve(false,false,purple,accent);
 auto independent=resolve(true,false,purple,accent);
 assert(a(seamless.key)==0 && a(independent.key)==0x29);
 assert(a(independent.key)<a(independent.function_key));
 assert((independent.key&0xFFFFFFu)==purple);
 assert((resolve(true,false,blue,accent).key&0xFFFFFFu)==blue);
 assert(a(resolve(true,true,purple,accent).key)>a(independent.key));
}
