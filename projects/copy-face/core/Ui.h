// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
namespace copy_face {
enum class Page {Closed,Main,Apps,Personality,Sound,Display,Controls,About};
enum class Action {None,Menu,Return,Apps,Personality,Sound,Display,Controls,About,Back,Performance,Developer,Mirror,Demo};
struct Ui {
 Page page=Page::Closed;bool performance=false,developer=true,mirror=false,demo=false,dirty=true;
 bool showDeveloper()const{return developer&&!performance;}
 static bool restoreZone(int x,int y){return x>=384&&x<640&&y>=0&&y<48;}
 unsigned rows()const{return page==Page::Main?7:page==Page::Display?5:1;}
 Action rowAction(unsigned row)const {
  static const Action main[]={Action::Return,Action::Apps,Action::Personality,Action::Sound,Action::Display,Action::Controls,Action::About};
  static const Action display[]={Action::Performance,Action::Developer,Action::Mirror,Action::Demo,Action::Back};
  if(row>=rows())return Action::None;
  return page==Page::Main?main[row]:page==Page::Display?display[row]:Action::Back;
 }
 const char* rowLabel(unsigned row)const {
  static const char* main[]={"Return to play","Apps","Personality","Sound","Display","Controls","About"};
  if(row>=rows())return "";
  if(page==Page::Main)return main[row];
  if(page!=Page::Display)return "Back";
  switch(row){case 0:return "Performance";case 1:return developer?"Debug preview: ON":"Debug preview: OFF";
  case 2:return mirror?"Mirror: ON":"Mirror: OFF";case 3:return demo?"Pause simulation":"Play simulation";default:return "Back";}
 }
 Action hit(int x,int y)const {
  if(performance)return Action::None;
  if(page==Page::Closed)return x>=720&&x<864&&y>=0&&y<48?Action::Menu:Action::None;
  if(x>=312&&x<712&&y>=112&&y<112+int(rows())*52)return rowAction(unsigned((y-112)/52));
  return Action::None;
 }
 void apply(Action a) {
  if(a==Action::None)return;
  switch(a) {
   case Action::Menu:performance=false;page=Page::Main;break;
   case Action::Return:page=Page::Closed;break;
   case Action::Back:page=Page::Main;break;
   case Action::Apps:page=Page::Apps;break;
   case Action::Personality:page=Page::Personality;break;
   case Action::Sound:page=Page::Sound;break;
   case Action::Display:page=Page::Display;break;
   case Action::Controls:page=Page::Controls;break;
   case Action::About:page=Page::About;break;
   case Action::Performance:performance=true;page=Page::Closed;break;
   case Action::Developer:developer=!developer;break;
   case Action::Mirror:mirror=!mirror;break;
   case Action::Demo:demo=!demo;break;
   default:break;
  }
  dirty=true;
 }
};
class TouchMenu {
 bool down_=false,cancelled_=false,restore_=false,consumed_=false,blocked_=false;
 Action action_=Action::None;uint32_t began_=0,last_=0;
public:
 void cancel(){down_=false;cancelled_=true;action_=Action::None;restore_=false;consumed_=true;blocked_=true;}
 void expire(uint32_t now){if(down_&&uint32_t(now-last_)>150)cancel();}
 void sample(Ui& ui,int x,int y,unsigned count,uint32_t now) {
  expire(now);last_=now;
  if(count>1){cancel();return;}
  if(!count) {
   if(down_&&!cancelled_&&!consumed_&&ui.hit(x,y)==action_)ui.apply(action_);
   down_=false;blocked_=false;return;
  }
  if(blocked_)return;
  if(!down_) {down_=true;cancelled_=false;consumed_=false;began_=now;action_=ui.hit(x,y);restore_=ui.performance&&Ui::restoreZone(x,y);}
  if(consumed_)return;
  if(restore_) {
   if(!Ui::restoreZone(x,y))cancelled_=true;
   if(!cancelled_&&uint32_t(now-began_)>=800){ui.apply(Action::Menu);consumed_=true;}
  }else if(ui.hit(x,y)!=action_)cancelled_=true;
 }
 Action pressed()const{return down_&&!cancelled_&&!consumed_?action_:Action::None;}
};
}
