#include <cassert>
#include <cstdio>
#include "../firmware/main/Menu.h"
using namespace crowpanel;
static Command down(MenuInput& i,const MenuModel& m,int x,int y,uint32_t t){Contact p{x,y};return i.sample(true,&p,1,t,m);}
static Command up(MenuInput& i,const MenuModel& m,uint32_t t){return i.sample(true,nullptr,0,t,m);}
static void tap(MenuInput& i,MenuModel& m,int x,int y,uint32_t t){assert(down(i,m,x,y,t)==Command::None);auto c=up(i,m,t+20);assert(c!=Command::None);assert(m.apply(c));i.cancel(false);}
int main(){
 MenuModel m;MenuInput i;
 // A menu opens on release only, never on initial press.
 assert(!m.performance&&m.page==Page::Play&&m.eco&&m.persona==1);
 assert(down(i,m,700,40,100)==Command::None);assert(i.pressed()==Command::Open);
 assert(up(i,m,120)==Command::Open);assert(m.apply(Command::Open));assert(m.page==Page::Root);
 Item items[6];assert(m.items(items)==6);assert(items[0].command==Command::Return);
 // Page navigation and explicit Back; Apps has only the current playable app.
 tap(i,m,700,190,200);assert(m.page==Page::Apps);
 assert(m.items(items)==2);assert(items[0].command==Command::Return);
 tap(i,m,700,190,250);assert(m.page==Page::Root);
 tap(i,m,200,280,300);assert(m.page==Page::Personality);
 tap(i,m,700,190,350);assert(m.persona==2&&m.page==Page::Personality);
 tap(i,m,700,280,400);assert(m.page==Page::Root);
 tap(i,m,700,280,450);assert(m.page==Page::Display);
 tap(i,m,700,190,500);assert(!m.eco);
 tap(i,m,200,190,550);assert(m.page==Page::PerformanceConfirm);
 tap(i,m,700,190,600);assert(m.page==Page::Display); // Cancel is reversible.
 tap(i,m,200,190,650);tap(i,m,200,190,700);
 assert(m.performance&&m.page==Page::Play&&m.items(items)==0);
 // No restore at799ms; a fresh held contact at800ms restores once.
 assert(down(i,m,512,32,1000)==Command::None);
 for(uint32_t t=1050;t<=1750;t+=50)assert(down(i,m,512,32,t)==Command::None);
 assert(down(i,m,512,32,1799)==Command::None);
 assert(down(i,m,512,32,1800)==Command::Restore);assert(m.apply(Command::Restore));i.cancel(true);
 assert(!m.performance&&m.page==Page::Root);assert(i.mask(1800)==0);
 assert(down(i,m,200,190,1810)==Command::None);assert(up(i,m,1820)==Command::None);
 tap(i,m,200,190,1900);assert(m.page==Page::Play);
 // A play-origin drag cannot activate Menu; moving out and back cancels a tap.
 down(i,m,250,300,2000);assert(i.mask(2000)==1);
 down(i,m,700,40,2020);assert(up(i,m,2040)==Command::None);
 down(i,m,700,40,2100);down(i,m,500,40,2120);down(i,m,700,40,2140);assert(up(i,m,2160)==Command::None);
 // A second contact invalidates menu taps even if the ordering changes.
 down(i,m,700,40,2200);Contact pair[2]={{700,40},{250,300}};
 assert(i.sample(true,pair,2,2220,m)==Command::None);std::swap(pair[0],pair[1]);
 i.sample(true,pair,2,2240,m);down(i,m,700,40,2260);assert(up(i,m,2280)==Command::None);
 // Direct two-eye play remains independent; no UI-origin poke leaks.
 Contact eyes[2]={{250,300},{750,300}};i.sample(true,eyes,2,2300,m);assert(i.mask(2300)==3);
 i.sample(true,eyes+1,1,2320,m);assert(i.mask(2320)==2);up(i,m,2340);assert(i.mask(2340)==0);
 down(i,m,700,40,2400);assert(i.mask(2400)==0);
 // Missing release, failure and stale packets cannot synthesize activation.
 i.expire(2500);assert(i.pressed()==Command::None);assert(up(i,m,2510)==Command::None);
 down(i,m,700,40,2600);i.sample(false,nullptr,0,2620,m);assert(up(i,m,2640)==Command::None);
 down(i,m,250,300,2700);assert(i.mask(2799)==1);i.expire(2800);assert(i.mask(2800)==0);
 down(i,m,250,300,2820);assert(i.mask(2820)==0);up(i,m,2840);down(i,m,250,300,2860);assert(i.mask(2860)==1);up(i,m,2880);
 // An idle gap alone must not consume the next fresh tap.
 i.expire(4000);tap(i,m,700,40,4100);assert(m.page==Page::Root);
 // Every page has reachable Back/Return and every button lies on screen.
 for(Page page:{Page::Root,Page::Apps,Page::Personality,Page::Display,Page::PerformanceConfirm,Page::Controls,Page::About}){
  m.page=page;unsigned n=m.items(items);bool exit=false;assert(n>0&&n<=6);
  for(unsigned k=0;k<n;k++){auto b=items[k].bounds;assert(b.x>=0&&b.y>=88&&b.x+b.w<=1024&&b.y+b.h<=600);assert(b.w>=160&&b.h>=52);if(items[k].command==Command::Back||items[k].command==Command::Return)exit=true;}
  assert(exit);
 }
 // Restore has strict origin/bounds, drag cancellation, freshness and clock wrap.
 m.page=Page::Play;m.performance=true;i.cancel(false);
 down(i,m,350,32,5000);down(i,m,512,32,5020);assert(up(i,m,5040)==Command::None);
 down(i,m,512,32,5100);down(i,m,672,32,5120);down(i,m,512,32,5140);assert(up(i,m,5160)==Command::None);
 down(i,m,512,32,5200);assert(down(i,m,512,32,6000)==Command::None);up(i,m,6020);
 uint32_t start=0xffffff00U;down(i,m,352,0,start);
 for(uint32_t delta=50;delta<800;delta+=50)assert(down(i,m,352,0,start+delta)==Command::None);
 assert(down(i,m,352,0,start+800)==Command::Restore);
 puts("PASS menu navigation, performance restore, touch origins, cancellation, freshness and wrap");
}
