// SPDX-License-Identifier: MIT
#pragma once
#include "Round.h"
namespace waveshare_eyes {
enum class Page {Play,Root,Apps,Personality,Sound,Display,Controls,About,Performance};
struct Rect {
 int x,y,w,h;
 bool contains(int px,int py)const{return px>=x&&px<x+w&&py>=y&&py<y+h;}
};
constexpr Rect menuButton{136,88,88,36},backButton{82,290,80,36},nextButton{198,290,80,36},restoreZone{140,10,81,31};
inline Rect itemRect(int i){return {64,138+52*i,232,44};}
class Menu {
 bool down_=false,cancelled_=false,blocked_=false;
 int capture_=-1;uint32_t pressedAt_=0;
 void changed(){++epoch;poke=0;blocked_=blocked_||down_;down_=false;capture_=-1;}
 void go(Page p){page=p;helpPage=0;changed();}
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
    go(pages[rootPage*3+id]);break;
   }
   case Page::Personality:setPersona(id+1);break;
   case Page::Sound:if(id==0)toggleMute();else {volume=id;changed();}break;
   case Page::Display:if(id==0)go(Page::Performance);else {brightness=id-1;changed();}break;
   case Page::Performance:if(id==2)enterPerformance();break;
   default:break;
  }
 }
public:
 Page page=Page::Play;bool header=true,muted=false,audioReady=true,lightReady=true;
 unsigned persona=1,volume=2,brightness=1,rootPage=0,helpPage=0,poke=0;uint32_t epoch=0;
 bool playing()const{return page==Page::Play;}
 bool hasNext()const{return page==Page::Root||page==Page::Controls;}
 int itemCount()const{
  switch(page){
   case Page::Root:return rootPage==2?1:3;
   case Page::Personality:return 3;
   case Page::Sound:return audioReady?3:0;
   case Page::Display:return lightReady?3:1;
   case Page::Performance:return 3;
   default:return 0;
  }
 }
 const char* itemLabel(int i)const{
  if(i<0||i>=itemCount())return nullptr;
  switch(page){
   case Page::Root:{static const char* const labels[]={"Return to play","Apps","Personality","Sound","Display","Controls","About"};return labels[(rootPage%3)*3+i];}
   case Page::Personality:{static const char* const labels[]={"Curious","Dreamy","Cheeky"};return labels[i];}
   case Page::Sound:return i==0?(muted?"Unmute":"Mute"):i==1?"Volume Soft":"Volume Quiet";
   case Page::Display:return i==0?"Performance":i==1?"Brightness Dim":"Brightness Normal";
   case Page::Performance:return i==2?"Enter performance":nullptr;
   default:return nullptr;
  }
 }
 bool selected(int i)const{
  return (page==Page::Root&&rootPage==0&&i==0)||(page==Page::Personality&&persona==unsigned(i+1))||
   (page==Page::Sound&&((i==0&&muted)||(i>0&&volume==unsigned(i))))||(page==Page::Display&&i>0&&brightness==unsigned(i-1));
 }
 bool pressed(int id)const{return down_&&!cancelled_&&!blocked_&&capture_==id;}
 void openMenu(){header=true;rootPage=0;go(Page::Root);}
 void returnToPlay(){go(Page::Play);}
 void enterPerformance(){header=false;go(Page::Play);}
 void togglePerformance(){if(header)enterPerformance();else openMenu();}
 void setPersona(unsigned id){if(id>=1&&id<=3){persona=id;changed();}}
 void toggleMute(){muted=!muted;changed();}
 void back(){if(page==Page::Root)go(Page::Play);else if(page==Page::Performance)go(Page::Display);else if(page!=Page::Play)go(Page::Root);}
 void next(){if(page==Page::Root)rootPage=(rootPage+1)%3;else if(page==Page::Controls)helpPage=(helpPage+1)%2;else return;changed();}
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
