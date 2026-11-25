#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "../firmware/FeatherFace/GamepadFace.h"
static bool tick(GamepadFace &g,uint32_t now,uint16_t x=512,uint16_t y=512,uint32_t pressed=0){return g.update(true,x,y,GamepadFace::MASK&~pressed,now);}
static void press(GamepadFace &g,uint32_t now,uint32_t button){tick(g,now,512,512,button);assert(tick(g,now+30,512,512,button));tick(g,now+40);tick(g,now+70);}
int main(){
 GamepadFace g;tick(g,0);assert(!g.manual&&g.axisX==0&&g.axisY==0);
 for(unsigned i=1;i<20;i++)tick(g,i*25,500+i%20,510+i%15);
 assert(!g.manual); // Center noise must not take over battery mode.
 assert(tick(g,1000,0,512));assert(g.manual&&g.axisX==100&&g.axisY==0);
 for(unsigned i=0;i<20;i++){tick(g,1025+i*25,0,512);g.step(1025+i*25);}
 assert(g.lookX>5.5f&&g.lookX<=6&&g.lookY==0);
 tick(g,1600,1023,0);assert(g.axisX==-100&&g.axisY==-100); // Left/up physical orientation.
 for(unsigned i=0;i<30;i++){tick(g,1625+i*25);g.step(1625+i*25);}
 assert(g.lookX<.01f&&g.lookX>-.01f&&g.lookY>-.01f&&g.lookY<.01f);
 const uint32_t keys[]={GamepadFace::A,GamepadFace::B,GamepadFace::X,GamepadFace::Y,GamepadFace::START};
 const Expression expressions[]={HAPPY,SORRY,SURPRISED,ANGRY,NEUTRAL};
 for(unsigned i=0;i<5;i++){press(g,3000+i*100,keys[i]);assert(g.manual&&g.expression==expressions[i]);}
 unsigned count=g.presses;tick(g,4000,512,512,GamepadFace::A);tick(g,4010);tick(g,4020,512,512,GamepadFace::A);tick(g,4030);
 assert(g.presses==count); // Bounce must not become a new press.
 press(g,4100,GamepadFace::SELECT);assert(!g.manual);
 tick(g,4200,0,0,GamepadFace::SELECT);tick(g,4230,0,0,GamepadFace::SELECT);assert(!g.manual);
 tick(g,4300,0,0);tick(g,4330,0,0);assert(!g.manual); // Select wins until stick recenters.
 tick(g,4400);assert(tick(g,4430,0,0)&&g.manual);
 g.update(false,0,0,0,4500);assert(!g.manual&&!g.connected&&g.lookX==0&&g.lookY==0);
 tick(g,4600,512,512,GamepadFace::A);tick(g,4700,512,512,GamepadFace::A);assert(!g.manual); // Replug with held key is not a press.
 tick(g,4800);tick(g,4830);press(g,4900,GamepadFace::A);assert(g.manual&&g.expression==HAPPY);
 g.update(false,0,0,0,5000);tick(g,UINT32_MAX-10);tick(g,UINT32_MAX-5,512,512,GamepadFace::X);assert(tick(g,30,512,512,GamepadFace::X)&&g.expression==SURPRISED);
 g.update(true,65535,512,GamepadFace::MASK,100);assert(!g.manual&&!g.connected); // Invalid ADC is not a pose.
 puts("PASS gamepad orientation, deadzone, expressions, debounce, select, unplug, stale-held and wrap");
}
