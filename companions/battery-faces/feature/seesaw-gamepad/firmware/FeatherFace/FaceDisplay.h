// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
#if defined(FACE_SILVER) && FACE_SILVER
constexpr int DISPLAY_WIDTH=200,DISPLAY_HEIGHT=150,FACE_X=36,FACE_Y=75,FACE_W=128,FACE_H=64;
#elif defined(FACE_WAVESHARE_169)
constexpr int DISPLAY_WIDTH=240,DISPLAY_HEIGHT=280,FACE_X=16,FACE_Y=164,FACE_W=208,FACE_H=104;
#else
constexpr int DISPLAY_WIDTH=135,DISPLAY_HEIGHT=240,FACE_X=3,FACE_Y=157,FACE_W=128,FACE_H=64;
#endif
inline uint16_t rainbowInk(unsigned x,unsigned width=128){
 const uint16_t colors[]={0xf800,0xfd20,0xffe0,0x07e0,0x07ff,0x041f,0xf81f};
 return colors[(x*7/width)%7];
}
inline void scaleFace(const uint8_t *source,uint16_t *target,uint16_t ink,bool rainbow=false) {
 for(int y=0;y<DISPLAY_HEIGHT;y++)for(int x=0;x<DISPLAY_WIDTH;x++) {
  bool on=false;int sx=0;
  if(x>=FACE_X && x<FACE_X+FACE_W && y>=FACE_Y && y<FACE_Y+FACE_H) {
   sx=(x-FACE_X)*128/FACE_W;int sy=(y-FACE_Y)*64/FACE_H;
   on=(source[(sy/8)*128+sx]>>(sy%8))&1;
  }
  target[y*DISPLAY_WIDTH+x]=on?(rainbow?rainbowInk(sx):ink):0;
 }
}
inline uint32_t faceHash(const uint8_t *source) {
 uint32_t h=2166136261U;for(unsigned i=0;i<1024;i++)h=(h^source[i])*16777619U;return h;
}
