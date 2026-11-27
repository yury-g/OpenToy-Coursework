// SPDX-License-Identifier: MIT
#include "Scene.h"
#include <cstdio>
#include <vector>
using namespace touch169;
int main(){
 Interaction u;Scene s;Diagnostics d;std::vector<uint16_t> b(width*height);
 for(uint32_t t=0;t<=12000;t+=50){
  if(t==3000){u.update({true,true,55,140},t);}
  if(t==3050){u.update({true,false,55,140},t);s.input(u,t);}
  if(t==7000)u.page=Page::Hardware;
  if(t==9000)u.page=Page::Software;
  if(t==11000)u.page=Page::Play;
  s.render(b.data(),u,d,t);fwrite(b.data(),2,b.size(),stdout);
 }
}
