#include "../firmware/AnimatronicEyes/Power.h"
#include "../firmware/AnimatronicEyes/Companion.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace feather_face;
int main(){
 uint8_t mono[1024]={};RefreshGate g;DisplayStamp s;
 assert(g.changed(mono,s));assert(!g.changed(mono,s));
 mono[100]=1;assert(g.changed(mono,s));assert(!g.changed(mono,s));
 s.battery=50;assert(g.changed(mono,s));s.valid=true;assert(g.changed(mono,s));
 s.buttons=SELECT;assert(g.changed(mono,s));s.expression=ANGRY;assert(g.changed(mono,s));
 s.idle=true;assert(g.changed(mono,s));s.poke=1;assert(g.changed(mono,s));s.scale=1.5f;assert(g.changed(mono,s));
 for(unsigned id=1;id<=3;id++){
  Companion c(id);RefreshGate gate;uint16_t previous[240*135]={};unsigned skipped=0;
  for(unsigned t=0;t<30000;t+=50){
   auto p=c.update(512,512,0,false,t);drawCompanion(p,t,mono);auto tr=companionTransform(p);
   DisplayStamp stamp;stamp.scale=tr.scale;stamp.idle=p.idle.active;stamp.expression=p.eye[0].expression;
   uint16_t rendered[240*135];nesso_eyes::render(mono,rendered,tr,nullptr,0);
   if(!gate.changed(mono,stamp)){assert(!std::memcmp(previous,rendered,sizeof rendered));skipped++;}
   std::memcpy(previous,rendered,sizeof rendered);
  }
  assert(skipped>100); // Substantial unchanged display work can be omitted with identical output.
 }
 puts("PASS exact frame reuse, status/transform invalidation and identical displayed eyes at 20 Hz");
}
