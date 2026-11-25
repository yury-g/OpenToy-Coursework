// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
namespace rlcd_eyes {
enum class KeyEvent {None,Short,Hold};
// Only the qualified user KEY enters this adapter. BOOT is never sampled.
class Key {
 bool raw_=false,stable_=false,fired_=false;
 uint32_t changed_=0,pressed_=0;
public:
 KeyEvent update(bool down,uint32_t now){
  if(down!=raw_){raw_=down;changed_=now;}
  if(uint32_t(now-changed_)>=30&&stable_!=raw_){
   stable_=raw_;
   if(stable_){pressed_=now;fired_=false;}
   else {const bool shortPress=!fired_;fired_=false;return shortPress?KeyEvent::Short:KeyEvent::None;}
  }
  if(stable_&&raw_&&!fired_&&uint32_t(now-pressed_)>=800){fired_=true;return KeyEvent::Hold;}
  return KeyEvent::None;
 }
};
}
