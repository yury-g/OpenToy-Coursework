// SPDX-License-Identifier: MIT
#include <array>
#include <cassert>
#include "../firmware/GravityWaterNesso/src/fluid/Particles.h"
#include "../firmware/GravityWaterNesso/NessoPixels.h"
#include "../firmware/GravityWaterNesso/WaterOverlay.h"

static uint16_t decodeRgb565(uint16_t swapped) {
 return uint16_t((swapped>>8)|(swapped<<8));
}

static void drawCursorRing(uint16_t* buf,int w,int h,int cx,int cy,uint16_t color) {
 for(int dy=-5;dy<=5;++dy) for(int dx=-5;dx<=5;++dx) {
  int d2=dx*dx+dy*dy;
  if(d2>=16 && d2<=30) {
   int px=cx+dx,py=cy+dy;
   if(px>=0 && px<w && py>=0 && py<h) buf[py*w+px]=color;
  }
 }
}

static void checkDamageAndOverlay() {
 gravity_water::Particles<135,240,400> pool;
 std::array<uint16_t,135*240> full{},screen{};
 nesso_water::WaterDirtyTracker tracker;
 for(int tick=0;tick<720;tick++) {
  if(tick%37==0)pool.burst(400,-850,255);
  if(tick%9==0)pool.repel(tick%135,(tick*3)%240);
  // Cycle gravity in all directions including upside-down so beads enter & leave overlay rows [0,17).
  int gx=(tick>=240 && tick<360)?256:(tick>=360 && tick<480)?-256:0;
  int gy=(tick>=120 && tick<240)?-256:(tick>=480)?256:0;
  pool.step(gx,gy,tick%120<60,tick<540);
  int battery=(tick<100)?-1:(tick<200)?100:(tick<320)?72:(tick<450)?15:(tick<560)?4:-1;
  bool imuFresh=(tick%90)<70;
  bool showOverlay=(tick<620 || tick>=670);
  bool cursorActive=(tick>=80 && tick<400);
  int cx=20+(tick*3)%95,cy=5+(tick*2)%225; // cursor sweeps directly under top metadata too

  nesso_water::drawParticles(full.data(),135,240,0,pool);
  if(cursorActive)drawCursorRing(full.data(),135,240,cx,cy,nesso_water::pixelColor(255,216,126));
  nesso_water::drawWaterOverlay(full.data(),135,240,"WATER","0.3.1","2026-11-27",battery,imuFresh,showOverlay);

  auto rows=nesso_water::occupiedRows(pool,0,240);
  if(cursorActive) {
   rows.first=std::min(rows.first,std::max(0,cy-5));
   rows.last=std::max(rows.last,std::min(239,cy+5));
  }
  nesso_water::DirtySpan spanA,spanB;
  tracker.compute(rows,240,battery,imuFresh,showOverlay,spanA,spanB);
  if(spanA.valid()) for(int i=spanA.first*135;i<(spanA.last+1)*135;i++) screen[i]=full[i];
  if(spanB.valid()) for(int i=spanB.first*135;i<(spanB.last+1)*135;i++) screen[i]=full[i];
  assert(screen==full); // moving particles/cursor under metadata & label updates never leave stale pixels
 }
}

int main() {
 checkDamageAndOverlay();
 gravity_water::Particles<135,240,400> pool;
 std::array<uint16_t,135*240+2> storage,plain;
 for(int direction=0;direction<4;direction++) {
  for(int i=0;i<240;i++) pool.step(direction==0?256:direction==1?-256:0,direction==2?256:direction==3?-256:0);
  storage.fill(0xdead);
  nesso_water::drawParticles(storage.data()+1,135,240,0,pool);
  assert(storage.front()==0xdead && storage.back()==0xdead);
  int bright=0,topBright=0,bottomBright=0;
  for(int y=0;y<240;y++) {
   for(int x=0;x<135;x++) {
    uint16_t c=storage[1+y*135+x];
    assert(c!=0xdead); // entire 135x240 native canvas is rendered
    unsigned rgb=decodeRgb565(c);
    if(((rgb>>5)&63)>30) {
     bright++;
     if(y<17) topBright++;
     if(y>=223) bottomBright++;
    }
   }
  }
  assert(bright>1000); // individual bead highlights remain visible in each pose
  if(direction==3) assert(topBright>120);    // upside-down fills under top overlay
  if(direction==2) assert(bottomBright>120); // upright fills bottom edge
 }

 // Verify overlay preserves underlying water pixels outside glyphs/halo and stays within [0, kOverlayRows).
 plain.fill(0xdead);
 storage.fill(0xdead);
 nesso_water::drawParticles(plain.data()+1,135,240,0,pool);
 nesso_water::drawParticles(storage.data()+1,135,240,0,pool);
 nesso_water::drawWaterOverlay(storage.data()+1,135,240,"WATER","0.3.1","2026-11-27",-1,false,true);
 assert(storage.front()==0xdead && storage.back()==0xdead);
 for(int i=1+135*nesso_water::kOverlayRows;i<=135*240;i++) assert(storage[i]==plain[i]);
 // Corner (0,0) and middle gap (70,4) remain pure water pixels (no opaque full-width reserved bar).
 assert(storage[1+0]==plain[1+0]);
 assert(storage[1+4*135+70]==plain[1+4*135+70]);
 // Performance mode (showOverlay=false) leaves all pixels identical to plain water render.
 std::array<uint16_t,135*240> perf{};
 nesso_water::drawParticles(perf.data(),135,240,0,pool);
 nesso_water::drawWaterOverlay(perf.data(),135,240,"WATER","0.3.1","2026-11-27",85,false,false);
 for(int i=0;i<135*240;i++) assert(perf[i]==plain[1+i]);
}
