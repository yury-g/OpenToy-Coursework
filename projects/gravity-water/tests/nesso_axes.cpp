// SPDX-License-Identifier: MIT
#include <cassert>
#include "../firmware/GravityWaterNesso/NessoAxes.h"
int main() {
 // Measured while owner said board was upright on its bottom edge and bar correct:
 // accel=(0.996,0.005,0.003). It must send particles toward SCREEN bottom.
 auto upright=nesso_water::screenDown(1,0);
 assert(upright.x==0 && upright.y==1);
 auto right=nesso_water::screenDown(0,1);
 assert(right.x==1 && right.y==0);
 auto left=nesso_water::screenDown(0,-1);
 assert(left.x==-1 && left.y==0);
 auto inverted=nesso_water::screenDown(-1,0);
 assert(inverted.x==0 && inverted.y==-1);
}
