#include "../firmware/WaveshareEyes/MenuView.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
using namespace waveshare_eyes;
static uint32_t now=1000;
void tapRect(Menu& m,Rect r){int cx=r.x+r.w/2,cy=r.y+r.h/2;m.update({true,cx,cy},now+=20);m.update({false,cx,cy},now+=20);}
void tap(Menu& m,int x,int y){m.update({true,x,y},now+=20);m.update({false,x,y},now+=20);}
void row(Menu& m,int i){tapRect(m,itemRect(i));}
void next(Menu& m){tapRect(m,nextButton);}
void back(Menu& m){tapRect(m,backButton);}
void assertSafeRect(Rect r){
 assert(r.w>0&&r.h>0);
 for(int y=r.y;y<r.y+r.h;y++)for(int x=r.x;x<r.x+r.w;x++){
  assert(x>=0&&x<360&&y>=0&&y<360);
  assert((x-180)*(x-180)+(y-180)*(y-180)<176*176);
 }
}
bool rectsOverlap(Rect a,Rect b){return a.x<b.x+b.w&&b.x<a.x+a.w&&a.y<b.y+b.h&&b.y<a.y+a.h;}
int main(){
 // All round touch targets must be comfortably sized (>=48px tall) and inside safe circle.
 assert(menuButton.h>=48&&menuButton.w>=120);
 assert(backButton.h>=48&&backButton.w>=100);
 assert(nextButton.h>=48&&nextButton.w>=100);
 assert(restoreZone.h>=48&&restoreZone.w>=96);
 assert(itemRect(0).h>=48&&itemRect(0).w>=232);
 assert(itemRect(1).h>=48&&itemRect(1).w>=232);
 assertSafeRect(menuButton);assertSafeRect(backButton);assertSafeRect(nextButton);
 assertSafeRect(restoreZone);assertSafeRect(itemRect(0));assertSafeRect(itemRect(1));
 assert(!rectsOverlap(itemRect(0),itemRect(1)));
 assert(!rectsOverlap(itemRect(1),backButton)&&!rectsOverlap(itemRect(1),nextButton));
 assert(!rectsOverlap(backButton,nextButton));
 // Menu and Restore must not overlap any eye hit region.
 for(int y=menuButton.y;y<menuButton.y+menuButton.h;y++)for(int x=menuButton.x;x<menuButton.x+menuButton.w;x++)assert(!eyeAt(x,y));
 for(int y=restoreZone.y;y<restoreZone.y+restoreZone.h;y++)for(int x=restoreZone.x;x<restoreZone.x+restoreZone.w;x++)assert(!eyeAt(x,y));

 Menu m;assert(m.page==Page::Play&&m.header&&!m.muted&&m.persona==1);
 int mx=menuButton.x+menuButton.w/2,my=menuButton.y+menuButton.h/2;
 int rx=restoreZone.x+restoreZone.w/2,ry=restoreZone.y+restoreZone.h/2;
 // A control doesn't activate on press; crossing into Menu from an eye isn't a tap.
 m.update({true,mx,my},now+=20);assert(m.page==Page::Play&&m.pressed(10));
 m.update({false,mx,my},now+=20);assert(m.page==Page::Root);
 row(m,0);assert(m.page==Page::Play);
 m.update({true,82,180},now+=20);assert(m.poke==1);
 m.update({true,mx,my},now+=20);m.update({},now+=20);assert(m.page==Page::Play&&m.poke==0);
 // Release outside or with unknown (0,0) cancels even without intermediate drag samples.
 m.update({true,mx,my},now+=20);m.update({false,40,120},now+=20);assert(m.page==Page::Play);
 m.update({true,mx,my},now+=20);m.update({false,0,0},now+=20);assert(m.page==Page::Play);
 m.update({true,mx,my},now+=20);m.update({},now+=20);assert(m.page==Page::Play);
 // Leaving and reentering cancels; failed reads must not act like a successful release.
 m.update({true,mx,my},now+=20);m.update({true,40,120},now+=20);m.update({true,mx,my},now+=20);m.update({},now+=20);assert(m.page==Page::Play);
 m.update({true,mx,my},now+=20);m.update({},now+=20,false);assert(m.page==Page::Play);
 m.update({true,82,180},now+=20);assert(!m.poke);m.update({},now+=20);

 // Root order across paginated pages: Return to play, Apps, Personality, Sound, Display, Controls, About.
 tap(m,mx,my);
 assert(std::string(m.itemLabel(0))=="Return to play");
 assert(std::string(m.itemLabel(1))=="Apps");
 row(m,1);assert(m.page==Page::Apps);assert(m.itemCount()==0);back(m);assert(m.page==Page::Root);

 next(m);
 assert(std::string(m.itemLabel(0))=="Personality");
 assert(std::string(m.itemLabel(1))=="Sound");
 row(m,0);assert(m.page==Page::Personality);
 assert(std::string(m.itemLabel(0))=="Curious");
 assert(std::string(m.itemLabel(1))=="Dreamy");
 next(m);assert(std::string(m.itemLabel(0))=="Cheeky");
 row(m,0);assert(m.persona==3&&m.selected(0));
 back(m);assert(m.page==Page::Root);

 row(m,1);assert(m.page==Page::Sound);
 assert(std::string(m.itemLabel(0))=="Mute");
 assert(std::string(m.itemLabel(1))=="Volume Soft");
 row(m,0);assert(m.muted&&std::string(m.itemLabel(0))=="Unmute");
 row(m,1);assert(m.volume==1&&m.selected(1));
 next(m);assert(std::string(m.itemLabel(0))=="Volume Quiet");
 row(m,0);assert(m.volume==2&&m.selected(0));
 back(m);assert(m.page==Page::Root);

 next(m);
 assert(std::string(m.itemLabel(0))=="Display");
 assert(std::string(m.itemLabel(1))=="Controls");
 row(m,0);assert(m.page==Page::Display);
 assert(std::string(m.itemLabel(0))=="Performance");
 assert(std::string(m.itemLabel(1))=="Brightness Dim");
 row(m,1);assert(m.brightness==0&&m.selected(1));
 next(m);assert(std::string(m.itemLabel(0))=="Brightness Normal");
 row(m,0);assert(m.brightness==1&&m.selected(0));
 next(m);row(m,0);assert(m.page==Page::Performance);
 assert(std::string(m.itemLabel(1))=="Enter performance");
 row(m,1);assert(m.page==Page::Play&&!m.header);

 // Restore once, then require release; sliding from an eye never qualifies.
 m.update({true,82,180},now+=20);m.update({true,rx,ry},now+=20);m.update({true,rx,ry},now+=800);assert(!m.header);m.update({},now+=20);
 m.update({true,rx,ry},now+=20);auto start=now;m.update({true,rx,ry},now+=799);assert(!m.header);m.update({true,rx,ry},now=start+800);assert(m.header&&m.page==Page::Root);
 m.update({true,180,150},now+=20);assert(!m.poke&&m.page==Page::Root);m.update({},now+=20);

 // Root page 3 -> Controls, page 4 -> About.
 next(m);next(m);
 row(m,1);assert(m.page==Page::Controls);next(m);back(m);assert(m.page==Page::Root);
 next(m);assert(std::string(m.itemLabel(0))=="About");
 row(m,0);assert(m.page==Page::About);back(m);back(m);assert(m.page==Page::Play);

 // External transitions consume held eye contacts, and explicit loss cancels holds.
 m.update({true,82,180},now+=20);m.openMenu();m.returnToPlay();m.update({true,82,180},now+=20);assert(!m.poke);m.update({},now+=20);m.update({true,82,180},now+=20);assert(m.poke==1);
 m.loseInput();m.update({true,82,180},now+=20);assert(!m.poke);m.update({},now+=20);
 m.enterPerformance();m.update({true,rx,ry},0xfffffff0);m.update({true,rx,ry},784);assert(m.header&&m.page==Page::Root);m.update({},804);
 m.audioReady=false;m.page=Page::Sound;assert(m.itemCount()==0&&!m.hasNext());back(m);assert(m.page==Page::Root);
 m.lightReady=false;m.openMenu();next(m);next(m);row(m,0);assert(m.page==Page::Display&&m.itemCount()==1&&!m.hasNext());

 // Obvious press feedback: pressed interior uses high-contrast highlight distinct from unpressed.
 {
  uint16_t idleRow[360]={},pressRow[360]={};
  int midY=menuButton.y+menuButton.h/2;
  buttonRow(idleRow,midY,menuButton,"Menu",false);
  buttonRow(pressRow,midY,menuButton,"Menu",true);
  int diff=0;for(int x=menuButton.x+4;x<menuButton.x+menuButton.w-4;x++)if(idleRow[x]!=pressRow[x])++diff;
  assert(diff>menuButton.w/2);
  assert(pressRow[menuButton.x+8]!=0x528a&&pressRow[menuButton.x+8]!=0x1082);
 }

 // Short 25ms tap inside a 50ms frame: frame-only polling misses it; inter-strip servicing catches it
 // without mixing old/new menu within the in-progress frame.
 {
  auto sampleAtTime=[&](uint32_t t)->Touch{
   if(t>=12&&t<37)return {true,mx,my};
   if(t>=37&&t<45)return {false,mx,my};
   return {false,0,0};
  };
  Menu frameOnly;frameOnly.update(sampleAtTime(0),0);frameOnly.update(sampleAtTime(50),50);
  assert(frameOnly.page==Page::Play); // missed when polling only at frame boundaries

  Menu live;uint8_t mono[1024]={};feather_face::Companion c(1);
  auto pose=c.update(512,512,0,false,1600,0);feather_face::drawCompanion(pose,1600,mono);
  const FrameSnapshot snap{live,4200,1,0xffff};
  std::vector<uint16_t> expected(360*360,0),actual(360*360,0);
  for(int y=0;y<360;y++)renderFrameRow(snap,mono,y,&expected[y*360]);
  uint16_t strip[60*360]={};
  uint32_t stripEndMs=0;
  bool ok=renderFrameStrips(
   snap,mono,60,strip,false,
   [&](int y,const uint16_t* pixels){
    std::memcpy(&actual[y*360],pixels,60*360*sizeof(uint16_t));
    return true;
   },
   [&](int){
    stripEndMs+=8;
    live.update(sampleAtTime(stripEndMs),stripEndMs);
   }
  );
  assert(ok);
  assert(live.page==Page::Root); // 25ms tap caught and committed during inter-strip servicing
  assert(actual==expected);      // zero mid-frame tearing despite transition at strip 4
 }

 // Curved header and centered Menu/menu pages never overlap, with real/missing voltage.
 for(int mv:{-1,2500,4200,4500})for(int y=0;y<128;y++){
  uint16_t status[360]={},control[360]={};Menu play;
  renderStatusRow(y,status,true,mv,1,false);renderMenuRow(play,y,control,mv);
  for(int x=0;x<360;x++)assert(!(status[x]&&control[x]));
 }
 // All actual controls and pixels fit round safe bounds and never overlap status header, including missing/valid battery.
 for(Page page:{Page::Play,Page::Root,Page::Apps,Page::Personality,Page::Sound,Page::Display,Page::Controls,Page::About,Page::Performance}){
  for(bool light:{true,false})for(bool audio:{true,false}){
   m.page=page;m.header=true;m.audioReady=audio;m.lightReady=light;
   for(unsigned pg=0;pg<4;pg++){
    m.rootPage=pg;m.subPage=pg%2;m.helpPage=pg%2;
    for(int y=0;y<360;y++){
     uint16_t guarded[362]={},status[360]={};guarded[0]=123;guarded[361]=456;
     renderMenuRow(m,y,guarded+1,4200);renderStatusRow(y,status,true,4200,1,false);
     assert(guarded[0]==123&&guarded[361]==456);
     for(int x=0;x<360;x++){
      if(guarded[x+1]){
       assert((x-180)*(x-180)+(y-180)*(y-180)<176*176);
       assert(!status[x]);
      }
     }
    }
   }
  }
 }
 puts("PASS round menu navigation, larger targets, inter-strip short taps, zero tearing and safe pixels");
}
