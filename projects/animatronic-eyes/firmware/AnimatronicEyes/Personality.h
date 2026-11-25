// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
namespace feather_face {
struct Personality {
 unsigned id;const char* name;uint32_t seed;unsigned pace;int gazeRange;
 unsigned firstBlink,winkPeriod;int eyeWidth[2],eyeHeight[2],eyeOffset[2];uint16_t ink;
};
inline const Personality& personality(unsigned id){
 static const Personality profiles[]={
  {0,"CLASSIC",0x15ea1234,100,60,1500,0,{28,28},{32,32},{0,0},0xffff},
  {1,"CURIOUS",0xc5011234,125,65,1000,0,{26,26},{32,32},{0,0},0xe7ff},
  {2,"DREAMY", 0xd2ea5678,70,35,2400,0,{28,28},{28,28},{0,0},0xff7b},
  {3,"CHEEKY", 0xc3ee90ab,110,60,1700,8500,{28,26},{30,32},{-1,1},0xcff9}
 };
 return profiles[id<4?id:0];
}
}
