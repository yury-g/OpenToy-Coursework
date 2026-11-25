#include "../firmware/FeatherFace/PeerBattery.h"
#include <cassert>
int main(){uint8_t replacement[]={0xac,0x27,0x6e,0xb1,0xcb,0xb0},retired[]={0xb8,0xf8,0x62,0xd5,0x80,0x14};assert(faceEnabled(1));assert(!faceEnabled(7));assert(!memcmp(GREEN_MAC,replacement,6));PeerPacket p;p.valid=1;p.percent=50;p.volts=3.8;PeerBattery b;assert(b.receive(replacement,&p,sizeof(p),100,GREEN_MAC));assert(!b.receive(retired,&p,sizeof(p),110,retired));assert(b.fresh(120));}
