#include "../firmware/FeatherFace/MonitorPacket.h"
#include <cassert>
int main(){
 // A relay must preserve source age; repeating an old packet cannot make it live.
 MonitorReceiver r;MonitorPacket p;p.face=0;p.packet.valid=1;p.packet.percent=35;p.packet.volts=3.8f;
 assert(r.receive(&p,sizeof(p),1000));assert(r.peers[0].fresh(1000));assert(!r.peers[2].seen);
 p.sourceAgeMs=7000;assert(r.receive(&p,sizeof(p),10000));assert(r.peers[0].fresh(10000));assert(!r.peers[0].fresh(11000));
 p.sourceAgeMs=9000;assert(r.receive(&p,sizeof(p),12000));assert(!r.peers[0].fresh(12000));
 p.face=7;assert(!r.receive(&p,sizeof(p),12300));p.face=4;p.sourceAgeMs=0;assert(r.receive(&p,sizeof(p),12400));p.face=3;p.sourceAgeMs=0;assert(r.receive(&p,sizeof(p),12500));assert(r.peers[3].fresh(12500));
 p.face=9;assert(!r.receive(&p,sizeof(p),13000));p.face=0;p.version=99;assert(!r.receive(&p,sizeof(p),13000));p.version=1;
 p.packet.percent=INFINITY;assert(!r.receive(&p,sizeof(p),13000));p.packet.percent=35;
 assert(!r.receive(&p,sizeof(p)-1,13000));
 unsigned live[FACE_COUNT];assert(r.liveFaces(live,13000)==2);assert(live[0]==3&&live[1]==4);assert(r.liveFaces(live,30000)==0);
}
