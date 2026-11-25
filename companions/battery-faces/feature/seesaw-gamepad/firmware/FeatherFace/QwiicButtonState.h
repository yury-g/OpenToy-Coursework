// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
struct QwiicButtonState {
 bool initialized=false,pressed=false,raw=false,armed=false;uint32_t changed=0;
 bool update(bool valid,uint8_t status,uint32_t now){
  if(!valid){initialized=false;armed=false;return false;}
  bool down=status&4;
  if(!initialized){initialized=true;pressed=raw=down;changed=now;armed=!down;return false;}
  if(raw!=down){raw=down;changed=now;}
  if(pressed!=raw&&uint32_t(now-changed)>=30){pressed=raw;if(!pressed)armed=true;else if(armed){armed=false;return true;}}
  return false;
 }
};
