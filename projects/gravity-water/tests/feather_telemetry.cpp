// SPDX-License-Identifier: MIT
#include <cassert>
#include <string>
#include "../firmware/GravityWaterFeather/Telemetry.h"
struct Port {
 bool connected=true;int capacity=0;std::string output;
 explicit operator bool() const{return connected;}
 int availableForWrite(){return capacity;}
 size_t write(const unsigned char* data,size_t n){assert(n<=size_t(capacity));output.append(reinterpret_cast<const char*>(data),n);capacity-=int(n);return n;}
};
int main(){
 feather_water::Telemetry log;Port port;
 log.append("report %d\n",42);log.pump(port);assert(log.pending());assert(port.output.empty());
 for(int i=0;i<8;++i){port.capacity=2;log.pump(port);}
 assert(!log.pending());assert(port.output=="report 42\n");
 log.append("old");port.connected=false;log.pump(port);assert(!log.pending());
 port.connected=true;log.append("new");port.capacity=64;log.pump(port);assert(port.output=="report 42\nnew");
 std::string oversized(2000,'x');log.append("%s",oversized.c_str());assert(!log.pending());
 log.append("ok");port.capacity=64;log.pump(port);assert(port.output=="report 42\nnewok");
}
