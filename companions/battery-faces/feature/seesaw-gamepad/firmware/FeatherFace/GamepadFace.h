// SPDX-License-Identifier: MIT
#pragma once
#include "DoodleProtocol.h"
// Input-only controller. It never starts a motor, battery test or radio action.
struct GamepadFace {
 static constexpr uint32_t A=1u<<5,B=1u<<1,X=1u<<6,Y=1u<<2,START=1u<<16,SELECT=1u;
 static constexpr uint32_t MASK=A|B|X|Y|START|SELECT;
 bool manual=false,connected=false;int axisX=0,axisY=0;float lookX=0,lookY=0;unsigned presses=0;Expression expression=NEUTRAL;
 uint32_t buttons=0,rawButtons=0,changedAt[6]={},lastStep=0;bool stickArmed=true,hasStep=false;
 static int axis(unsigned value){int n=int(value)-512;if(n>=-64&&n<=64)return 0;return n<0?(n+64)*100/448:(n-64)*100/447;}
 void automatic(){manual=false;lookX=lookY=0;stickArmed=false;}
 bool update(bool valid,uint16_t rawX,uint16_t rawY,uint32_t high,uint32_t now){
  if(!valid||rawX>1023||rawY>1023){connected=false;automatic();axisX=axisY=0;buttons=rawButtons=0;hasStep=false;return false;}
  uint32_t down=~high&MASK;bool first=!connected;
  if(first){connected=true;rawButtons=buttons=down;for(auto &t:changedAt)t=now;lastStep=now;hasStep=true;}
  const uint32_t keys[]={A,B,X,Y,START,SELECT};uint32_t rising=0;
  for(unsigned i=0;i<6;i++){
   uint32_t key=keys[i];if((rawButtons^down)&key){rawButtons^=key;changedAt[i]=now;}
   if(((buttons^rawButtons)&key)&&uint32_t(now-changedAt[i])>=25){buttons^=key;if(buttons&key)rising|=key;}
  }
  int oldX=axisX,oldY=axisY;axisX=axis(1023-rawX);axisY=axis(rawY); // Right/down are positive screen coordinates.
  if(buttons&SELECT){if(rising&SELECT)presses++;automatic();return (rising&SELECT)!=0;}
  if(!axisX&&!axisY)stickArmed=true;
  bool moved=!first&&stickArmed&&(axisX||axisY)&&(oldX!=axisX||oldY!=axisY);
  bool event=false;
  for(unsigned i=0;i<5;i++)if(rising&keys[i]){const Expression moods[]={HAPPY,SORRY,SURPRISED,ANGRY,NEUTRAL};expression=moods[i];manual=true;presses++;event=true;}
  if(moved){manual=true;event=true;}
  return event;
 }
 void step(uint32_t now){
  if(!manual||!connected){lookX=lookY=0;lastStep=now;hasStep=true;return;}
  uint32_t elapsed=hasStep?uint32_t(now-lastStep):0;lastStep=now;hasStep=true;if(elapsed>100)elapsed=100;
  float blend=float(elapsed)/(80+elapsed);lookX+=(axisX*.06f-lookX)*blend;lookY+=(axisY*.04f-lookY)*blend;
 }
};
