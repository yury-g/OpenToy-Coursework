// SPDX-License-Identifier: MIT
#include "../core/Geometry.h"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace copy_face;
LipLandmarks lips(float bend=0,float gap=0) {
 LipLandmarks l;l.valid=true;
 for(unsigned i=0;i<lipPoints;i++) {
  float u=float(i)/(lipPoints-1),curve=4*u*(1-u);
  l.upper[i]={.3f+.4f*u,.6f+bend*curve-gap*curve/2};
  l.lower[i]={l.upper[i].x,l.upper[i].y+gap*curve};
 }
 return l;
}
int main() {
 auto neutral=normalizeLips(lips());assert(neutral.valid);
 assert(expression(neutral)==Expression::Neutral);
 for(unsigned i=0;i<lipPoints;i++)assert(contact(neutral,i));
 auto smile=normalizeLips(lips(.07f));assert(expression(smile)==Expression::Smile);
 auto frown=normalizeLips(lips(-.07f));assert(expression(frown)==Expression::Frown);
 auto open=normalizeLips(lips(0,.12f));assert(expression(open)==Expression::Open);
 assert(!contact(open,lipPoints/2));assert(contact(open,0));
 auto partial=lips();partial.lower[4].y+=.04f;auto separated=normalizeLips(partial);
 assert(contact(separated,2)&&!contact(separated,4));
 auto rolled=lips(.07f);
 for(auto* edge:{&rolled.upper,&rolled.lower})for(auto& p:*edge) {
  float x=p.x-.5f,y=p.y-.6f;p.x=.5f+.8f*x-.6f*y;p.y=.6f+.6f*x+.8f*y;
 }
 auto shape=normalizeLips(rolled);assert(shape.valid&&expression(shape)==Expression::Smile);
 assert(std::fabs(shape.upper[4].y-smile.upper[4].y)<1e-5f);
 auto bad=lips();bad.upper[4].x=2;assert(!normalizeLips(bad).valid);
 bad=lips();bad.upper[4].x=bad.upper[2].x;assert(!normalizeLips(bad).valid);
 assert(expression(LipShape{})==Expression::Unknown);
 auto wide=lips(.03f);wide.imageAspect=2;
 for(auto* edge:{&wide.upper,&wide.lower})for(auto& p:*edge)p.y=.6f+(p.y-.6f)*2;
 auto corrected=normalizeLips(wide),square=normalizeLips(lips(.03f));
 assert(std::fabs(corrected.upper[4].y-square.upper[4].y)<1e-5f);
 EyeLandmarks eye;assert(!validEye(eye));
 puts("geometry: contact, separation, shape guesses, roll and invalid points passed");
}
