// SPDX-License-Identifier: MIT
#pragma once
#include "Menu.h"
#include "Status.h"
namespace waveshare_eyes {
inline void buttonRow(uint16_t* row,int y,Rect r,const char* label,bool pressed,bool selected=false){
 if(y<r.y||y>=r.y+r.h)return;
 const uint16_t bg=pressed?0x07ff:0x1082,edge=pressed||selected?0xffff:0x7bef,ink=pressed?0x0000:0xffff;
 const int border=pressed?3:2;
 for(int x=r.x;x<r.x+r.w;x++)row[x]=(x<r.x+border||x>=r.x+r.w-border||y<r.y+border||y>=r.y+r.h-border)?edge:bg;
 const int barTop=r.y+(r.h-12)/2;
 if(selected&&y>=barTop&&y<barTop+12)for(int x=r.x+10;x<r.x+15;x++)row[x]=ink;
 textRow(row,y,r.x+(r.w-int(std::strlen(label))*12)/2,r.y+(r.h-14)/2,label,ink);
}
inline void menuLine(uint16_t* row,int y,int top,const char* text){textRow(row,y,180-int(std::strlen(text))*6,top,text,0xbdf7);}
inline void renderMenuRow(const Menu& m,int y,uint16_t* row,int battery){
 if(m.page==Page::Play){if(m.header)buttonRow(row,y,menuButton,"Menu",m.pressed(10));return;}
 char title[28];
 switch(m.page){
  case Page::Root:std::snprintf(title,sizeof title,"Menu %u of 4",(m.rootPage%4)+1);break;
  case Page::Apps:std::snprintf(title,sizeof title,"Apps");break;
  case Page::Personality:std::snprintf(title,sizeof title,"Personality %u of 2",(m.subPage%2)+1);break;
  case Page::Sound:if(m.audioReady)std::snprintf(title,sizeof title,"Sound %u of 2",(m.subPage%2)+1);else std::snprintf(title,sizeof title,"Sound");break;
  case Page::Display:if(m.lightReady)std::snprintf(title,sizeof title,"Display %u of 2",(m.subPage%2)+1);else std::snprintf(title,sizeof title,"Display");break;
  case Page::Controls:std::snprintf(title,sizeof title,"Controls %u of 2",(m.helpPage%2)+1);break;
  case Page::About:std::snprintf(title,sizeof title,"About");break;
  case Page::Performance:std::snprintf(title,sizeof title,"Performance");break;
  default:title[0]=0;
 }
 menuLine(row,y,100,title);
 for(int i=0;i<m.itemCount();i++)if(auto label=m.itemLabel(i))buttonRow(row,y,itemRect(i),label,m.pressed(i),m.selected(i));
 if(m.page==Page::Apps){menuLine(row,y,136,"EYES1 - current app");menuLine(row,y,180,"More apps require");menuLine(row,y,204,"installation");}
 if(m.page==Page::Sound&&!m.audioReady){menuLine(row,y,148,"Speaker unavailable");menuLine(row,y,184,"Play still works");}
 if(m.page==Page::Display&&!m.lightReady)menuLine(row,y,206,"Backlight unavailable");
 if(m.page==Page::Controls){
  if(m.helpPage==0){menuLine(row,y,128,"Poke either eye");menuLine(row,y,154,"Tap Menu for settings");menuLine(row,y,186,"Fixed upright screen");menuLine(row,y,218,"BOOT RESET reserved");}
  else {menuLine(row,y,128,"Performance restore");menuLine(row,y,156,"Hold top-center edge");menuLine(row,y,182,"for 0.8s");menuLine(row,y,214,"Release before play");}
 }
 if(m.page==Page::About){
  char id[40],volts[8],bat[24];std::snprintf(id,sizeof id,"%s v%s",appName,version);batteryText(battery,volts);std::snprintf(bat,sizeof bat,"Battery %s",volts);
  menuLine(row,y,126,id);menuLine(row,y,148,created);menuLine(row,y,174,"Waveshare 1.85C");menuLine(row,y,196,"BOX V2");menuLine(row,y,222,bat);
 }
 if(m.page==Page::Performance){menuLine(row,y,126,"Hold top-center edge");menuLine(row,y,148,"for 0.8s to restore");menuLine(row,y,170,"Menu and information");}
 buttonRow(row,y,backButton,"Back",m.pressed(12));
 if(m.hasNext())buttonRow(row,y,nextButton,"Next",m.pressed(13));
}
struct FrameSnapshot {
 Menu menu;
 int battery=-1;
 unsigned persona=1;
 uint16_t color=0xffff;
};
inline void renderFrameRow(const FrameSnapshot& frame,const uint8_t* mono,int y,uint16_t* row){
 if(frame.menu.playing())renderRow(mono,y,row,frame.color);else std::fill(row,row+360,0);
 renderStatusRow(y,row,frame.menu.header,frame.battery,frame.persona,frame.menu.muted);
 renderMenuRow(frame.menu,y,row,frame.battery);
}
template<class EmitStrip,class ServiceStrip>
inline bool renderFrameStrips(const FrameSnapshot& frame,const uint8_t* mono,int stripRows,uint16_t* buffer,bool bigEndian,EmitStrip&& emit,ServiceStrip&& service){
 for(int y=0;y<360;y+=stripRows){
  for(int r=0;r<stripRows;r++){
   auto* row=buffer+r*360;
   renderFrameRow(frame,mono,y+r,row);
   if(bigEndian)for(int x=0;x<360;x++)row[x]=(row[x]>>8)|(row[x]<<8);
  }
  if(!emit(y,buffer))return false;
  service(y);
 }
 return true;
}
}
