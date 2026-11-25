#include "../firmware/WaveshareEyes/MenuView.h"
#include <cassert>
#include <cstdio>
using namespace waveshare_eyes;
static uint32_t now=1000;
void tap(Menu& m,int x,int y){m.update({true,x,y},now+=20);m.update({false,x,y},now+=20);}
void row(Menu& m,int i){tap(m,180,160+i*52);}
void next(Menu& m){tap(m,238,308);}
void back(Menu& m){tap(m,122,308);}
int main(){
 Menu m;assert(m.page==Page::Play&&m.header&&!m.muted&&m.persona==1);
 // A control doesn't activate on press; crossing into Menu from an eye isn't a tap.
 m.update({true,180,105},now+=20);assert(m.page==Page::Play);m.update({false,180,105},now+=20);assert(m.page==Page::Root);
 row(m,0);assert(m.page==Page::Play);
 m.update({true,82,180},now+=20);assert(m.poke==1);
 m.update({true,180,105},now+=20);m.update({},now+=20);assert(m.page==Page::Play&&m.poke==0);
 // Release outside cancels even when no intermediate drag sample was seen.
 m.update({true,180,105},now+=20);m.update({false,40,120},now+=20);assert(m.page==Page::Play);
 m.update({true,180,105},now+=20);m.update({},now+=20);assert(m.page==Page::Play);
 // Leaving and reentering cancels; failed reads must not act like a successful release.
 m.update({true,180,105},now+=20);m.update({true,40,120},now+=20);m.update({true,180,105},now+=20);m.update({},now+=20);assert(m.page==Page::Play);
 m.update({true,180,105},now+=20);m.update({},now+=20,false);assert(m.page==Page::Play);
 m.update({true,82,180},now+=20);assert(!m.poke);m.update({},now+=20);
 tap(m,180,105);row(m,1);assert(m.page==Page::Apps);assert(m.itemCount()==0);back(m);assert(m.page==Page::Root);
 row(m,2);assert(m.page==Page::Personality);row(m,2);assert(m.persona==3);back(m);
 next(m);row(m,0);assert(m.page==Page::Sound);row(m,0);assert(m.muted);row(m,1);assert(m.volume==1);row(m,2);assert(m.volume==2);back(m);
 row(m,1);assert(m.page==Page::Display);row(m,1);assert(m.brightness==0);row(m,2);assert(m.brightness==1);row(m,0);assert(m.page==Page::Performance);
 row(m,2);assert(m.page==Page::Play&&!m.header);
 // Restore once, then require release; sliding from an eye never qualifies.
 m.update({true,82,180},now+=20);m.update({true,180,20},now+=20);m.update({true,180,20},now+=800);assert(!m.header);m.update({},now+=20);
 m.update({true,180,20},now+=20);auto start=now;m.update({true,180,20},now+=799);assert(!m.header);m.update({true,180,20},now=start+800);assert(m.header&&m.page==Page::Root);
 m.update({true,180,160},now+=20);assert(!m.poke&&m.page==Page::Root);m.update({},now+=20);
 next(m);row(m,2);assert(m.page==Page::Controls);next(m);back(m);assert(m.page==Page::Root);next(m);row(m,0);assert(m.page==Page::About);back(m);back(m);assert(m.page==Page::Play);
 // External transitions consume held eye contacts, and explicit loss cancels holds.
 m.update({true,82,180},now+=20);m.openMenu();m.returnToPlay();m.update({true,82,180},now+=20);assert(!m.poke);m.update({},now+=20);m.update({true,82,180},now+=20);assert(m.poke==1);
 m.loseInput();m.update({true,82,180},now+=20);assert(!m.poke);m.update({},now+=20);
 m.enterPerformance();m.update({true,180,20},0xfffffff0);m.update({true,180,20},784);assert(m.header&&m.page==Page::Root);m.update({},804);
 m.audioReady=false;m.page=Page::Sound;assert(m.itemCount()==0);back(m);assert(m.page==Page::Root);
 // Curved header and centered Menu never overlap, with real/missing voltage.
 for(int mv:{-1,2500,4200,4500})for(int y=0;y<128;y++){
  uint16_t status[360]={},control[360]={};Menu play;
  renderStatusRow(y,status,true,mv,1,false);renderMenuRow(play,y,control,mv);
  for(int x=0;x<360;x++)assert(!(status[x]&&control[x]));
 }
 // All actual controls and pixels fit round safe bounds, including missing battery.
 for(Page page:{Page::Play,Page::Root,Page::Apps,Page::Personality,Page::Sound,Page::Display,Page::Controls,Page::About,Page::Performance}){
  m.page=page;m.header=true;m.audioReady=true;
  for(unsigned pg=0;pg<3;pg++){
   m.rootPage=pg;
   for(int y=0;y<360;y++){
    uint16_t guarded[362]={};guarded[0]=123;guarded[361]=456;renderMenuRow(m,y,guarded+1,-1);
    assert(guarded[0]==123&&guarded[361]==456);
    for(int x=0;x<360;x++)if(guarded[x+1])assert((x-180)*(x-180)+(y-180)*(y-180)<176*176);
   }
  }
 }
 puts("PASS round menu navigation, release/drag/loss semantics, restore hold/wrap, settings and safe pixels");
}
