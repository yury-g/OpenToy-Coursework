// SPDX-License-Identifier: MIT
#include <cassert>
#include "../firmware/GravityWaterFeather/GamepadCursor.h"
int main(){
 feather_water::GamepadCursor c;
 assert(!c.update(512,512,false,false,.01f));assert(c.x()==67&&c.y()==151);
 assert(c.update(512,512,true,false,.01f));
 for(int i=0;i<500;++i)c.update(0,1023,false,false,.01f);
 assert(c.x()==129&&c.y()==221);
 for(int i=0;i<500;++i)c.update(1023,0,false,false,.01f);
 assert(c.x()==5&&c.y()==5);
 assert(!c.update(512,512,false,true,.01f));assert(c.x()==67&&c.y()==151);
 c.update(0,512,false,false,10);assert(c.x()<=76); // stale time cannot jump across screen
}
