// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <algorithm>
#include <cmath>
namespace copy_face {
constexpr unsigned lipPoints=9;
struct Point {float x=0,y=0;};
using LipEdge=std::array<Point,lipPoints>;
// Matched inner upper/lower lip samples, in image-left to image-right order.
// Only a qualified landmark adapter may mark real observations valid.
struct LipLandmarks {bool valid=false;LipEdge upper{},lower{};float imageAspect=1;};
struct EyeLandmarks {bool valid=false;std::array<Point,6> contour{};float imageAspect=1;};
struct LipShape {bool valid=false;LipEdge upper{},lower{};};
inline bool imagePoint(Point p) {
 return std::isfinite(p.x)&&std::isfinite(p.y)&&p.x>=0&&p.x<=1&&p.y>=0&&p.y<=1;
}
inline float length(Point a,Point b){return std::hypot(a.x-b.x,a.y-b.y);}
inline LipShape normalizeLips(const LipLandmarks& lips) {
 LipShape out;if(!lips.valid||!std::isfinite(lips.imageAspect)||lips.imageAspect<.2f||lips.imageAspect>5)return out;
 for(unsigned i=0;i<lipPoints;i++)if(!imagePoint(lips.upper[i])||!imagePoint(lips.lower[i]))return out;
 Point left{(lips.upper[0].x+lips.lower[0].x)/2,(lips.upper[0].y+lips.lower[0].y)/2};
 Point right{(lips.upper.back().x+lips.lower.back().x)/2,(lips.upper.back().y+lips.lower.back().y)/2};
 left.y/=lips.imageAspect;right.y/=lips.imageAspect;
 float width=length(left,right);if(width<.025f||right.x<=left.x)return out;
 float ux=(right.x-left.x)/width,uy=(right.y-left.y)/width;
 Point center{(left.x+right.x)/2,(left.y+right.y)/2};
 auto local=[&](Point p){float x=p.x-center.x,y=p.y/lips.imageAspect-center.y;return Point{2*(x*ux+y*uy)/width,2*(-x*uy+y*ux)/width};};
 for(unsigned i=0;i<lipPoints;i++) {
  out.upper[i]=local(lips.upper[i]);out.lower[i]=local(lips.lower[i]);
  const auto& a=out.upper[i];const auto& b=out.lower[i];
  if(std::fabs(a.x)>1.15f||std::fabs(b.x)>1.15f||std::fabs(a.y)>1||std::fabs(b.y)>1||
     std::fabs(a.x-b.x)>.20f||b.y<a.y-.025f)return LipShape{};
  if(i&&(a.x<=out.upper[i-1].x||b.x<=out.lower[i-1].x))return LipShape{};
 }
 out.valid=true;return out;
}
// Estimated contact, not a physical measurement: gap <= 1.5% of mouth width.
inline bool contact(const LipShape& l,unsigned i) {return l.valid&&i<lipPoints&&length(l.upper[i],l.lower[i])<=.03f;}
enum class Expression {Unknown,Neutral,Smile,Frown,Open};
inline Expression expression(const LipShape& l) {
 if(!l.valid)return Expression::Unknown;
 float gap=0,bend=0;
 for(unsigned i=3;i<=5;i++){gap+=l.lower[i].y-l.upper[i].y;bend+=(l.lower[i].y+l.upper[i].y)/2;}
 if(gap/3>.16f)return Expression::Open;
 if(bend/3>.13f)return Expression::Smile;
 if(bend/3<-.13f)return Expression::Frown;
 return Expression::Neutral;
}
inline const char* expressionName(Expression e) {
 switch(e){case Expression::Neutral:return "Neutral";case Expression::Smile:return "Smile";
 case Expression::Frown:return "Frown";case Expression::Open:return "Open mouth";default:return "Unavailable";}
}
inline bool validEye(const EyeLandmarks& e) {
 if(!e.valid||!std::isfinite(e.imageAspect)||e.imageAspect<.2f||e.imageAspect>5)return false;
 for(auto p:e.contour)if(!imagePoint(p))return false;
 return length(e.contour[0],e.contour[3])>.01f;
}
inline const char* eyeActivity(const EyeLandmarks& e) {
 if(!validEye(e))return "unknown";
 auto dist=[&](unsigned a,unsigned b){return std::hypot(e.contour[a].x-e.contour[b].x,(e.contour[a].y-e.contour[b].y)/e.imageAspect);};
 float ratio=(dist(1,5)+dist(2,4))/(2*dist(0,3));
 return ratio<.12f?"closed":"open";
}
}
