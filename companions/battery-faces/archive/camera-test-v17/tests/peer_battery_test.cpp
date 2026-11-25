#include "../firmware/FeatherFace/PeerBattery.h"
#include <cassert>
#include <limits>
int main(){
 PeerBattery s;PeerPacket p;p.valid=1;p.percent=86;p.volts=4.07f;p.seq=3;p.seconds=120;
 assert(!s.fresh(0));assert(!s.receive(GREEN_MAC,&p,sizeof(p),0,GREEN_MAC));
 assert(!s.receive(BLUE_MAC,&p,sizeof(p)-1,10));
 uint8_t wrong[6]={};assert(!s.receive(wrong,&p,sizeof(p),10));
 p.version=99;assert(!s.receive(BLUE_MAC,&p,sizeof(p),10));p.version=3;
 p.volts=std::numeric_limits<float>::quiet_NaN();assert(!s.receive(BLUE_MAC,&p,sizeof(p),10));p.volts=4.07f;
 p.deltaPercent=INFINITY;assert(!s.receive(BLUE_MAC,&p,sizeof(p),10));p.deltaPercent=0;
 assert(s.receive(BLUE_MAC,&p,sizeof(p),0xfffffff0U));assert(s.fresh(30));assert(!s.fresh(9000));
 assert(s.packet.seq==3 && s.count==1);
 p.valid=0;p.volts=0;p.percent=0;assert(s.receive(BLUE_MAC,&p,sizeof(p),10000));assert(s.fresh(10001));assert(!s.packet.valid);
 p.valid=1;p.percent=101;p.volts=4;assert(!s.receive(BLUE_MAC,&p,sizeof(p),10002));
 // Catch source mixups and malformed measured-current fields.
 PeerBattery blue,orange;PeerPacket other;other.valid=1;other.volts=3.9f;other.percent=55;
 assert(blue.receive(BLUE_MAC,&other,sizeof(other),20000,BLUE_MAC));
 assert(!orange.receive(BLUE_MAC,&other,sizeof(other),20000,ORANGE_MAC));
 other.currentValid=1;other.currentMilliAmps=-74;other.chargerState=2;
 assert(orange.receive(ORANGE_MAC,&other,sizeof(other),20000,ORANGE_MAC));
 other.currentMilliAmps=NAN;assert(!orange.receive(ORANGE_MAC,&other,sizeof(other),21000,ORANGE_MAC));
 other.currentMilliAmps=0;other.chargerState=99;assert(!orange.receive(ORANGE_MAC,&other,sizeof(other),21000,ORANGE_MAC));
 assert(blue.packet.percent==55 && orange.packet.currentMilliAmps==-74);
}
