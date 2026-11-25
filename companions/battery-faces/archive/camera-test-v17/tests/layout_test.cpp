#include "../firmware/FeatherFace/TileLayout.h"
#include "../firmware/FeatherFace/FaceNavigation.h"
#include <cassert>
int main(){
 assert(FACE_COUNT==8 && !faceEnabled(1) && faceEnabled(4));
 assert(nextFace(0,1)==2 && nextFace(2,-1)==0 && nextFace(7,1)==0 && nextFace(0,-1)==7);
 for(unsigned count=1;count<=20;count++)for(unsigned i=0;i<count;i++){
  auto a=tileRect(i,count);assert(a.x>=16&&a.y>=72&&a.w>0&&a.h>0&&a.x+a.w<=1264&&a.y+a.h<=704);
  for(unsigned j=0;j<i;j++){auto b=tileRect(j,count);assert(a.x>=b.x+b.w||b.x>=a.x+a.w||a.y>=b.y+b.h||b.y>=a.y+a.h);}
 }
}
