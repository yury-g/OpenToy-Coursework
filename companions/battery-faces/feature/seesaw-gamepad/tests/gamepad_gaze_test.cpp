#include <cassert>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include "../firmware/FeatherFace/DoodlePixels.h"
int main(){
 FaceState s=restingDoodle();uint8_t normal[1024],same[1024];
 drawDoodle(doodleTarget(s),s.expression,normal,0,0,0);
 drawDoodle(doodleTarget(s),s.expression,same,0,0,0,0,0,0);assert(!memcmp(normal,same,1024));
 struct {uint32_t before;uint8_t data[1024];uint32_t after;} shifted{0x12345678,{},0x87654321};
 drawDoodle(doodleTarget(s),s.expression,shifted.data,0,0,0,0,6,-4);
 assert(memcmp(normal,shifted.data,1024));
 for(unsigned x=0;x<128;x++)for(unsigned y=40;y<64;y++)assert((normal[y/8*128+x]&(1u<<(y%8)))==(shifted.data[y/8*128+x]&(1u<<(y%8))));
 assert(shifted.before==0x12345678&&shifted.after==0x87654321);
 for(unsigned e=0;e<6;e++)for(int x:{-6,6})for(int y:{-4,4}){
  s.expression=Expression(e);drawDoodle(doodleTarget(s),s.expression,shifted.data,0,0,0,0,x,y);
  assert(shifted.before==0x12345678&&shifted.after==0x87654321);
 }
}
