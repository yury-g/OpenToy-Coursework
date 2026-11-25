// SPDX-License-Identifier: MIT
#pragma once
#include "DoodlePixels.h"
struct CachedFace {
 FaceState prior;bool ready=false;uint8_t normal[1024]={},closed[1024]={};
 static bool same(const FaceState &a,const FaceState &b){return a.expression==b.expression && a.eyeBend==b.eyeBend && a.browBend==b.browBend && a.mouthBend==b.mouthBend && a.mouthSkew==b.mouthSkew && a.jaw==b.jaw && a.left==b.left && a.right==b.right;}
 const uint8_t *pixels(const FaceState &s,bool blink){
  if(!ready || !same(prior,s)){
   prior=s;ready=true;drawDoodle(doodleTarget(s),s.expression,normal,0,0,0);
   FaceState b=s;b.left=b.right=100;drawDoodle(doodleTarget(b),b.expression,closed,0,0,0);
  }
  return blink?closed:normal;
 }
};
