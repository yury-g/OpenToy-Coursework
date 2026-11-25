#include "../firmware/FeatherFace/CameraCapture.h"
#include <cassert>
int main(){assert(freshCameraCapture(1200000,1,0));assert(freshCameraCapture(1500000,1,0));assert(!freshCameraCapture(1500001,1,0));assert(!freshCameraCapture(30000000,1,0));assert(!freshCameraCapture(999999,1,0));assert(!freshCameraCapture(1200000,1,1000000));assert(!freshCameraCapture(1200000,-1,0));}
