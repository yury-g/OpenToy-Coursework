// SPDX-License-Identifier: MIT
#include "../core/Tracking.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
using namespace copy_face;
Detection face(float x=.25f,float confidence=.9f) {
 Detection d; d.x=x; d.y=.25f; d.w=.2f; d.h=.4f; d.confidence=confidence; return d;
}
void feed(Tracker& t,uint32_t now,Detection d=face()) {
 Frame f; f.captured=now; f.count=1; f.faces[0]=d; t.observe(f,now);
}
void acquire(Tracker& t,uint32_t start=10,Detection d=face()) {
 feed(t,start,d); feed(t,start+50,d); feed(t,start+100,d);
}
int main() {
 { Tracker t; acquire(t); auto p=t.step(110); assert(p.tracking); t.step(210); p=t.step(310); assert(p.x<-.2f); }
 { Tracker t; acquire(t); Frame f; f.count=2; f.faces[0]=face(.7f,.99f); f.faces[1]=face(.26f,.6f);
   f.captured=160; t.observe(f,160); t.step(160); auto p=t.step(360); assert(p.x<0 && p.tracking); }
 { Tracker t; acquire(t); t.step(110); auto p=t.step(700); assert(!p.tracking); assert(std::fabs(p.x)<.01f); }
 { Tracker t; acquire(t); feed(t,200,face(.7f)); feed(t,300,face(.7f));
   assert(t.step(300).tracking); feed(t,600,face(.7f));feed(t,650,face(.7f));feed(t,700,face(.7f));
   t.step(700); assert(t.step(900).x>0); }
 { Tracker t; acquire(t,10,face(.25f,.69f)); assert(!t.step(120).tracking);
   auto d=face(); d.x=std::numeric_limits<float>::quiet_NaN(); acquire(t,200,d); assert(!t.step(310).tracking); }
 { Tracker t; Frame f;f.count=1;f.faces[0]=face();f.captured=10;
   t.observe(f,500);t.observe(f,550);t.observe(f,600);assert(!t.step(600).tracking); }
 { Tracker t; acquire(t);t.step(110);Frame f;f.count=1;f.faces[0]=face(.35f);f.captured=109;
   t.observe(f,120);assert(t.step(210).x<-.18f);assert(t.step(559).tracking); }
 { Tracker t; acquire(t); auto a=t.step(110,false);a=t.step(310,false);
   Tracker u; acquire(u);u.step(110,true);auto b=u.step(310,true);assert(std::fabs(a.x+b.x)<1e-5f && a.y==b.y); }
 { Tracker t; auto d=face(); d.aperture=1; d.bend=1; acquire(t,10,d);
   t.step(110);auto p=t.step(310);assert(p.aperture==0 && p.bend==0);
   d.mouthValid=true; feed(t,320,d);p=t.step(420);assert(p.aperture>.5f && p.bend>.5f);
   d.mouthValid=false;feed(t,430,d);p=t.step(630);assert(p.aperture<.2f); }
 { Tracker t;acquire(t,UINT32_MAX-80);auto p=t.step(20);assert(p.tracking);assert(!t.step(600).tracking); }
 { Tracker a,b; acquire(a);acquire(b);a.step(110);b.step(110);
   Pose pa,pb;for(unsigned x=120;x<=310;x+=10)pa=a.step(x);pb=b.step(310);
   assert(std::fabs(pa.x-pb.x)<1e-5f); }
 { Tracker t; Frame f;f.count=9;f.captured=1;t.observe(f,1);assert(!t.step(1).tracking);
   auto d=face(); d.w=-1;acquire(t,20,d);assert(!t.step(130).tracking); }
 { Tracker t; Frame f;f.count=1;f.faces[0]=face();f.captured=100;
   t.observe(f,100);t.observe(f,150);t.observe(f,200);assert(!t.step(200).tracking); }
 { Tracker t;feed(t,10);feed(t,60);feed(t,400);assert(!t.step(400).tracking);
   feed(t,450);feed(t,500);assert(t.step(500).tracking); }
 { Tracker t;Frame f;f.count=2;f.faces[0]=face(.1f,.95f);f.faces[1]=face(.7f,.8f);
   f.captured=10;t.observe(f,10);f.faces[0]=face(.7f,.99f);f.faces[1]=face(.1f,.8f);
   f.captured=60;t.observe(f,60);f.captured=110;t.observe(f,110);t.step(110);assert(t.step(210).x<0); }
 { Tracker t;auto d=face();d.lips.valid=true;
   for(unsigned i=0;i<lipPoints;i++){float x=.3f+.1f*i/(lipPoints-1);d.lips.upper[i]={x,.6f};d.lips.lower[i]={x,.6f};}
   acquire(t,10,d);t.step(110);auto p=t.step(210);assert(p.lips.valid);
   Detection selected;uint32_t captured;assert(t.selected(selected,captured,210)&&captured==110);
   Frame f;f.captured=220;f.count=2;f.faces[0]=face(.7f,.99f);f.faces[0].lips=d.lips;f.faces[1]=d;
   t.observe(f,220);assert(t.selected(selected,captured,220)&&selected.x==d.x);
   assert(!t.selected(selected,captured,700));
 }
 puts("tracking: 16 scenarios passed");
}
