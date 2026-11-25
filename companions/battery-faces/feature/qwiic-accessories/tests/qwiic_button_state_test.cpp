#include <cassert>
#include <cstdio>
#include "../firmware/FeatherFace/QwiicButtonState.h"
int main(){QwiicButtonState b;assert(!b.update(true,4,0));assert(b.pressed);assert(!b.update(true,4,40));assert(!b.update(true,0,60));assert(!b.update(true,0,90));assert(!b.update(true,4,100));assert(b.update(true,4,130));assert(!b.update(true,4,300));assert(!b.update(false,0,350));assert(!b.update(true,4,400));assert(!b.update(true,0,500));assert(!b.update(true,0,530));assert(!b.update(true,4,600));assert(b.update(true,4,630));puts("Qwiic button: held boot, debounce, one pulse per press, no phantom reconnect PASS");}
