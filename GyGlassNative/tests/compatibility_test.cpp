#include "../native/compatibility.hpp"
#include <cassert>
using namespace gyglass;
int main() {
 auto current=parse_host_version("4.0.0.58111");
 assert(current.valid && current.build==58111);
 assert(classify_host(current)==HostProfile::KnownVersionNeedsRuntimeCheck);
 assert(classify_host(parse_host_version("4.0.0.58107"))==HostProfile::KnownVersionNeedsRuntimeCheck);
 assert(classify_host(parse_host_version("4.0.0.58112"))==HostProfile::SameFamilyNeedsProbe);
 assert(classify_host(parse_host_version("4.1.0.10000"))==HostProfile::Unknown);
 assert(!parse_host_version("4.0.0.58111-malformed").valid);
 assert(!parse_host_version("4.0.0").valid);
 assert(!parse_host_version("999999999999.0.0.0").valid);
}
