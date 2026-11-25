#include "../firmware/FeatherFace/CameraView.h"
#include <cassert>
int main(){CameraView v;unsigned p=0;v.nextInfo(p,false);assert(p==1&&!v.active);v.nextInfo(p,false);assert(p==0&&!v.active);v.nextInfo(p,true);assert(p==1&&!v.active);v.nextInfo(p,true);assert(v.active);assert(!v.expire(true));assert(v.expire(false));assert(!v.active);v.select(true);assert(v.active);v.leave();assert(!v.active);p=0;v.nextInfo(p,true);v.nextInfo(p,true);v.nextInfo(p,true);assert(!v.active&&p==0);}
