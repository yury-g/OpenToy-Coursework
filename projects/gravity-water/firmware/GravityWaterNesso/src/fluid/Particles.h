// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>
namespace gravity_water {
// Fixed-point particle pool. Coordinates and velocities are Q8 pixels and
// Q8 pixels/tick; call step at 60 Hz. Gravity input is Q8 g in screen axes.
// A spatial grid bounds neighbour search. No Arduino, heap, display or clock.
template<int W,int H,int N> class Particles {
 public:
 static constexpr int radius=691, diameter=radius*2;
 struct Particle {int32_t x,y,vx,vy; uint8_t energy=0;};
 Particles() {
  const int columns=(W*256-2*radius)/diameter;
  static_assert(W>=16 && H>=16 && W<=320 && H<=320,"supported viewport");
  static_assert(N>0 && N<32767,"particle index capacity");
  static_assert(N<=((W*256-2*radius)/diameter)*((H*256-2*radius)/1200+1),"particles must fit initial pool");
  for(int i=0;i<N;i++) {
   int row=i/columns,col=i%columns;
   p_[i]={radius+col*diameter+(row%2?diameter/2:0),H*256-radius-row*1200,0,0};
   clampToBottle(p_[i]);
  }
 }
 int count() const {return N;}
 const Particle& particle(int i) const {return p_[i];}
 bool sleeping() const {return resting_;}
 int heat() const {return heat_;}
 int meanEnergy() const {int sum=0;for(const auto &p:p_)sum+=p.energy;return sum/N;}
 void burst(int dx,int dy,int strength) {
  wake();strength=std::max(0,std::min(255,strength));
  dx=std::max(-900,std::min(900,dx));dy=std::max(-900,std::min(900,dy));
  for(auto &p:p_) {
   p.vx=dx+(int(random()&511)-256)*strength/128;
   p.vy=dy+(int(random()&511)-256)*strength/128;
   p.x+=p.vx/2;p.y+=p.vy/2;clampToBottle(p);
   p.energy=std::max<int>(p.energy,strength);
  }
 }
 void repel(int x,int y) {
  if(x<0||x>=W||y<0||y>=H)return;
  wake();constexpr int reach=24*256;
  for(auto &p:p_) {
   int dx=p.x-x*256,dy=p.y-y*256;
   int distance=std::max(std::abs(dx),std::abs(dy))+3*std::min(std::abs(dx),std::abs(dy))/8;
   if(distance>=reach)continue;
   if(distance<32){dx=32*((random()&1)?1:-1);dy=32;distance=44;}
   int push=(reach-distance)/3;
   p.x+=dx*push/distance;p.y+=dy*push/distance;clampToBottle(p);
   p.vx=dx*600/distance;p.vy=dy*600/distance;p.energy=220;
  }
 }
 void step(int gx,int gy,bool heating=false,bool moving=false) {
  gx=std::max(-256,std::min(256,gx));gy=std::max(-256,std::min(256,gy));
  heat_=heating?std::min(255,heat_+12):std::max(0,heat_-20);
  if(heating||heat_||moving||std::abs(gx-restGX_)+std::abs(gy-restGY_)>8) {
   wake();restGX_=gx;restGY_=gy;
  } else if(quietTicks_<72) ++quietTicks_;
  if(quietTicks_>=72 && settledWindows_>=2) {
   resting_=true;
   for(auto &p:p_){p.vx=p.vy=0;p.energy=0;}
   return;
  }
  for(int i=0;i<N;i++) {
   auto &p=p_[i];oldX_[i]=p.x;oldY_[i]=p.y;
   if(heat_ && (random()&15)==0) {
    int lift=heat_*3;
    p.vx-=gx*lift/256;p.vy-=gy*lift/256;
    p.vx+=(int(random()&255)-128)*heat_/128;
    p.vy+=(int(random()&255)-128)*heat_/128;
   }
   p.vx=std::max<int32_t>(-1100,std::min<int32_t>(1100,p.vx+gx*48/256));
   p.vy=std::max<int32_t>(-1100,std::min<int32_t>(1100,p.vy+gy*48/256));
   p.x+=p.vx;p.y+=p.vy;clampToBottle(p);
  }
  // One positional relaxation pass resolve local overlaps. Pair corrections
  // are symmetric, so they do not create a directional force in the pool.
  for(int pass=0;pass<1;pass++) {
   std::fill(head_,head_+gridW*gridH,int16_t(-1));
   for(int i=0;i<N;i++) {int cell=cellOf(p_[i]);next_[i]=head_[cell];head_[cell]=i;}
   for(int i=0;i<N;i++) {
    auto &a=p_[i];int cx=(a.x/256)/cellSize,cy=(a.y/256)/cellSize;
    for(int y=std::max(0,cy-1);y<=std::min(gridH-1,cy+1);y++)
     for(int x=std::max(0,cx-1);x<=std::min(gridW-1,cx+1);x++)
      for(int j=head_[y*gridW+x];j>=0;j=next_[j]) {
       if(j<=i) continue;
       auto &b=p_[j];int dx=b.x-a.x,dy=b.y-a.y;
       int ax=std::abs(dx),ay=std::abs(dy);
       if(ax>=diameter||ay>=diameter) continue;
       // Octagonal norm is within ~7% of a circle; avoids software sqrt on C6.
       int distance=std::max(ax,ay)+3*std::min(ax,ay)/8;
       if(distance>=diameter) continue;
       if(!distance) {dx=((i+j)&1)?1:-1;dy=1;distance=1;}
       int overlap=(diameter-distance)/2;
       int ox=dx*overlap/distance,oy=dy*overlap/distance;
       a.x-=ox;a.y-=oy;b.x+=ox;b.y+=oy;
      }
   }
   for(auto &p:p_) clampToBottle(p);
  }
  for(int i=0;i<N;i++) {
   auto &p=p_[i];
   bool contact=std::abs((p.x-oldX_[i])-p.vx)+std::abs((p.y-oldY_[i])-p.vy)>16;
   int damping=contact?(heat_?238:170):244;
   p.vx=(p.x-oldX_[i])*damping/256;p.vy=(p.y-oldY_[i])*damping/256;
   int kinetic=std::min<int32_t>(255,(std::abs(p.vx)+std::abs(p.vy))/5);
   int target=std::max(heat_,kinetic);
   p.energy=(int(p.energy)*2+target)/3;
  }
  // Sleep only after the pool has stopped travelling, not merely after input stops.
  if(++centerTicks_>=12) {
   int32_t x=0,y=0;for(const auto &p:p_){x+=p.x;y+=p.y;}x/=N;y/=N;
   if(std::abs(x-centerX_)+std::abs(y-centerY_)<128)++settledWindows_;else settledWindows_=0;
   centerX_=x;centerY_=y;centerTicks_=0;
  }
 }
 private:
 int heat_=0,quietTicks_=0,restGX_=0,restGY_=0;
 int centerTicks_=0,settledWindows_=0;int32_t centerX_=0,centerY_=0;
 bool resting_=false;
 uint32_t randomState_=0x6a09e667;
 uint32_t random(){randomState_^=randomState_<<13;randomState_^=randomState_>>17;randomState_^=randomState_<<5;return randomState_;}
 void wake(){quietTicks_=0;settledWindows_=0;resting_=false;}
 static constexpr int cellSize=6,gridW=(W+cellSize-1)/cellSize,gridH=(H+cellSize-1)/cellSize;
 Particle p_[N];
 int32_t oldX_[N],oldY_[N];
 int16_t head_[gridW*gridH],next_[N];
 static void clampToBottle(Particle& p) {
  p.x=std::max<int32_t>(int32_t(radius),std::min<int32_t>(W*256-radius,p.x));
  p.y=std::max<int32_t>(int32_t(radius),std::min<int32_t>(H*256-radius,p.y));
 }
 static int cellOf(const Particle& p) {return (p.y/256/cellSize)*gridW+p.x/256/cellSize;}
};
}
