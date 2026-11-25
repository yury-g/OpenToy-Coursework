// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
#include "PeerBattery.h"
inline unsigned nextFace(unsigned selected,int direction){for(unsigned n=0;n<FACE_COUNT;n++){selected=(selected+(direction<0?FACE_COUNT-1:1))%FACE_COUNT;if(faceEnabled(selected))return selected;}return 0;}
struct SwipeGesture {
 bool active=false;int startX=0,startY=0,lastX=0,lastY=0;uint32_t began=0;
 // 1=left/next, -1=right/previous, 2=short tap, 3=up/cameras, -3=down/cameras; emit only once on release.
 int update(bool down,int x,int y,uint32_t now){
  if(down){if(!active){active=true;startX=x;startY=y;began=now;}lastX=x;lastY=y;return 0;}
  if(!active)return 0;active=false;
  uint32_t elapsed=now-began;int dx=lastX-startX,dy=lastY-startY,ax=dx<0?-dx:dx,ay=dy<0?-dy:dy;
  if(ax<12 && ay<12 && elapsed>=600)return 4;
  if(elapsed<40 || elapsed>1500)return 0;
  if(ax>=30 && ax*2>=ay*3)return dx<0?1:-1;
  if(ay>=30 && ay>=ax*3)return dy<0?3:-3;
  if(ax<12 && ay<12 && elapsed<=400)return 2;
  return 0;
 }
};
