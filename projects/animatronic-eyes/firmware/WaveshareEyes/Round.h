// SPDX-License-Identifier: MIT
#pragma once
#if __has_include("Companion.h")
#include "Companion.h"
#else
#include "../AnimatronicEyes/Companion.h"
#endif
namespace waveshare_eyes {
struct Touch {bool down=false;int x=-1,y=-1;};
inline bool touchPacketValid(const uint8_t* data,bool ok){
 if(!ok||data[1]>1)return false;
 if(data[1]==0)return true;
 return (((data[2]&15)<<8)|data[3])<360&&(((data[4]&15)<<8)|data[5])<360&&(data[2]>>6)!=3;
}
inline Touch decodeTouch(const uint8_t* data,bool ok){
 if(!touchPacketValid(data,ok))return {};
 int x=((data[2]&15)<<8)|data[3],y=((data[4]&15)<<8)|data[5];
 if(x>=360||y>=360)return {};
 // Preserve lift coordinates. A release outside a button must cancel even if
 // no intermediate down sample was observed during the display transfer.
 return {data[1]==1&&(data[2]>>6)!=1,x,y};
}
// Both mapping and presentation use the same fixed projection. Performance
// mode hides the header without moving eyes or changing touch coordinates.
inline bool facePoint(int x,int y,int& sx,int& sy){
 if(x<0||x>=360||y<128||y>=280)return false;
 sx=(x+12)/3;sy=(y-94)/3; // exact integer inverse of the 3x projection
 return x>=0&&x<360&&y>=128&&y<280&&sx>=0&&sx<128&&sy>=0&&sy<64;
}
inline unsigned eyeAt(int x,int y){
 int sx,sy;if(!facePoint(x,y,sx,sy)||sy<12||sy>=52)return 0;
 return sx>=18&&sx<54?1:sx>=74&&sx<110?2:0;
}
inline void renderRow(const uint8_t* mono,int y,uint16_t* row,uint16_t color){
 for(int x=0;x<360;x++){
  int sx,sy;bool lit=facePoint(x,y,sx,sy)&&mono[(sy/8)*128+sx]&(1<<(sy%8));
  row[x]=lit&&(x-180)*(x-180)+(y-180)*(y-180)<176*176?color:0;
 }
}
}
