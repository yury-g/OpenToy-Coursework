// SPDX-License-Identifier: MIT
#pragma once
#include "Menu.h"
#include "Status.h"
namespace waveshare_eyes {
inline void buttonRow(uint16_t* row,int y,Rect r,const char* label,bool pressed,bool selected=false){
 if(y<r.y||y>=r.y+r.h)return;
 const uint16_t bg=pressed?0x528a:0x1082,edge=selected?0xffff:0x7bef;
 for(int x=r.x;x<r.x+r.w;x++)row[x]=(x<r.x+2||x>=r.x+r.w-2||y<r.y+2||y>=r.y+r.h-2)?edge:bg;
 if(selected&&y>=r.y+17&&y<r.y+27)for(int x=r.x+7;x<r.x+11;x++)row[x]=0xffff;
 textRow(row,y,r.x+(r.w-int(std::strlen(label))*12)/2,r.y+(r.h-14)/2,label,0xffff);
}
inline void menuLine(uint16_t* row,int y,int top,const char* text){textRow(row,y,180-int(std::strlen(text))*6,top,text,0xbdf7);}
inline void renderMenuRow(const Menu& m,int y,uint16_t* row,int battery){
 if(m.page==Page::Play){if(m.header)buttonRow(row,y,menuButton,"Menu",m.pressed(10));return;}
 char title[28];
 switch(m.page){
  case Page::Root:std::snprintf(title,sizeof title,"Menu %u of 3",m.rootPage+1);break;
  case Page::Apps:std::snprintf(title,sizeof title,"Apps");break;
  case Page::Personality:std::snprintf(title,sizeof title,"Personality");break;
  case Page::Sound:std::snprintf(title,sizeof title,"Sound");break;
  case Page::Display:std::snprintf(title,sizeof title,"Display");break;
  case Page::Controls:std::snprintf(title,sizeof title,"Controls %u of 2",m.helpPage+1);break;
  case Page::About:std::snprintf(title,sizeof title,"About");break;
  case Page::Performance:std::snprintf(title,sizeof title,"Performance");break;
  default:title[0]=0;
 }
 menuLine(row,y,108,title);
 for(int i=0;i<m.itemCount();i++)if(auto label=m.itemLabel(i))buttonRow(row,y,itemRect(i),label,m.pressed(i),m.selected(i));
 if(m.page==Page::Apps){menuLine(row,y,148,"EYES1 - current app");menuLine(row,y,196,"More apps require");menuLine(row,y,218,"installation");}
 if(m.page==Page::Sound&&!m.audioReady){menuLine(row,y,156,"Speaker unavailable");menuLine(row,y,190,"Play still works");}
 if(m.page==Page::Display&&!m.lightReady)menuLine(row,y,212,"Backlight unavailable");
 if(m.page==Page::Controls){
  if(m.helpPage==0){menuLine(row,y,145,"Poke either eye");menuLine(row,y,171,"Tap Menu for settings");menuLine(row,y,208,"Fixed upright screen");menuLine(row,y,242,"BOOT RESET reserved");}
  else {menuLine(row,y,145,"Performance restore");menuLine(row,y,175,"Hold top-center edge");menuLine(row,y,201,"for 0.8s");menuLine(row,y,235,"Release before play");}
 }
 if(m.page==Page::About){
  char id[40],volts[8],bat[24];std::snprintf(id,sizeof id,"%s v%s",appName,version);batteryText(battery,volts);std::snprintf(bat,sizeof bat,"Battery %s",volts);
  menuLine(row,y,140,id);menuLine(row,y,163,created);menuLine(row,y,191,"Waveshare 1.85C");menuLine(row,y,214,"BOX V2");menuLine(row,y,247,bat);
 }
 if(m.page==Page::Performance){menuLine(row,y,143,"Hold top-center edge");menuLine(row,y,170,"for 0.8s to restore");menuLine(row,y,197,"Menu and information");}
 buttonRow(row,y,backButton,"Back",m.pressed(12));
 if(m.hasNext())buttonRow(row,y,nextButton,"Next",m.pressed(13));
}
}
