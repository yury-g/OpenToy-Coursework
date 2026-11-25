#include "../firmware/AnimatronicEyes/Controls.h"
#include "../shared/opentoy/DoodlePixels.h"
#include <cassert>
#include <cstdio>
using namespace feather_face;
int main(){
 Controls c(1);auto s=c.update(0,1023,0,true);assert(s.pupilX==100&&s.pupilY==100);
 s=c.update(0,0,SELECT,true);assert(s.y==-100&&s.pupilX==0);
 s=c.update(0,512,START,true);assert(s.right==100&&s.left==0);
 s=c.update(512,0,START,true);assert(s.left==100&&s.right==100&&s.bottomLids);
 s=c.update(512,1023,START,true);assert(!s.bottomLids);
 s=c.update(512,512,A,true);assert(s.expression==HAPPY);
 s=c.update(512,512,0,true);assert(s.expression==HAPPY);
 s=c.update(0,0,A,false);assert(s.expression==NEUTRAL&&s.pupilX==0&&s.left==0);
 Controls d(2);s=d.update(0,0,0,true);assert(s.browBend==100&&s.eyeBend==100);
 s=d.update(0,0,SELECT,true);assert(s.mouthSkew==100&&s.mouthBend==100&&s.eyeBend==0);
 s=d.update(0,0,START,true);assert(s.jaw==100&&s.right==100);
 s=d.update(512,512,SELECT|A,true);assert(s.expression==NERVOUS);
 d.update(512,512,0,true);s=d.update(512,512,SELECT|B,true);assert(s.expression==NEUTRAL);
 s=d.update(512,512,SELECT|START,true);assert(s.left==100&&s.right==100);
 s=d.update(0,0,START,false);assert(s.jaw==0&&s.left==0&&s.right==0);
 Controls presets(2);
 const uint32_t keys[]={0,A,X,B,Y,SELECT|A};
 const uint32_t hashes[]={0x59988171,0x52790a43,0xb0698fbd,0x4860388b,0xcbc3acd2,0x9e231224};
 for(int i=0;i<6;i++){
  presets.update(512,512,0,true);auto pose=presets.update(512,512,keys[i],true);uint8_t pixels[1024];
  drawDoodle(doodleTarget(pose),pose.expression,pixels,0,0,0);uint32_t h=2166136261U;
  for(auto b:pixels){h^=b;h*=16777619U;}assert(h==hashes[i]);
 }
 for(int i=450;i<575;i++)assert(Controls::axis(i)==0);
 assert(!fresh(true,100,100100));assert(fresh(true,0xfffffff0U,20));assert(!fresh(false,100,101));
 puts("PASS face layers, presets, axis directions, disconnect neutral and stale/wrap boundaries");
}
