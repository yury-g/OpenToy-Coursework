// SPDX-License-Identifier: MIT
#pragma once
#include "Presentation.h"
namespace crowpanel {
enum class Page { Play,Root,Apps,Personality,Display,PerformanceConfirm,Controls,About };
enum class Command { None,Open,Return,Apps,Personality,Display,Controls,About,Back,Curious,Dreamy,Cheeky,Eco,Performance,EnterPerformance,Restore };
struct Rect {int x,y,w,h;bool contains(int px,int py)const{return px>=x&&px<x+w&&py>=y&&py<y+h;}};
struct Item {Command command;const char* label;Rect bounds;};
struct MenuModel {
 Page page=Page::Play;bool performance=false,eco=true;unsigned persona=1;
 static Rect menuBox(){return {620,16,168,56};}
 static Rect restoreBox(){return {352,0,320,64};}
 Presentation presentation()const{Presentation p;p.header=!performance;p.eco=eco;return p;}
 unsigned items(Item* out)const{
  unsigned n=0;
  auto add=[&](Command command,const char* label){
   unsigned k=n++;out[k]={command,label,{32+int(k%2)*488,164+int(k/2)*88,472,72}};
  };
  switch(page){
   case Page::Play:if(!performance){out[0]={Command::Open,"Menu",menuBox()};n=1;}break;
   case Page::Root:add(Command::Return,"Return to play");add(Command::Apps,"Apps");add(Command::Personality,"Personality");add(Command::Display,"Display");add(Command::Controls,"Controls");add(Command::About,"About");break;
   case Page::Apps:add(Command::Return,"Eyes 1 (current)");add(Command::Back,"Back");break;
   case Page::Personality:add(Command::Curious,persona==1?"Curious (selected)":"Curious");add(Command::Dreamy,persona==2?"Dreamy (selected)":"Dreamy");add(Command::Cheeky,persona==3?"Cheeky (selected)":"Cheeky");add(Command::Back,"Back");break;
   case Page::Display:add(Command::Performance,"Performance");add(Command::Eco,eco?"ECO: ON":"ECO: OFF");add(Command::Back,"Back");break;
   case Page::PerformanceConfirm:add(Command::EnterPerformance,"Enter performance");add(Command::Back,"Back");break;
   case Page::Controls:case Page::About:add(Command::Back,"Back");break;
  }
  return n;
 }
 Command hit(int x,int y)const{Item choices[6];unsigned n=items(choices);for(unsigned k=0;k<n;k++)if(choices[k].bounds.contains(x,y))return choices[k].command;return Command::None;}
 const char* title()const{
  switch(page){case Page::Root:return "Menu";case Page::Apps:return "Apps";case Page::Personality:return "Personality";case Page::Display:return "Display";case Page::PerformanceConfirm:return "Performance";case Page::Controls:return "Controls";case Page::About:return "About";default:return "";}
 }
 bool apply(Command c){
  if(c==Command::Restore){if(!performance||page!=Page::Play)return false;performance=false;page=Page::Root;return true;}
  Item choices[6];unsigned n=items(choices);bool allowed=false;
  for(unsigned k=0;k<n;k++)if(choices[k].command==c)allowed=true;
  if(!allowed)return false;
  switch(c){
   case Command::Open:page=Page::Root;break;
   case Command::Return:page=Page::Play;break;
   case Command::Apps:page=Page::Apps;break;
   case Command::Personality:page=Page::Personality;break;
   case Command::Display:page=Page::Display;break;
   case Command::Controls:page=Page::Controls;break;
   case Command::About:page=Page::About;break;
   case Command::Back:page=page==Page::PerformanceConfirm?Page::Display:Page::Root;break;
   case Command::Curious:persona=1;break;
   case Command::Dreamy:persona=2;break;
   case Command::Cheeky:persona=3;break;
   case Command::Eco:eco=!eco;break;
   case Command::Performance:page=Page::PerformanceConfirm;break;
   case Command::EnterPerformance:performance=true;page=Page::Play;break;
   default:return false;
  }
  return true;
 }
};
class MenuInput {
 enum class Mode { Idle,Face,Tap,Hold,Cancelled };
 Mode mode_=Mode::Idle;bool blocked_=false;uint32_t last_=0,start_=0;
 unsigned mask_=0;Command target_=Command::None;Rect bounds_{0,0,0,0};
public:
 void cancel(bool waitForRelease=true){mode_=Mode::Idle;blocked_=waitForRelease;mask_=0;target_=Command::None;}
 void expire(uint32_t now){if(mode_!=Mode::Idle&&uint32_t(now-last_)>=100)cancel();}
 Command sample(bool ok,const Contact* points,unsigned count,uint32_t now,const MenuModel& shown){
  if(!ok||count>5||(count&&!points)){cancel();return Command::None;}
  expire(now);last_=now;
  if(!count){Command result=!blocked_&&mode_==Mode::Tap?target_:Command::None;cancel(false);return result;}
  if(blocked_)return Command::None;
  if(mode_==Mode::Idle){
   Command hit=shown.hit(points[0].x,points[0].y);
   if(count==1&&shown.performance&&shown.page==Page::Play&&MenuModel::restoreBox().contains(points[0].x,points[0].y)){
    mode_=Mode::Hold;start_=now;bounds_=MenuModel::restoreBox();
   }else if(count==1&&hit!=Command::None){
    mode_=Mode::Tap;target_=hit;Item items[6];unsigned n=shown.items(items);
    for(unsigned k=0;k<n;k++)if(items[k].command==hit)bounds_=items[k].bounds;
   }else mode_=shown.page==Page::Play?Mode::Face:Mode::Cancelled;
  }
  if(mode_==Mode::Face){
   mask_=0;auto p=shown.presentation();for(unsigned k=0;k<count;k++)mask_|=p.eyeAt(points[k].x,points[k].y);
  }else if(mode_==Mode::Tap||mode_==Mode::Hold){
   if(count!=1||!bounds_.contains(points[0].x,points[0].y)){mode_=Mode::Cancelled;target_=Command::None;}
   else if(mode_==Mode::Hold&&uint32_t(now-start_)>=800){cancel();return Command::Restore;}
  }
  return Command::None;
 }
 unsigned mask(uint32_t now)const{return !blocked_&&mode_==Mode::Face&&uint32_t(now-last_)<100?mask_:0;}
 Command pressed()const{return !blocked_&&mode_==Mode::Tap?target_:Command::None;}
};
}
