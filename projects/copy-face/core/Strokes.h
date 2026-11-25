// SPDX-License-Identifier: MIT
#pragma once
#include "Geometry.h"
#include <cstdint>
namespace copy_face {
inline void stroke(uint16_t* pixels,int width,int height,Point a,Point b,float radius,uint16_t color) {
 int x0=std::max(0,int(std::floor(std::min(a.x,b.x)-radius-1)));
 int y0=std::max(0,int(std::floor(std::min(a.y,b.y)-radius-1)));
 int x1=std::min(width-1,int(std::ceil(std::max(a.x,b.x)+radius+1)));
 int y1=std::min(height-1,int(std::ceil(std::max(a.y,b.y)+radius+1)));
 float dx=b.x-a.x,dy=b.y-a.y,denom=dx*dx+dy*dy;
 for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++) {
  float t=denom?std::max(0.f,std::min(1.f,((x+.5f-a.x)*dx+(y+.5f-a.y)*dy)/denom)):0;
  float alpha=std::max(0.f,std::min(1.f,radius+.5f-std::hypot(x+.5f-a.x-t*dx,y+.5f-a.y-t*dy)));
  if(alpha<=0)continue;
  uint16_t old=pixels[y*width+x];
  auto mix=[&](unsigned mask,unsigned shift){return unsigned(((old>>shift)&mask)*(1-alpha)+((color>>shift)&mask)*alpha+.5f);};
  pixels[y*width+x]=uint16_t((mix(31,11)<<11)|(mix(63,5)<<5)|mix(31,0));
 }
}
}
