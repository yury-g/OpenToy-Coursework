// SPDX-License-Identifier: MIT
#include <cassert>
#include "../firmware/GravityWaterFeather/FeatherAxes.h"
int main(){
 auto down=feather_water::screenDown(0,-1);assert(down.x==0&&down.y==1);
 auto right=feather_water::screenDown(-1,0);assert(right.x==1&&right.y==0);
 auto up=feather_water::screenDown(0,1);assert(up.y==-1);
 auto left=feather_water::screenDown(1,0);assert(left.x==-1);
 // These verify the mounting contract, not the external sensor's physical orientation.
}
