// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cstdint>
namespace rlcd_eyes {
constexpr int width=400,height=300,frameBytes=15000,headerHeight=50;
// ST7305 landscape packing: pairs of columns, four inverted rows per byte.
// Matches the vendor's exact 400x300 panel, not a generic ST7305 size.
inline void pixel(uint8_t* buffer,int x,int y,bool white){
 if(x<0||x>=width||y<0||y>=height)return;
 const unsigned ry=height-1-y,index=(x/2)*75+ry/4,bit=7-((ry%4)*2+x%2);
 if(white)buffer[index]|=1U<<bit;else buffer[index]&=~(1U<<bit);
}
// Historical v0.1 renderer retained for regression comparisons and provenance.
// Production v0.2 uses NativeEyes.h; do not enlarge this bitmap on the panel.
inline void compose(const uint8_t* mono,uint8_t* buffer,bool header){
 std::fill(buffer,buffer+frameBytes,255);
 const int top=header?headerHeight:0,center=(top+height)/2;
 // A 4x crop of the canonical 128x64 canvas retains the complete 90x36
 // eye envelope, including jitter; only unused margins lie offscreen.
 for(int y=top;y<height;y++)for(int x=0;x<width;x++){
  const int sx=(x+56)/4,sy=(y-center+128)/4;
  if(sx>=0&&sx<128&&sy>=0&&sy<64&&(mono[(sy/8)*128+sx]&(1U<<(sy%8))))pixel(buffer,x,y,false);
 }
}
struct Header {bool visible=true;void toggle(){visible=!visible;}};
}
