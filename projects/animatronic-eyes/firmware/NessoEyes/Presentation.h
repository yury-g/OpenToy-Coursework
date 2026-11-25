// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace nesso_eyes {
constexpr float tau=6.2831853071795864769f;
// Fixed landscape framebuffer. Gravity's down vector in that frame is (ax,-ay),
// from the owner-verified Gravity Water axes. Interpolate on the shortest arc;
// never switch LCD rotation or allocate a framebuffer during movement.
class Orientation {
 float angle_=0,target_=0;uint32_t sampled_=0,advanced_=0;bool valid_=false,started_=false;
public:
 void sample(uint32_t now,float x,float y,float z,float spin){
  float xy=x*x+y*y,mag=xy+z*z;
  valid_=std::isfinite(mag)&&std::isfinite(spin)&&mag>=.72f&&mag<=1.30f&&xy>=.15f&&std::fabs(spin)<800;
  if(valid_){target_=std::atan2(-x,-y);sampled_=now;}
 }
 void advance(uint32_t now){
  uint32_t dt=started_?std::min<uint32_t>(now-advanced_,50):0;started_=true;advanced_=now;
  if(!valid_||uint32_t(now-sampled_)>250)return;
  float delta=std::remainder(target_-angle_,tau),step=delta*dt/(80.f+dt),limit=dt*.009425f;
  angle_=std::remainder(angle_+std::max(-limit,std::min(limit,step)),tau);
 }
 float angle()const{return angle_;}
};
struct Point {float x,y;};
struct Transform {
 float c,s,scale,viewWidth,viewHeight;
 explicit Transform(float angle=0):c(std::cos(angle)),s(std::sin(angle)){
  float ac=std::fabs(c),as=std::fabs(s);
  // An inset, smoothly changing upright viewport anchors the rotating status
  // strip. Eyes use their tight 90x36 envelope, including maximum jitter.
  viewWidth=234*c*c+129*s*s;viewHeight=129*c*c+234*s*s;
  float fit=std::min(234/(viewWidth*ac+viewHeight*as),129/(viewWidth*as+viewHeight*ac));
  viewWidth*=fit;viewHeight*=fit;
  scale=std::min(234/(90*ac+36*as),129/(90*as+36*ac));
  scale=std::min(scale,(viewHeight-30)/36); // Keep eyes clear of the status strip.
 }
 Point local(float x,float y)const{x-=120;y-=67.5f;return {c*x+s*y,-s*x+c*y};}
 Point screen(float x,float y)const{x*=scale;y*=scale;return {120+c*x-s*y,67.5f+s*x+c*y};}
 Point headerPoint(float x,float y)const{x-=viewWidth/2;y-=viewHeight/2;return {120+c*x-s*y,67.5f+s*x+c*y};}
 bool header(float x,float y)const{auto p=local(x,y);return p.x>=-viewWidth/2&&p.x<viewWidth/2&&p.y>=-viewHeight/2&&p.y<-viewHeight/2+15;}
 bool face(float x,float y)const{return x>=0&&x<240&&y>=0&&y<135&&!header(x,y);}
 unsigned eye(float x,float y)const{
  if(!face(x,y))return 0;auto p=local(x,y);float px=p.x/scale+64,py=p.y/scale+32;
  if(py<12||py>51)return 0;
  return px>=18&&px<=53?1:px>=74&&px<=109?2:0;
 }
};
// One inverse transform per scanline, then integer additions per pixel: the
// C6 has no floating-point unit. 16.16 coordinates use pixel centers.
inline void render(const uint8_t* mono,uint16_t* buffer,const Transform& tr,const uint16_t* hud,int hudWidth){
 const int32_t fxStep=int32_t(tr.c/tr.scale*65536),fyStep=int32_t(-tr.s/tr.scale*65536);
 const int32_t hxStep=int32_t(tr.c*65536),hyStep=int32_t(-tr.s*65536);
 for(int y=0;y<135;y++){
  auto p=tr.local(.5f,y+.5f);
  int32_t fx=int32_t((p.x/tr.scale+64)*65536),fy=int32_t((p.y/tr.scale+32)*65536);
  int32_t hx=int32_t((p.x+tr.viewWidth/2)*65536),hy=int32_t((p.y+tr.viewHeight/2)*65536);
  for(int x=0;x<240;x++,fx+=fxStep,fy+=fyStep,hx+=hxStep,hy+=hyStep){
   int sx=fx>>16,sy=fy>>16,hxPixel=hx>>16,hyPixel=hy>>16;
   uint16_t color=0;
   if(sx>=0&&sx<128&&sy>=0&&sy<64&&(mono[(sy/8)*128+sx]&(1<<(sy%8))))color=0xffff;
   if(hud&&hxPixel>=0&&hxPixel<hudWidth&&hyPixel>=0&&hyPixel<15)color=hud[hyPixel*240+hxPixel];
   buffer[y*240+x]=color;
  }
 }
}
struct Offset {int x=0,y=0;};
inline Offset jitter(uint32_t now,int mood,unsigned eye){
 if(mood>=0)return {};
 uint32_t n=now/(eye?41:32)+eye*0x9e3779b9U;n^=n>>16;n*=0x7feb352dU;n^=n>>15;n*=0x846ca68bU;n^=n>>16;
 float strength=std::fmin(1.f,-mood/100.f);
 return {int(std::round((int(n%7)-3)*strength)),int(std::round((int((n>>8)%5)-2)*strength))};
}
}
