#include "../firmware/FeatherFace/FaceNavigation.h"
#include <cassert>
#include <cstring>
int main(){
 assert(FACE_COUNT==8);
 assert(!faceEnabled(1));
 assert(!strcmp(FACE_NAMES[6],"pink"));assert(!strcmp(FACE_NAMES[7],"teal"));
 assert(voltageEstimated(6));assert(hasBatteryMeasurement(6));assert(!hasBatteryMeasurement(7));
 assert(nextFace(5,1)==6);assert(nextFace(6,1)==7);assert(nextFace(7,1)==0);assert(nextFace(0,-1)==7);
 for(unsigned i=0;i<FACE_COUNT;i++)for(unsigned j=i+1;j<FACE_COUNT;j++)assert(memcmp(FACE_MACS[i],FACE_MACS[j],6));
}
