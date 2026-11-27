// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cmath>
namespace feather_water {
class GamepadCursor {
 float x_=67,y_=160;
public:
 static float axis(int raw){int d=std::max(0,std::min(1023,raw))-512;return std::abs(d)<=64?0.f:(d>0?d-64:d+64)/448.f;}
 bool update(int rawX,int rawY,bool press,bool center,float seconds) {
  if(center){x_=67;y_=160;}
  float dx=axis(1023-rawX),dy=axis(rawY);
  float dt=std::max(0.f,std::min(.05f,seconds));
  x_=std::max(5.f,std::min(129.f,x_+dx*180*dt));
  y_=std::max(5.f,std::min(234.f,y_+dy*180*dt));
  return press||dx!=0||dy!=0;
 }
 int x() const{return int(x_+.5f);}int y() const{return int(y_+.5f);}
};
}
