#include "../firmware/FeatherFace/CameraSnapshot.h"
#include <cassert>
#include <cstring>
int main(){uint8_t b[32];CameraSnapshotMeta m{7,10000,123,0x11223344,40},out{};encodeCameraSnapshot(b,m);assert(decodeCameraSnapshot(b,32,out));assert(out.sequence==7&&out.uptime==10000&&out.session==123&&out.checksum==0x11223344&&out.age==40);assert(!decodeCameraSnapshot(b,31,out));b[0]^=1;assert(!decodeCameraSnapshot(b,32,out));encodeCameraSnapshot(b,m);b[28]=0;assert(!decodeCameraSnapshot(b,32,out));m.age=5000;encodeCameraSnapshot(b,m);assert(!decodeCameraSnapshot(b,32,out));}
