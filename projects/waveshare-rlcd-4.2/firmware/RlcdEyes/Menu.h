// SPDX-License-Identifier: MIT
#pragma once
#include "Input.h"
namespace rlcd_eyes {
enum class Page {Play,Main,Apps,Personality,Sound,Display,Controls,About,Performance};
struct MenuAction {unsigned poke=0,personality=0;};
struct Menu {
 Page page=Page::Play;unsigned focus=0;bool headerVisible=true;
 void open(Page p){page=p;focus=0;}
 unsigned count()const{
  switch(page){case Page::Main:return 7;case Page::Apps:return 3;case Page::Personality:return 4;
   case Page::Display:return 3;case Page::Performance:return 2;default:return 1;}
 }
 MenuAction input(KeyEvent event){
  MenuAction out;if(event==KeyEvent::None)return out;
  if(page==Page::Play){
   if(event==KeyEvent::Short)out.poke=3;
   else {headerVisible=true;open(Page::Main);}
  }else if(event==KeyEvent::Short)focus=(focus+1)%count();
  else switch(page){
   case Page::Main:{const Page destinations[]={Page::Play,Page::Apps,Page::Personality,Page::Sound,Page::Display,Page::Controls,Page::About};open(destinations[focus]);break;}
   case Page::Apps:if(focus<2){out.poke=focus+1;open(Page::Play);}else open(Page::Main);break;
   case Page::Personality:if(focus<3)out.personality=focus+1;open(Page::Main);break;
   case Page::Display:if(focus==0)open(Page::Performance);else if(focus==1){headerVisible=true;open(Page::Play);}else open(Page::Main);break;
   case Page::Performance:if(focus==0){headerVisible=false;open(Page::Play);}else open(Page::Display);break;
   default:open(Page::Main);break;
  }
  return out;
 }
};
}
