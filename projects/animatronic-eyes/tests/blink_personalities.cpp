#include "../firmware/AnimatronicEyes/Companion.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace feather_face;
struct Trace {std::vector<unsigned> starts[2],ends[2];unsigned overlap=0;};
Trace trace(unsigned id,uint32_t base=0){
 Companion c(id);Trace tr;unsigned previous[2]={};
 for(unsigned t=0;t<60000;t+=10){auto p=c.update(512,512,0,false,base+t);auto l=p.idle.lid;
  if(l[0]&&l[1])tr.overlap++;
  for(unsigned i=0;i<2;i++){if(l[i]&&!previous[i])tr.starts[i].push_back(t);if(!l[i]&&previous[i])tr.ends[i].push_back(t);previous[i]=l[i];assert(l[i]<=100);}
 }
 return tr;
}
int main(){
 auto fast=trace(1),slow=trace(2),wink=trace(3);
 for(unsigned i=0;i<2;i++){
  unsigned doubles=0;for(unsigned j=1;j<fast.starts[i].size();j++)if(fast.starts[i][j]-fast.starts[i][j-1]<700)doubles++;
  assert(doubles>=5); // Repeated quick double blinks, not just a faster common pattern.
  assert(slow.ends[i].size()>=4);
  for(unsigned j=0;j<slow.ends[i].size();j++)assert(slow.ends[i][j]-slow.starts[i][j]>=1400);
 }
 assert(slow.starts[1][0]<slow.starts[0][0]); // Dreamy's right eye leads.
 assert(wink.overlap==0&&wink.starts[0].size()>12&&wink.starts[1].size()>12);
 for(unsigned j=0;j<wink.starts[0].size();j++){
  assert(wink.starts[1][j]<wink.starts[0][j]);
  if(j+1<wink.starts[1].size())assert(wink.starts[0][j]<wink.starts[1][j+1]);
 }
 for(unsigned id=1;id<=3;id++){
  auto normal=trace(id),wrap=trace(id,0xfffff000);assert(normal.starts[0]==wrap.starts[0]&&normal.starts[1]==wrap.starts[1]);
  Companion c(id);for(unsigned t=0;t<12000;t+=10)c.update(512,512,0,false,t);
  auto p=c.update(512,512,0,false,12000,1);assert(!p.idle.active&&p.eye[0].left==100);
  assert(!c.update(512,512,0,false,12500).idle.active);assert(c.update(512,512,0,false,13000).idle.active);
 }
 puts("PASS distinct double blinks, slow lingering lids, right-first alternating solo winks, touch priority and timer wrap");
}
