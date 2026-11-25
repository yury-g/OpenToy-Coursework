// SPDX-License-Identifier: MIT
#include <array>
#include <cassert>
#include "../firmware/GravityWaterNesso/src/fluid/Particles.h"
#include "../firmware/GravityWaterNesso/NessoPixels.h"
static void checkDamage() {
 gravity_water::Particles<135,227,400> pool;
 std::array<uint16_t,135*240> full{},screen{};
 nesso_water::Rows previous{13,239};
 for(int tick=0;tick<600;tick++) {
  if(tick%31==0)pool.burst(500,-850,255);
  if(tick%9==0)pool.repel(tick%135,tick%227);
  pool.step(tick<300?0:256,tick<300?256:0,tick%100<50);
  nesso_water::drawParticles(full.data(),135,240,13,pool);
  auto rows=nesso_water::occupiedRows(pool,13,240);
  int first=std::min(previous.first,rows.first),last=std::max(previous.last,rows.last);
  for(int i=first*135;i<(last+1)*135;i++)screen[i]=full[i];
  assert(screen==full); // partial transfers leave no trails or missing color changes
  previous=rows;
 }
}
int main() {
 checkDamage();
 gravity_water::Particles<135,227,400> pool;
 std::array<uint16_t,135*240+2> storage;
 for(int direction=0;direction<4;direction++) {
  for(int i=0;i<240;i++) pool.step(direction==0?256:direction==1?-256:0,direction==2?256:direction==3?-256:0);
  storage.fill(0xdead);
  nesso_water::drawParticles(storage.data()+1,135,240,13,pool);
  assert(storage.front()==0xdead && storage.back()==0xdead);
  for(int i=1;i<=135*13;i++) assert(storage[i]==0xdead);
  int bright=0;
  for(int i=1+135*13;i<=135*240;i++) {
   auto c=storage[i];unsigned rgb=(c>>8)|(c<<8);
   if(((rgb>>5)&63)>30)bright++;
  }
  assert(bright>1000); // individual bead highlights remain visible in each pose
 }
}
