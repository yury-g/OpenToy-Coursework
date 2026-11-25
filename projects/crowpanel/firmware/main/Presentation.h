// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <algorithm>
namespace crowpanel {
inline void expandPixels(const uint8_t* mono,uint16_t* output,int scale,uint16_t ink){
 const int width=128*scale;
 for(int y=0;y<64;y++){
  auto* row=output+y*scale*width;
  for(int x=0;x<128;x++)std::fill_n(row+x*scale,scale,(mono[(y/8)*128+x]&(1<<(y%8)))?ink:0);
  for(int copy=1;copy<scale;copy++)std::copy_n(row,width,row+copy*width);
 }
}
struct Contact { int x=-1,y=-1; };
struct Presentation {
 bool header=true,eco=true;
 int scale()const{return header?7:8;}
 int left()const{return (1024-128*scale())/2;}
 int top()const{return (header?88:0)+(600-(header?88:0)-64*scale())/2;}
 Contact pixelAt(int x,int y)const{return {(x-left())/scale(),(y-top())/scale()};}
 unsigned eyeAt(int x,int y)const{
  if(x<left()||x>=left()+128*scale()||y<top()||y>=top()+64*scale())return 0;
  return pixelAt(x,y).x<64?1:2;
 }

};
class TouchState {
 uint32_t at_=0;unsigned mask_=0;bool valid_=false;
public:
 void sample(bool ok,const Contact* points,unsigned count,uint32_t now,const Presentation& p,bool newPacket=true){
  if(ok&&!newPacket)return;
  at_=now;valid_=ok&&count<=5;mask_=0;
  if(valid_)for(unsigned i=0;i<count;i++)mask_|=p.eyeAt(points[i].x,points[i].y);
 }
 unsigned mask(uint32_t now)const{return valid_&&uint32_t(now-at_)<100?mask_:0;}
};
struct LayoutState {
 Presentation wanted,shown;bool dirty=true;
 void commit(){shown=wanted;dirty=false;}
};
class Deadline {
 bool running_=false;uint32_t start_=0,duration_=0;
public:
 void start(uint32_t now,uint32_t duration){start_=now;duration_=duration;running_=true;}
 bool active(uint32_t now){if(running_&&uint32_t(now-start_)>=duration_)running_=false;return running_;}
};
}
