// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <algorithm>
namespace nesso_water {
// M5GFX 16-bit sprites store RGB565 in display byte order.
inline uint16_t pixelColor(int r,int g,int b) {
 uint16_t c=((r>>3)<<11)|((g>>2)<<5)|(b>>3);
 return uint16_t((c>>8)|(c<<8));
}
struct HeatPalette {
 uint16_t colors[16][4];
 HeatPalette() {
  const int stops[5][3]={{25,105,220},{25,200,190},{235,220,55},{255,120,25},{255,45,35}};
  for(int level=0;level<16;level++) {
   int scaled=level*4*255/15,index=std::min(3,scaled/255),t=scaled-index*255;
   int c[3];for(int k=0;k<3;k++)c[k]=(stops[index][k]*(255-t)+stops[index+1][k]*t)/255;
   colors[level][0]=pixelColor(c[0]+(255-c[0])*3/4,c[1]+(255-c[1])*3/4,c[2]+(255-c[2])*3/4);
   colors[level][1]=pixelColor(c[0],c[1],c[2]);
   colors[level][2]=pixelColor(c[0]*3/4,c[1]*3/4,c[2]*3/4);
   colors[level][3]=pixelColor(c[0]/3,c[1]/3,c[2]/3);
  }
 }
};
struct Rows {int first,last;};
template<class Pool> Rows occupiedRows(const Pool& pool,int bar,int height) {
 Rows rows{height-1,bar};
 for(int i=0;i<pool.count();i++) {
  int y=(pool.particle(i).y+128)/256+bar;
  rows.first=std::min(rows.first,std::max(bar,y-2));
  rows.last=std::max(rows.last,std::min(height-1,y+2));
 }
 return rows;
}
template<class Pool> void drawParticles(uint16_t* pixels,int width,int height,int bar,const Pool& pool) {
 for(int y=bar;y<height;y++) {
  uint16_t color=pixelColor(9+y/80,18+y/45,37+y/25);
  std::fill(pixels+y*width,pixels+(y+1)*width,color);
 }
 for(int i=0;i<pool.count();i++) {
  const auto &p=pool.particle(i);
  int x=(p.x+128)/256-2,y=(p.y+128)/256-2+bar;
  static const HeatPalette palette;
  const auto& colors=palette.colors[p.energy>>4];
  // Round five-pixel bead, pale upper highlight, energy-colored body and shaded rim.
  constexpr int mask[5][5]={{-1,0,0,1,-1},{0,0,1,2,3},{1,1,2,2,3},{1,2,2,3,3},{-1,2,3,3,-1}};
  for(int dy=0;dy<5;dy++) for(int dx=0;dx<5;dx++) {
   int xx=x+dx,yy=y+dy,shade=mask[dy][dx];
   if(shade>=0 && xx>=0 && xx<width && yy>=bar && yy<height) pixels[yy*width+xx]=colors[shade];
  }
 }
}
}
