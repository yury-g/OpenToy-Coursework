// SPDX-License-Identifier: MIT
#define SILVER_RLCD_HOST_TEST
#include "../firmware/FeatherFace/SilverRlcd.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>
// Independently specified native ST7305 portrait decoder, not a production helper.
static bool white(const uint8_t *p,unsigned x,unsigned y){return p[(y/2)*75+x/4]&(0x80u>>((x%4)*2+y%2));}
static void rejects_bad_buffers_without_writing(){
 uint16_t source[200*150]={};uint8_t packed[15000];memset(packed,0x55,sizeof packed);
 assert(!silver_rlcd::encode(nullptr,30000,packed,sizeof packed));
 assert(!silver_rlcd::encode(source,29999,packed,sizeof packed));
 assert(!silver_rlcd::encode(source,30000,nullptr,sizeof packed));
 assert(!silver_rlcd::encode(source,30000,packed,sizeof packed-1));
 for(auto b:packed)assert(b==0x55);
}
static void white_background_and_nonzero_colors_fill_landscape(){
 std::vector<uint16_t> source(30000,0);uint8_t packed[15000];
 assert(silver_rlcd::encode(source.data(),source.size(),packed,sizeof packed));
 for(auto b:packed)assert(b==255);
 const uint16_t colors[]={0xffff,0xf800,0x07e0,0x001f,1};
 for(uint16_t color:colors){
  source.assign(30000,color);assert(silver_rlcd::encode(source.data(),source.size(),packed,sizeof packed));
  for(auto b:packed)assert(b==0); // 400x300 coverage: no margins or lost edges.
 }
}
static void golden_landscape_orientation_and_two_by_two_scaling(){
 // Logical x,y; hand-derived native byte and its final value. Mapping is
 // landscape X right / Y down -> native portrait x=299-Y, y=X.
 struct Case {unsigned sx,sy,index;uint8_t expected;};
 const Case cases[]={{0,0,74,0xf0},{199,0,14999,0xf0},{0,149,0,0x0f},
                     {199,149,14925,0x0f},{81,75,6112,0x0f}};
 for(const auto &test:cases){
  std::vector<uint16_t> source(30000,0);uint8_t packed[15000];source[test.sy*200+test.sx]=1;
  assert(silver_rlcd::encode(source.data(),source.size(),packed,sizeof packed));
  assert(packed[test.index]==test.expected);
  unsigned changed=0,black=0;
  for(unsigned i=0;i<15000;i++)if(packed[i]!=255)changed++;
  for(unsigned y=0;y<400;y++)for(unsigned x=0;x<300;x++)if(!white(packed,x,y))black++;
  assert(changed==1);assert(black==4); // Each logical pixel is exactly 2x2.
 }
}
static void writes_exact_native_length(){
 std::vector<uint16_t> source(30000,1);uint8_t guarded[15002];memset(guarded,0x66,sizeof guarded);
 assert(silver_rlcd::encode(source.data(),source.size(),guarded+1,15000));
 assert(guarded[0]==0x66&&guarded[15001]==0x66);
}
int main(){rejects_bad_buffers_without_writing();white_background_and_nonzero_colors_fill_landscape();golden_landscape_orientation_and_two_by_two_scaling();writes_exact_native_length();puts("PASS Silver RLCD landscape: rotation, exact2x scale, corner/interior byte goldens, colors, bounds");}
