#include "../shared/opentoy/DoodlePixels.h"
#include <cassert>
#include <cstdio>
#include <set>
uint32_t hash(const uint8_t *b){uint32_t h=2166136261;for(unsigned i=0;i<1024;i++){h^=b[i];h*=16777619;}return h;}
int main(){
 uint8_t guarded[1026];std::set<uint32_t> hashes;
 for(const char *mood:{"neutral","happy","surprised","angry","sorry","nervous"}){
  char command[80];snprintf(command,sizeof(command),"0 0 0 0 0 0 0 %s",mood);FaceState s;assert(parseDoodle(command,s));
  guarded[0]=0x35;guarded[1025]=0x57;drawDoodle(doodleTarget(s),s.expression,guarded+1,0,0,0);
  assert(guarded[0]==0x35&&guarded[1025]==0x57);hashes.insert(hash(guarded+1));
 }
 assert(hashes.size()==6);
 FaceState s;assert(parseDoodle("0 0 0 0 0 0 0 neutral",s));uint8_t a[1024],b[1024];drawDoodle(doodleTarget(s),s.expression,a,0,0,0);
 s.jaw=100;drawDoodle(doodleTarget(s),s.expression,b,0,0,0);assert(hash(a)!=hash(b));assert(!memcmp(a,b,512));
 for(int axis:{-100,100})for(unsigned lid:{0U,100U}){
  s.eyeBend=s.browBend=s.mouthBend=s.mouthSkew=axis;s.left=s.right=lid;s.jaw=100;
  guarded[0]=0x35;guarded[1025]=0x57;drawDoodle(doodleTarget(s),s.expression,guarded+1,1,-1,1);assert(guarded[0]==0x35&&guarded[1025]==0x57);
 }
 DoodleAnimator animation;FaceState start=restingDoodle();animation.reset(start,0);assert(animation.settled());
 FaceState changed=start;changed.mouthBend=100;animation.set(changed,10);assert(!animation.settled());
 assert(animation.current.v[DMOUTH]!=doodleTarget(changed).v[DMOUTH]);
 for(unsigned t=60;t<=3060;t+=50)animation.step(t);
 assert(animation.settled());assert(!memcmp(animation.current.v,doodleTarget(changed).v,sizeof(animation.current.v)));
 // Include the final active-to-settled redraw, with no residual wiggle pixels.
 changed.eyeBend=-100;changed.jaw=100;animation.reset(start,8000);uint8_t presented[1024]={},expected[1024];bool finalRedraw=false;
 for(unsigned t=8050;t<=11050;t+=50)if(animation.tick(changed,t,true)){
  drawDoodle(animation.current,changed.expression,presented,animation.wiggle(0,t),animation.wiggle(1,t),animation.wiggle(2,t));
  if(animation.settled())finalRedraw=true;
 }
 drawDoodle(doodleTarget(changed),changed.expression,expected,0,0,0);
 assert(finalRedraw&&animation.settled()&&!memcmp(presented,expected,1024));
 animation.set(start,0xfffffff0U);for(uint32_t t=34;t<3034;t+=50)animation.step(t);assert(animation.settled());
 animation.reset(start,4000);assert(animation.settled());assert(animation.wiggle(2,4100)==0);
 // Hiding a moving face must let its spring settle without requesting display frames.
 animation.reset(start,4000);for(unsigned t=4050;t<=7050;t+=50)assert(!animation.tick(changed,t,false));
 assert(animation.settled());assert(!memcmp(animation.current.v,doodleTarget(changed).v,sizeof(animation.current.v)));
 puts("PASS six distinct procedural faces, mouth isolation, buffer bounds, spring settling, clock wrap and neutral reset");
}
