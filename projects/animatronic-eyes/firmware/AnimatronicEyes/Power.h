// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <cstring>
namespace feather_face {
// Every display dependency outside the exact monochrome pixels must invalidate reuse.
struct DisplayStamp {
 float scale=0;int battery=-1;uint32_t buttons=0,uiEpoch=0;unsigned expression=0,poke=0;
 bool valid=false,idle=false,header=true;
 bool operator==(const DisplayStamp& b)const{
  return scale==b.scale&&battery==b.battery&&buttons==b.buttons&&uiEpoch==b.uiEpoch&&expression==b.expression&&poke==b.poke&&valid==b.valid&&idle==b.idle&&header==b.header;
 }
};
class RefreshGate {
 uint8_t previous_[1024]={};DisplayStamp stamp_;bool initialized_=false;
public:
 bool changed(const uint8_t* pixels,const DisplayStamp& stamp){
  if(initialized_&&stamp==stamp_&&!std::memcmp(previous_,pixels,sizeof previous_))return false;
  std::memcpy(previous_,pixels,sizeof previous_);stamp_=stamp;initialized_=true;return true;
 }
};
}
