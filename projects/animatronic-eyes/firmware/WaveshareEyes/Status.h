// SPDX-License-Identifier: MIT
#pragma once
#include "Metadata.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
namespace waveshare_eyes {
// Original compact 5x7 glyphs; doubled for a readable 14-pixel cap height.
inline const uint8_t* glyph(char c){
 static const char chars[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ.-@";
 static const uint8_t rows[][7]={
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14},
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},{7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
 {0,0,0,0,0,6,6},{0,0,0,31,0,0,0},{14,31,17,17,17,17,31}};
 static const uint8_t blank[7]={};if(c>='a'&&c<='z')c-=32;
 auto p=std::strchr(chars,c);return c&&p?rows[p-chars]:blank;
}
inline void textRow(uint16_t* row,int y,int x,int top,const char* text,uint16_t color,int scale=2){
 if(y<top||y>=top+7*scale)return;
 for(unsigned i=0;text[i];i++){
  uint8_t bits=glyph(text[i])[(y-top)/scale];
  for(int col=0;col<5*scale;col++){int px=x+int(i)*6*scale+col;if(px>=0&&px<360&&(bits&(16>>(col/scale))))row[px]=color;}
 }
}
inline void batteryText(int mv,char* out){if(mv<2500||mv>4500)std::snprintf(out,8,"--");else std::snprintf(out,8,"%d.%02dV",mv/1000,(mv%1000)/10);}
// Rasterize each glyph along a circular baseline, with its local axes rotated
// to the tangent. Cache only the upper 100 rows: no trigonometry per frame.
inline void arcText(uint8_t* canvas,const char* text,float radius,uint8_t ink){
 const int count=std::strlen(text);
 for(int i=0;i<count;i++){
  const float angle=(i-(count-1)*.5f)*12.f/radius;
  const float sn=std::sin(angle),cs=std::cos(angle);
  const float cx=180+radius*sn,cy=180-radius*cs;
  for(int y=int(cy)-10;y<=int(cy)+10;y++)for(int x=int(cx)-10;x<=int(cx)+10;x++){
   const float dx=x+.5f-cx,dy=y+.5f-cy;
   const float u=dx*cs+dy*sn+5,v=-dx*sn+dy*cs+7;
   if(u>=0&&u<10&&v>=0&&v<14&&x>=0&&x<360&&y>=0&&y<100&&
      (glyph(text[i])[int(v)/2]&(16>>(int(u)/2))))canvas[y*360+x]=ink;
  }
 }
}
inline void renderStatusRow(int y,uint16_t* row,bool visible,int mv,unsigned,bool){
 if(!visible||y<0||y>=100)return;
 static uint8_t canvas[360*100]={};static int lastBattery=-99999;
 if(lastBattery!=mv){
  std::fill(canvas,canvas+sizeof canvas,0);lastBattery=mv;
  char identity[40],volts[8],detail[40];
  std::snprintf(identity,sizeof identity,"%s V%s",appName,version);batteryText(mv,volts);
  std::snprintf(detail,sizeof detail,"%s %s @",created,volts);
  arcText(canvas,identity,160,1);arcText(canvas,detail,138,2);
 }
 for(int x=0;x<360;x++)if(canvas[y*360+x])row[x]=canvas[y*360+x]==1?0x07ff:0xbdf7;
}
}
