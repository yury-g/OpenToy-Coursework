#include "../firmware/FeatherFace/CameraControl.h"
#include <cassert>
int main(){uint8_t p[8];unsigned f=0;buildCameraControl(p,2);assert(decodeCameraControl(FACE_MACS[0],p,8,f)&&f==2);assert(decodeCameraControl(FACE_MACS[6],p,8,f));assert(!decodeCameraControl(FACE_MACS[1],p,8,f));assert(!decodeCameraControl(FACE_MACS[7],p,8,f));assert(!decodeCameraControl(FACE_MACS[0],p,7,f));p[6]=1;assert(!decodeCameraControl(FACE_MACS[0],p,8,f));buildCameraControl(p,3);assert(!decodeCameraControl(FACE_MACS[0],p,8,f));}
