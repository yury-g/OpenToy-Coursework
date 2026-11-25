#include "../firmware/ToyBLE/FrameRetry.h"
#include <cassert>
#include <cstdio>
int main(){FrameRetry r;assert(r.allowed(0));r.completed(false,100);assert(!r.allowed(599));assert(r.allowed(600));r.completed(true,600);assert(r.allowed(601));r.completed(false,0xffffff00u);assert(!r.allowed(0xfffffff0u));assert(r.allowed(0x100u));puts("PASS failed-frame retry backoff and clock wrap");}
