#include "../firmware/NessoEyes/TouchReaction.h"
#include <cassert>
#include <cstdio>
using namespace nesso_eyes;
int main(){
 Reaction r;auto p=r.update(100,false,0);assert(p.mood==100&&p.face.left==0&&p.face.right==0);
 p=r.update(1000,true,1);assert(p.eyeMood[0]==-100&&p.eyeMood[1]>-100&&p.eyeMood[1]<0&&p.face.left==100&&p.face.right==0);
 p=r.update(1100,true,1);assert(p.face.left==100&&p.face.right>0&&p.face.right<50); // Brief sympathetic flinch.
 p=r.update(5000,true,1);assert(p.eyeMood[0]==-100&&p.face.left==100&&p.face.right==0);
 p=r.update(5125,false,0);assert(p.face.left>0&&p.face.left<100&&p.face.right==0);
 p=r.update(5500,false,0);assert(p.face.left==0);
 int previous[2]={-100,-100};for(uint32_t t=5500;t<=14000;t+=10){p=r.update(t,false,0);for(int i=0;i<2;i++){assert(p.eyeMood[i]>=previous[i]&&p.eyeMood[i]<=100);previous[i]=p.eyeMood[i];}if(t==8500)assert(p.eyeMood[0]<100&&p.eyeMood[1]==100);}
 assert(p.mood==100);
 p=r.update(15000,true,2);assert(p.face.right==100&&p.face.left==0&&p.eyeMood[1]<p.eyeMood[0]);
 p=r.update(15100,true,3);assert(p.face.left==100&&p.face.right==100);
 p=r.update(15300,false,0);assert(p.face.left!=p.face.right); // Separate reopening timing.
 p=r.update(15400,true,0);assert(p.eyeMood[0]<0&&p.eyeMood[1]<0); // Face poke still angers.
 Reaction wrap;wrap.update(0xfffffff0,true,2);p=wrap.update(0x200,false,0);assert(p.face.right==0&&p.mood<0);p=wrap.update(0x3000,false,0);assert(p.mood==100);
 // Pixel endpoints stay identical to the canonical v1 renderer.
 for(int mood:{-100,0,100})for(unsigned eye=0;eye<2;++eye)for(unsigned lid:{0U,50U,100U}){
  FaceState s;s.left=s.right=lid;s.pupilX=40;s.pupilY=-30;
  FaceState original=s;original.expression=mood<0?ANGRY:mood>0?HAPPY:NEUTRAL;
  for(int y=-1;y<=32;++y)for(int x=-1;x<=28;++x)assert(pixel(s,mood,eye,x,y)==eyeLayerPixel(original,eye,x,y));
 }
 puts("PASS stronger directly poked eye, independent flinch/reopening/recovery, held/repeated touches, timer wrap, original pixel endpoints");
}
