#include "../firmware/FeatherFace/DoodlePixels.h"
#include "../firmware/FeatherFace/FaceDisplay.h"
#include <cassert>
#include <cstdio>
#include <set>
int main() {
 uint8_t source[1024]={};
 uint16_t guarded[DISPLAY_WIDTH*DISPLAY_HEIGHT+2];
 guarded[0]=0x1234; guarded[DISPLAY_WIDTH*DISPLAY_HEIGHT+1]=0x5678;
 // OLED page-packed corners must all survive scaling, with black letterboxing.
 source[0]=1; source[127]=1; source[7*128]=128; source[1023]=128;
 scaleFace(source,guarded+1,0x07ff);
 auto pixel=[&](int x,int y){return guarded[1+y*DISPLAY_WIDTH+x];};
 assert(pixel(FACE_X,FACE_Y)==0x07ff && pixel(FACE_X+FACE_W-1,FACE_Y)==0x07ff);
 assert(pixel(FACE_X,FACE_Y+FACE_H-1)==0x07ff && pixel(FACE_X+FACE_W-1,FACE_Y+FACE_H-1)==0x07ff);
 assert(pixel(FACE_X,FACE_Y-1)==0 && pixel(FACE_X+FACE_W-1,FACE_Y+FACE_H)==0);
 assert(pixel(FACE_X+FACE_W/2,FACE_Y+FACE_H/2)==0);
 assert(FACE_W==2*FACE_H);
 assert(guarded[0]==0x1234 && guarded[DISPLAY_WIDTH*DISPLAY_HEIGHT+1]==0x5678);
 std::set<uint32_t> hashes;
 for(unsigned e=NEUTRAL;e<=NERVOUS;e++) {
  FaceState s=restingDoodle();s.expression=(Expression)e;
  drawDoodle(doodleTarget(s),s.expression,source,0,0,0);
  scaleFace(source,guarded+1,0x07ff);
  unsigned lit=0;for(int i=0;i<DISPLAY_WIDTH*DISPLAY_HEIGHT;i++)lit+=guarded[i+1]!=0;
  assert(lit>100 && lit<5000);hashes.insert(faceHash(source));
 }
 assert(hashes.size()==6);
 puts("PASS TFT corners, OLED bit layout, letterbox, buffer bounds and six face expressions");
}
