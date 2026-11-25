#include "../firmware/FeatherFace/PeerBattery.h"
#include "../firmware/FeatherFace/FaceNavigation.h"
#include <cassert>
int main(){assert(FACE_COUNT==9);assert(faceEnabled(8)&&!faceEnabled(7));assert(voltageEstimated(8)&&hasBatteryMeasurement(8));assert(!strcmp(FACE_NAMES[8],"silver"));assert(nextFace(6,1)==8&&nextFace(8,1)==0&&nextFace(0,-1)==8);uint8_t mac[]={0x58,0xe6,0xc5,0x67,0xcb,0x70};assert(!memcmp(FACE_MACS[8],mac,6));}
