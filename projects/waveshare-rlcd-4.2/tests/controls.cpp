// SPDX-License-Identifier: MIT
#include "../firmware/RlcdEyes/Menu.h"
#include <cassert>
#include <cstdio>
using namespace rlcd_eyes;
int main(){
 Key key;
 assert(key.update(false,0)==KeyEvent::None);
 key.update(true,10);key.update(false,20);assert(key.update(false,60)==KeyEvent::None);
 key.update(true,100);key.update(true,130);key.update(false,200);
 assert(key.update(false,230)==KeyEvent::Short);
 key.update(true,300);key.update(true,330);
 assert(key.update(true,1129)==KeyEvent::None);
 assert(key.update(true,1130)==KeyEvent::Hold);
 assert(key.update(true,9000)==KeyEvent::None);
 key.update(false,9010);assert(key.update(false,9040)==KeyEvent::None);
 Key wrap;wrap.update(true,0xfffffff0U);wrap.update(true,14);
 assert(wrap.update(true,814)==KeyEvent::Hold);
 Menu m;assert(m.page==Page::Play&&m.headerVisible);
 assert(m.input(KeyEvent::Short).poke==3);
 assert(!m.input(KeyEvent::Hold).poke);assert(m.page==Page::Main&&m.focus==0);
 m.input(KeyEvent::Hold);assert(m.page==Page::Play);
 // Every top-level page is reachable and has an explicit return path.
 for(unsigned target=1;target<=6;target++){
  m.input(KeyEvent::Hold);for(unsigned i=0;i<target;i++)m.input(KeyEvent::Short);
  m.input(KeyEvent::Hold);assert(m.page!=Page::Main&&m.page!=Page::Play);
  while(m.focus+1<m.count())m.input(KeyEvent::Short);
  m.input(KeyEvent::Hold);assert(m.page==Page::Main);
  m.input(KeyEvent::Hold);assert(m.page==Page::Play);
 }
 m.input(KeyEvent::Hold);m.input(KeyEvent::Short);m.input(KeyEvent::Hold);
 assert(m.page==Page::Apps);assert(m.input(KeyEvent::Hold).poke==1&&m.page==Page::Play);
 m.input(KeyEvent::Hold);m.input(KeyEvent::Short);m.input(KeyEvent::Short);m.input(KeyEvent::Hold);
 m.input(KeyEvent::Short);assert(m.input(KeyEvent::Hold).personality==2&&m.page==Page::Main);
 m.input(KeyEvent::Hold);m.input(KeyEvent::Hold);
 for(unsigned i=0;i<4;i++)m.input(KeyEvent::Short);m.input(KeyEvent::Hold);
 assert(m.page==Page::Display);m.input(KeyEvent::Hold);assert(m.page==Page::Performance);
 m.input(KeyEvent::Hold);assert(m.page==Page::Play&&!m.headerVisible);
 m.input(KeyEvent::Hold);assert(m.page==Page::Main&&m.headerVisible&&m.focus==0);
 for(unsigned i=0;i<7;i++)m.input(KeyEvent::Short);assert(m.focus==0);
 assert(Menu{}.headerVisible);
 puts("PASS KEY debounce, short/hold isolation, stuck hold, wrap, menu reachability/back, pokes, personality and performance restore");
}
