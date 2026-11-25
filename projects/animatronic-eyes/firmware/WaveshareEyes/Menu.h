// SPDX-License-Identifier: MIT
#pragma once
#include "Round.h"
namespace waveshare_eyes {
enum class Page {Play,Root,Apps,Personality,Sound,Display,Controls,About,Performance};
struct Rect {
 int x,y,w,h;
 bool contains(int px,int py)const{return px>=x&&px<x+w&&py>=y&&py<y+h;}
};
constexpr Rect menuButton{110,75,140,52},backButton{66,256,108,56},nextButton{186,256,108,56},restoreZone{130,12,100,56};
inline Rect itemRect(int i){return {56,124+66*i,248,56};}
class Menu {
 bool down_=false,cancelled_=false,blocked_=false;
 int capture_=-1;uint32_t pressedAt_=0;
 void changed(){++epoch;poke=0;blocked_=blocked_||down_;down_=false;capture_=-1;}
 void go(Page p){page=p;subPage=0;helpPage=0;changed();}
 int hit(Touch t)const{
  if(!t.down)return -1;
  if(page==Page::Play){
   if(header&&menuButton.contains(t.x,t.y))return 10;
   if(!header&&restoreZone.contains(t.x,t.y))return 11;
   return -1;
  }
  if(backButton.contains(t.x,t.y))return 12;
  if(hasNext()&&nextButton.contains(t.x,t.y))return 13;
  for(int i=0;i<itemCount();i++)if(itemLabel(i)&&itemRect(i).contains(t.x,t.y))return i;
  return -1;
 }
 void activate(int id){
  if(id==10){openMenu();return;}
  if(id==12){back();return;}
  if(id==13){next();return;}
  if(id<0||id>=itemCount()||!itemLabel(id))return;
  switch(page){
   case Page::Root:{
    const Page pages[]={Page::Play,Page::Apps,Page::Personality,Page::Sound,Page::Display,Page::Controls,Page::About};
    go(pages[(rootPage%4)*2+id]);break;
   }
   case Page::Personality:setPersona((subPage%2)*2+id+1);break;
   case Page::Sound:{int idx=(subPage%2)*2+id;if(idx==0)toggleMute();else {volume=idx;changed();}break;}
   case Page::Display:{int idx=(subPage%2)*2+id;if(idx==0)go(Page::Performance);else {brightness=idx-1;changed();}break;}
   case Page::Performance:if(id==1)enterPerformance();break;
   default:break;
  }
 }
public:
 Page page=Page::Play;bool header=true,muted=false,audioReady=true,lightReady=true;
 unsigned persona=1,volume=2,brightness=1,rootPage=0,subPage=0,helpPage=0,poke=0;uint32_t epoch=0;
 bool playing()const{return page==Page::Play;}
 bool hasNext()const{return page==Page::Root||page==Page::Personality||(page==Page::Sound&&audioReady)||(page==Page::Display&&lightReady)||page==Page::Controls;}
 int itemCount()const{
  switch(page){
   case Page::Root:return rootPage==3?1:2;
   case Page::Personality:return subPage==1?1:2;
   case Page::Sound:return !audioReady?0:(subPage==1?1:2);
   case Page::Display:return !lightReady?1:(subPage==1?1:2);
   case Page::Performance:return 2;
   default:return 0;
  }
 }
 const char* itemLabel(int i)const{
  if(i<0||i>=itemCount())return nullptr;
  switch(page){
   case Page::Root:{static const char* const labels[]={"Return to play","Apps","Personality","Sound","Display","Controls","About"};return labels[(rootPage%4)*2+i];}
   case Page::Personality:{static const char* const labels[]={"Curious","Dreamy","Cheeky"};return labels[(subPage%2)*2+i];}
   case Page::Sound:{int idx=(subPage%2)*2+i;return idx==0?(muted?"Unmute":"Mute"):idx==1?"Volume Soft":"Volume Quiet";}
   case Page::Display:{int idx=(subPage%2)*2+i;return idx==0?"Performance":idx==1?"Brightness Dim":"Brightness Normal";}
   case Page::Performance:return i==1?"Enter performance":nullptr;
   default:return nullptr;
  }
 }
 bool selected(int i)const{
  if(i<0||i>=itemCount())return false;
  switch(page){
   case Page::Root:return rootPage==0&&i==0;
   case Page::Personality:return persona==unsigned((subPage%2)*2+i+1);
   case Page::Sound:{int idx=(subPage%2)*2+i;return (idx==0&&muted)||(idx>0&&volume==unsigned(idx));}
   case Page::Display:{int idx=(subPage%2)*2+i;return idx>0&&brightness==unsigned(idx-1);}
   default:return false;
  }
 }
 int pressedId()const{return down_&&!cancelled_&&!blocked_?capture_:-1;}
 bool pressed(int id)const{return pressedId()==id;}
 void openMenu(){header=true;rootPage=0;go(Page::Root);}
 void returnToPlay(){go(Page::Play);}
 void enterPerformance(){header=false;go(Page::Play);}
 void togglePerformance(){if(header)enterPerformance();else openMenu();}
 void setPersona(unsigned id){if(id>=1&&id<=3){persona=id;changed();}}
 void toggleMute(){muted=!muted;changed();}
 void back(){if(page==Page::Root)go(Page::Play);else if(page==Page::Performance)go(Page::Display);else if(page!=Page::Play)go(Page::Root);}
 void next(){
  if(page==Page::Root)rootPage=(rootPage+1)%4;
  else if(page==Page::Personality)subPage=(subPage+1)%2;
  else if(page==Page::Sound&&audioReady)subPage=(subPage+1)%2;
  else if(page==Page::Display&&lightReady)subPage=(subPage+1)%2;
  else if(page==Page::Controls)helpPage=(helpPage+1)%2;
  else return;
  changed();
 }
 // Failure/timeout is cancellation, never the release that commits a tap.
 void loseInput(){poke=0;blocked_=true;down_=false;capture_=-1;cancelled_=true;}
 void update(Touch t,uint32_t now,bool valid=true){
  poke=0;
  if(!valid){loseInput();return;}
  if(blocked_){if(!t.down){blocked_=false;down_=false;}return;}
  if(!t.down){
   int chosen=down_&&!cancelled_&&hit({true,t.x,t.y})==capture_?capture_:-1;
   down_=false;capture_=-1;cancelled_=false;
   activate(chosen);return;
  }
  if(!down_){down_=true;cancelled_=false;capture_=hit(t);pressedAt_=now;}
  if(hit(t)!=capture_)cancelled_=true;
  if(!cancelled_&&capture_==11&&uint32_t(now-pressedAt_)>=800){openMenu();return;}
  if(page==Page::Play&&capture_==-1)poke=eyeAt(t.x,t.y);
 }
};
}
