// SPDX-License-Identifier: MIT
#include <cassert>
#include <cstdio>
#include <vector>
#include "../firmware/main/Presentation.h"
using namespace crowpanel;
int main(){
 Presentation p;
 assert(p.header && p.eco);
 assert(p.eyeAt(316,344)==1 && p.eyeAt(708,344)==2);
 assert(p.eyeAt(100,20)==0 && p.eyeAt(100,570)==0);
 auto left=p.pixelAt(316,344);assert(left.x==36 && left.y==32);
 p.header=false;
 auto hidden=p.pixelAt(288,300);assert(hidden.x==36 && hidden.y==32);
 assert(p.eyeAt(288,300)==1 && p.eyeAt(736,300)==2);
 assert(p.eyeAt(-1,200)==0 && p.eyeAt(1024,200)==0);
 TouchState touch;Contact points[2]={{288,300},{736,300}};
 touch.sample(true,points,2,100,p);assert(touch.mask(199)==3);assert(touch.mask(200)==0);
 touch.sample(false,points,2,201,p);assert(touch.mask(201)==0);
 touch.sample(true,points,2,0xfffffff0,p);assert(touch.mask(20)==3);assert(touch.mask(100)==0);
 points[0]={100,570};touch.sample(true,points,1,300,p);assert(touch.mask(300)==0);
 // A responsive controller without a new packet must not renew a held touch.
 points[0]={288,300};touch.sample(true,points,1,400,p);
 touch.sample(true,points,1,490,p,false);assert(touch.mask(500)==0);
 Deadline timer;assert(!timer.active(0x80000001));
 timer.start(0xfffffff0,300);assert(timer.active(20));assert(!timer.active(284));
 assert(!timer.active(0x80000100));
 LayoutState layout;layout.wanted.header=false;layout.dirty=true;
 assert(layout.shown.header); // Failed lock leaves last displayed hit geometry intact.
 layout.commit();assert(!layout.shown.header&&!layout.dirty);
 uint8_t mono[1024]={};mono[0]=1;
 for(int scale:{7,8}){
  std::vector<uint16_t> framebuffer(128*64*scale*scale+2,0xabcd);
  expandPixels(mono,framebuffer.data()+1,scale,0xffff);
  assert(framebuffer.front()==0xabcd&&framebuffer.back()==0xabcd);
  for(int y=0;y<64*scale;y++)for(int x=0;x<128*scale;x++)
   assert(framebuffer[1+y*128*scale+x]==(x<scale&&y<scale?0xffff:0));
 }
 puts("PASS CrowPanel layout, touch and session defaults");
}
