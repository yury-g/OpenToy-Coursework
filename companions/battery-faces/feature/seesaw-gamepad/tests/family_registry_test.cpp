#include "../firmware/FeatherFace/FaceNavigation.h"
#include <cassert>
#include <cstring>
int main(){
 assert(FACE_COUNT==9);
 assert(faceEnabled(1) && !faceEnabled(7));
 assert(!strcmp(FACE_NAMES[6],"orange"));assert(!strcmp(FACE_NAMES[2],"pink"));
 assert(voltageEstimated(6));assert(hasBatteryMeasurement(6));assert(!hasBatteryMeasurement(7));
 assert(nextFace(5,1)==6);assert(nextFace(6,1)==8);assert(nextFace(7,1)==8);assert(nextFace(0,-1)==8);
 for(unsigned i=0;i<FACE_COUNT;i++)for(unsigned j=i+1;j<FACE_COUNT;j++)assert(memcmp(FACE_MACS[i],FACE_MACS[j],6));
}
