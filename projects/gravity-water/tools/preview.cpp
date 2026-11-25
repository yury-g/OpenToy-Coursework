// SPDX-License-Identifier: MIT
// Writes a PPM from the same particle solver and pixel renderer as the device.
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include "../firmware/GravityWaterNesso/src/fluid/Particles.h"
#include "../firmware/GravityWaterNesso/NessoPixels.h"
int main(int argc,char** argv) {
 gravity_water::Particles<135,227,400> pool;
 int ticks=argc>1?std::atoi(argv[1]):240;
 for(int i=0;i<ticks;i++) pool.step(i<240?0:256,i<240?256:0);
 uint16_t pixels[135*240]{};
 nesso_water::drawParticles(pixels,135,240,13,pool);
 std::printf("P6\n135 240\n255\n");
 for(auto swapped:pixels) {
  unsigned c=(swapped>>8)|(swapped<<8);
  unsigned char out[3]={static_cast<unsigned char>(((c>>11)&31)*255/31),static_cast<unsigned char>(((c>>5)&63)*255/63),static_cast<unsigned char>((c&31)*255/31)};
  std::fwrite(out,1,3,stdout);
 }
}
