// SPDX-License-Identifier: MIT
#include "../firmware/RlcdEyes/Display.h"
#include "../firmware/RlcdEyes/Input.h"
#include <array>
#include <cassert>
#include <cstdio>
using namespace rlcd_eyes;
int main(){
 // Catch reversed rows/interleaved bit packing, aliasing and out-of-bounds writes.
 std::array<uint8_t,15002> guarded{};guarded.front()=0xA5;guarded.back()=0x5A;auto* pixels=guarded.data()+1;
 pixel(pixels,0,0,true);assert(pixels[74]==2);
 pixel(pixels,1,0,true);assert(pixels[74]==3);
 pixel(pixels,0,299,true);assert(pixels[0]==128);
 pixel(pixels,399,299,true);assert(pixels[14925]==64);
 pixel(pixels,-1,0,true);pixel(pixels,400,299,true);pixel(pixels,0,300,true);
 assert(guarded.front()==0xA5&&guarded.back()==0x5A);
 std::fill(pixels,pixels+15000,0);
 for(int y=0;y<300;y++)for(int x=0;x<400;x++)pixel(pixels,x,y,true);
 for(int i=0;i<15000;i++)assert(pixels[i]==255);
 uint8_t mono[1024]={};mono[4*128+64]=1;compose(mono,pixels,true);
 assert(!(pixels[7531]&128));assert(pixels[7574]&2);
 compose(mono,pixels,false);assert(!(pixels[7537]&32));
 Header h;assert(h.visible);h.toggle();assert(!h.visible);h.toggle();assert(h.visible);h.toggle();assert(Header{}.visible);
 puts("PASS RLCD pixel packing/coverage/bounds, viewport, session header");
}
