// SPDX-License-Identifier: MIT
#pragma once
#include "TouchReaction.h"
#include <cmath>
namespace nesso_eyes {
struct IdlePose {
 bool active=false;int x[2]={},y[2]={};unsigned lid[2]={};
 FaceState apply(FaceState face,unsigned eye)const{
  if(active){face.pupilX=x[eye];face.pupilY=y[eye];if(eye)face.right=std::max(face.right,lid[eye]);else face.left=std::max(face.left,lid[eye]);}
  return face;
 }
};
struct IdleSettings {
 uint32_t seed=0x15ea1234;unsigned pace=100;int gazeRange=60;
 unsigned firstBlink=1500,winkPeriod=0;
 unsigned blinkStyle=0; // 0 classic, 1 double blink, 2 lingering, 3 alternating solo wink.
};
class Idle {
 struct Eye {float x=0,y=25;int targetX=0,targetY=25;uint32_t lookAt=0,blinkAt=0,blinkStart=0;bool blinking=false,echo=false;} eye_[2];
 IdleSettings settings_;uint32_t nextWink_=0;unsigned winkEye_=0;
 uint32_t period(uint32_t ms)const{return std::max<uint32_t>(1,ms*100/settings_.pace);}
 bool initialized_=false,running_=false;uint32_t lastTouch_=0,lastFrame_=0,nextLook_=0,rng_=0x15ea1234;
 uint32_t random(){rng_^=rng_<<13;rng_^=rng_>>17;rng_^=rng_<<5;return rng_;}
 static bool due(uint32_t now,uint32_t time){return int32_t(now-time)>=0;}
public:
 Idle(IdleSettings settings={}):settings_(settings),rng_(settings.seed){}
 uint32_t blinks[2]={},looks=0;
 IdlePose update(uint32_t now,bool touched){
  IdlePose p;
  if(!initialized_){initialized_=true;lastTouch_=lastFrame_=now;}
  uint32_t dt=std::min<uint32_t>(now-lastFrame_,50);lastFrame_=now;
  if(touched){lastTouch_=now;running_=false;return p;}
  if(uint32_t(now-lastTouch_)<900)return p;
  if(!running_){
   running_=true;nextLook_=now+period(100);nextWink_=now+5000;
   for(unsigned i=0;i<2;i++){
    eye_[i]=Eye{};eye_[i].lookAt=now;eye_[i].blinkAt=now+period(settings_.firstBlink+i*130);
    if(settings_.blinkStyle==1)eye_[i].blinkAt=now+1000+i*400;
    if(settings_.blinkStyle==2)eye_[i].blinkAt=now+(i?900:2100);
   }
   if(settings_.blinkStyle==3){winkEye_=1;nextWink_=now+650;}
  }
  p.active=true;
  if(due(now,nextLook_)){
   ++looks;int x=int(random()%(2*settings_.gazeRange+1))-settings_.gazeRange,y=20+random()%31;
   for(unsigned i=0;i<2;i++){eye_[i].targetX=x+int(random()%9)-4;eye_[i].targetY=y+int(random()%7)-3;eye_[i].lookAt=now+period(35+i*65);}
   nextLook_=now+period(1800+random()%1600);
  }
  if(!settings_.blinkStyle&&settings_.winkPeriod&&due(now,nextWink_)){
   auto& e=eye_[winkEye_];if(!e.blinking)e.blinkAt=now;
   winkEye_^=1;nextWink_=now+settings_.winkPeriod;
  }
  if(settings_.blinkStyle==3&&due(now,nextWink_)){
   auto& e=eye_[winkEye_];e.blinking=true;e.blinkStart=now;++blinks[winkEye_];
   winkEye_^=1;nextWink_=now+1000+random()%1000;
  }
  for(unsigned i=0;i<2;i++){
   auto& e=eye_[i];
   if(due(now,e.lookAt)){float ease=dt/(float(period(110+i*35))+dt);e.x+=(e.targetX-e.x)*ease;e.y+=(e.targetY-e.y)*ease;}
   p.x[i]=int(std::round(e.x));p.y[i]=int(std::round(e.y));
   if(settings_.blinkStyle!=3&&!e.blinking&&due(now,e.blinkAt)){e.blinking=true;e.blinkStart=now;++blinks[i];}
   if(e.blinking){
    uint32_t age=now-e.blinkStart,close=period(60+i*12),hold=period(40),open=period(110+i*25);
    if(settings_.blinkStyle==1){close=60+i*10;hold=60;open=100+i*20;}
    if(settings_.blinkStyle==2){close=450+i*150;hold=300+i*150;open=700+i*200;}
    if(settings_.blinkStyle==3){close=80;hold=200;open=170;}
    if(age<close)p.lid[i]=100*age/close;
    else if(age<close+hold)p.lid[i]=100;
    else if(age<close+hold+open)p.lid[i]=100*(close+hold+open-age)/open;
    else{
     e.blinking=false;
     if(settings_.blinkStyle==1){
      // A second fast blink, then a long irregular pause; each eye keeps its own phase.
      e.blinkAt=now+(e.echo?1700+random()%2300+i*750:140);e.echo=!e.echo;
     }else if(settings_.blinkStyle==2)e.blinkAt=now+4000+random()%4000;
     else if(settings_.blinkStyle!=3)e.blinkAt=now+period(2400+random()%2600+i*170);
    }
   }
  }
  return p;
 }
};
}
