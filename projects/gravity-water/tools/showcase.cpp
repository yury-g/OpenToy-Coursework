// SPDX-License-Identifier: MIT
// Deterministic scripted demo of the shipping solver/renderer. Not device footage.
// Frames: ASCII tick/phase/touch metadata, then 135x227 RGB bytes. 15fps output.
#include <cstdio>
#include <cmath>
#include "../firmware/GravityWaterNesso/src/fluid/Particles.h"
#include "../firmware/GravityWaterNesso/NessoPixels.h"
int main() {
 gravity_water::Particles<135,227,400> p;
 for(int i=0;i<600;i++)p.step(0,256);
 uint16_t pixels[135*240]{};
 for(int t=0;t<1260;t++) {
  int phase=t<120?0:t<300?1:t<420?2:t<600?3:t<780?4:t<960?5:6;
  int tx=-1,ty=-1,gx=0,gy=256;
  if(phase==3){tx=25+int(85*(.5+.5*std::sin((t-420)*.045)));ty=178+int(22*std::sin((t-420)*.07));p.repel(tx,ty);}
  if(phase==4 && t%8==0)p.burst(t%16?750:-750,-680,220);
  if(phase==5){float a=1.3f*std::sin((t-780)*6.2831853f/180);gx=int(std::sin(a)*256);gy=int(std::cos(a)*256);}
  p.step(gx,gy,phase==1,phase==3||phase==4||phase==5);
  if(t%4)continue;
  nesso_water::drawParticles(pixels,135,240,13,p);
  std::printf("%d %d %d %d\n",t,phase,tx,ty);
  for(int i=135*13;i<135*240;i++) {
   unsigned c=(pixels[i]>>8)|(pixels[i]<<8);
   unsigned char rgb[3]={static_cast<unsigned char>(((c>>11)&31)*255/31),static_cast<unsigned char>(((c>>5)&63)*255/63),static_cast<unsigned char>((c&31)*255/31)};
   std::fwrite(rgb,1,3,stdout);
  }
 }
}
