#include "../firmware/FeatherFace/FaceNavigation.h"
#include <cassert>
int main(){
 // Catch missing wrap, accidental tap-as-swipe, diagonal drags and repeated triggering.
 assert(nextFace(7,1)==0);assert(nextFace(0,-1)==7);assert(nextFace(0,1)==2);
 SwipeGesture s;
 s.update(true,60,80,0);s.update(true,62,81,4900);assert(s.update(false,0,0,5000)==4);
 s.update(true,60,80,0);s.update(true,62,81,700);assert(s.update(false,0,0,750)==4);
 s.update(true,60,180,100);s.update(true,62,60,250);assert(s.update(false,0,0,300)==3);
 s.update(true,60,30,400);s.update(true,62,150,550);assert(s.update(false,0,0,600)==-3);
 assert(s.update(true,110,100,1000)==0);assert(s.update(true,35,106,1200)==0);
 assert(s.update(false,0,0,1250)==1);assert(s.update(false,0,0,1260)==0);
 assert(s.update(true,30,100,1700)==0);assert(s.update(true,100,104,1900)==0);assert(s.update(false,0,0,1950)==-1);
 assert(s.update(true,60,100,2300)==0);assert(s.update(true,65,103,2380)==0);assert(s.update(false,0,0,2400)==2);
 s.update(true,90,30,2800);s.update(true,40,150,3000);assert(s.update(false,0,0,3050)==0);
 s.update(true,110,60,3500);s.update(true,30,60,5200);assert(s.update(false,0,0,5250)==0);
 s.update(true,110,60,0xffffff00U);s.update(true,20,60,0xfffffff0U);assert(s.update(false,0,0,0x10U)==1);
}
