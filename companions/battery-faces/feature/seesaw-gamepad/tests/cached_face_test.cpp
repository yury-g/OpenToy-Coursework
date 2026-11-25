#include "../firmware/FeatherFace/CachedFace.h"
#include <cassert>
#include <cstring>
int main(){
 // A changed pose/eyelid must invalidate the cached pixels; static reuse must preserve original geometry.
 CachedFace cache;uint8_t expected[1024];FaceState s=restingDoodle();s.expression=NERVOUS;
 drawDoodle(doodleTarget(s),s.expression,expected,0,0,0);assert(!memcmp(cache.pixels(s,false),expected,1024));
 auto first=cache.pixels(s,false);assert(!memcmp(first,expected,1024));
 FaceState closed=s;closed.left=closed.right=100;drawDoodle(doodleTarget(closed),closed.expression,expected,0,0,0);assert(!memcmp(cache.pixels(s,true),expected,1024));
 s.expression=HAPPY;s.jaw=15;drawDoodle(doodleTarget(s),s.expression,expected,0,0,0);assert(!memcmp(cache.pixels(s,false),expected,1024));
 s.mouthBend=30;s.left=40;drawDoodle(doodleTarget(s),s.expression,expected,0,0,0);assert(!memcmp(cache.pixels(s,false),expected,1024));
}
