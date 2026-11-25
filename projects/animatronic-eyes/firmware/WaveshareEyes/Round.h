// SPDX-License-Identifier: MIT
#pragma once
#if __has_include("Companion.h")
#include "Companion.h"
#else
#include "../AnimatronicEyes/Companion.h"
#endif
namespace waveshare_eyes {
struct Touch {bool down=false;int x=-1,y=-1;};
inline Touch decodeTouch(const uint8_t* data,bool ok){
 if(!ok||data[1]!=1)return {};
 int x=((data[2]&15)<<8)|data[3],y=((data[4]&15)<<8)|data[5];
 if(x>=360||y>=360||(data[2]>>6)==1||(data[2]>>6)==3)return {};
 return {true,x,y};
}
// Both mapping and presentation use the same fixed projection. Performance
// mode hides the header without moving eyes or changing touch coordinates.
inline bool facePoint(int x,int y,int& sx,int& sy){
 if(x<0||x>=360||y<100||y>=280)return false;
 sx=(x+12)/3;sy=(y-94)/3; // exact integer inverse of the 3x projection
 return x>=0&&x<360&&y>=100&&y<280&&sx>=0&&sx<128&&sy>=0&&sy<64;
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
enum class Action {None,Header,Mute,Personality};
inline Action controlAt(const Touch& t){
 if(!t.down)return Action::None;
 if(t.x>=140&&t.x<=220&&t.y>=10&&t.y<=40)return Action::Header;
 if(t.y>=290&&t.y<=330){if(t.x>=90&&t.x<=170)return Action::Mute;if(t.x>=190&&t.x<=270)return Action::Personality;}
 return Action::None;
}
class HoldControl {
 bool down_=false,fired_=false;Action zone_=Action::None;uint32_t start_=0;
public:
 Action update(Touch t,uint32_t now){
  if(!t.down){down_=fired_=false;zone_=Action::None;return Action::None;}
  auto zone=controlAt(t);
  if(!down_){down_=true;zone_=zone;start_=now;}
  if(zone!=zone_)fired_=true; // sliding into or out of a control cancels until release
  if(!fired_&&zone_!=Action::None&&uint32_t(now-start_)>=800){fired_=true;return zone_;}
  return Action::None;
 }
};
}
