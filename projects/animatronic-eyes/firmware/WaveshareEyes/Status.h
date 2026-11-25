// SPDX-License-Identifier: MIT
#pragma once
#include "Metadata.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
namespace waveshare_eyes {
// Original compact 5x7 glyphs; doubled for a readable 14-pixel cap height.
inline const uint8_t* glyph(char c){
 static const char chars[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ.-";
 static const uint8_t rows[][7]={
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14},
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},{7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
 {0,0,0,0,0,6,6},{0,0,0,31,0,0,0}};
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
inline void renderStatusRow(int y,uint16_t* row,bool visible,int mv,unsigned persona,bool muted){
 if(!visible)return;
 char identity[40],volts[8];std::snprintf(identity,sizeof identity,"%s v%s",appName,version);batteryText(mv,volts);
 textRow(row,y,64,50,identity,0x07ff);textRow(row,y,64,70,created,0xbdf7);
 textRow(row,y,280-int(std::strlen(volts))*12,50,volts,0xbdf7);
 if(y>=52&&y<=61)for(int x=286;x<299;x++)if(x==286||x==296||y==52||y==61||(x>296&&y>=55&&y<=58))row[x]=0xbdf7;
 const char* names[]={"","CURIOUS","DREAMY","CHEEKY"};
 textRow(row,y,64,90,names[persona<=3?persona:0],0x7bef,1);
 textRow(row,y,250,90,muted?"MUTE":"SOUND",0x7bef,1);
}
}
