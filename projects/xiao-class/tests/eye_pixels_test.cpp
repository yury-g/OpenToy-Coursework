#include "../firmware/ToyBLE/EyePixels.h"
#include <cassert>
#include <cstdio>
int main(){
 FaceState s;
 assert(!eyeLayerPixel(s,0,14,16));assert(eyeLayerPixel(s,0,14,6));assert(!eyeLayerPixel(s,0,0,0));
 s.pupilX=100;s.pupilY=-100;
 assert(!eyeLayerPixel(s,0,20,9));assert(eyeLayerPixel(s,0,14,16));
 // Moving a pupil can only remove light inside the unchanged white shape.
 for(int px=-100;px<=100;px+=20)for(int py=-100;py<=100;py+=20){
  s.pupilX=px;s.pupilY=py;
  for(int y=0;y<32;y++)for(int x=0;x<28;x++)if(eyeLayerPixel(s,0,x,y))assert(eyeWhitePixel(NEUTRAL,0,x,y));
 }
 s=FaceState{};s.left=50;s.right=0;s.bottomLids=true;
 assert(!eyeLayerPixel(s,0,14,26)&&eyeLayerPixel(s,0,14,6));assert(eyeLayerPixel(s,1,14,26));
 s.bottomLids=false;assert(eyeLayerPixel(s,0,14,26)&&!eyeLayerPixel(s,0,14,6));
 s.left=100;unsigned count=0;for(int y=0;y<32;y++)for(int x=0;x<28;x++)count+=eyeLayerPixel(s,0,x,y);
 assert(count==28);assert(eyeLayerPixel(s,0,14,16)&&!eyeLayerPixel(s,0,14,6));
 assert(!eyeLayerPixel(s,0,-1,16)&&!eyeLayerPixel(s,0,28,16)&&!eyeLayerPixel(s,0,14,32));
 s=FaceState{};s.y=-100;assert(eyeWhiteY(s)==2);s.y=100;assert(eyeWhiteY(s)==30);
 puts("PASS pupil containment, opposite lid edges, independent wink and white travel bounds");
}
