#include "../firmware/NessoEyes/Presentation.h"
#include <cassert>
#include <cstdio>
#include <set>
using namespace nesso_eyes;
constexpr float pi=3.14159265358979323846f;
int main(){
 Orientation o;o.sample(0,0,-1,0,0);o.advance(0);
 // Gravity follows every intermediate angle, including the +/-pi seam.
 float previous=o.angle();
 for(unsigned t=10;t<=4000;t+=10){float target=t*.002f;o.sample(t,-std::sin(target),-std::cos(target),0,120);o.advance(t);float change=std::remainder(o.angle()-previous,2*pi);assert(change>=-.001f&&change<.04f);previous=o.angle();}
 assert(std::fabs(std::remainder(o.angle()-8.f,2*pi))<.2f);
 float held=o.angle();o.sample(4010,0,0,1,0);o.advance(4500);assert(o.angle()==held);
 o.sample(4510,9,9,9,0);o.advance(4600);assert(o.angle()==held);
 o.sample(4610,NAN,0,0,0);o.advance(4700);assert(o.angle()==held);
 Orientation wrap;wrap.sample(0xfffffff0,1,0,0,0);wrap.advance(0xfffffff0);wrap.sample(20,1,0,0,0);wrap.advance(20);assert(wrap.angle()<0&&std::isfinite(wrap.angle()));
 // Visible eye bounds include maximum trembling. Every angle fits; inverse
 // mapping picks the same eye throughout a full turn, including diagonals.
 for(int deg=0;deg<360;deg++){
  Transform tr(deg*pi/180);
  for(float x:{-45.f,45.f})for(float y:{-18.f,18.f}){auto p=tr.screen(x,y);assert(p.x>=2&&p.x<=238&&p.y>=2&&p.y<=133);}
  for(unsigned eye=0;eye<2;eye++){auto p=tr.screen(eye?28:-28,0);assert(tr.eye(p.x,p.y)==(1U<<eye));}
  assert(!tr.eye(0,0));auto h=tr.headerPoint(10,5);assert(!tr.face(h.x,h.y));
 }
 // Exercise the actual fixed-point renderer, with guards around the output.
 uint8_t mono[1024]={};for(int y=14;y<50;y++)for(int x=19;x<109;x++)mono[(y/8)*128+x]|=1<<(y%8);
 uint16_t output[240*135+2];output[0]=0x1234;output[240*135+1]=0x5678;
 for(int deg=0;deg<360;deg++){
  Transform tr(deg*pi/180);render(mono,output+1,tr,nullptr,0);unsigned white=0;
  for(int y=0;y<135;y++)for(int x=0;x<240;x++)if(output[1+y*240+x]){++white;assert(x>=2&&x<238&&y>=2&&y<133);}
  assert(white>3000);assert(output[0]==0x1234&&output[240*135+1]==0x5678);
 }
 // Tight eye framing makes both cardinal layouts materially larger.
 assert(Transform(0).scale>2.5f);assert(Transform(pi/2).scale>1.4f);
 std::set<int> positions;
 for(unsigned t=0;t<1000;t+=33){auto p=jitter(t,-100,0);assert(std::abs(p.x)<=3&&std::abs(p.y)<=2);positions.insert(p.x*10+p.y);auto calm=jitter(t,0,0);assert(calm.x==0&&calm.y==0);auto happy=jitter(t,100,1);assert(happy.x==0&&happy.y==0);}
 assert(positions.size()>10);
 puts("PASS continuous gravity rotation/seam, invalid/flat hold, clock wrap, maximum fitted bounds and transformed touch at 360 angles, anger jitter");
}
