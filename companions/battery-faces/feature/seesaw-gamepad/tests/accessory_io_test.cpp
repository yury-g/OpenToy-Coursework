#include <cassert>
#include <cstring>
#include "../firmware/FeatherFace/AccessoryProtocol.h"
using namespace accessory;
int main(){
 static_assert(sizeof(Command)==32&&sizeof(Status)==48&&sizeof(PeerPacket)==76);
 Status s;s.targetBoot=1;s.sequence=1;
 for(unsigned legacy=0;legacy<8;legacy++){s.flags=legacy;assert(valid(s));assert(!ioKnown(s.flags));assert(!strcmp(activityMode(s.flags),"unknown"));}
 for(unsigned drive=0;drive<=127;drive++){s.flags=CAPABLE|READY|LIVE_IO|BUTTON_READY|ioDriveBits(drive);assert(valid(s));assert(ioDrive(s.flags)==drive);assert(ioKnown(s.flags));}
 assert(ioDrive(ioDriveBits(128))==127);
 s.flags=LIVE_IO|OUTLINE_VISIBLE|OUTLINE_ACTIVE;assert(valid(s));
 s.flags=ALLOWED_STATUS_FLAGS;assert(valid(s));
 for(unsigned bit=0;bit<32;bit++)if(!(ALLOWED_STATUS_FLAGS&(1U<<bit))){s.flags=LIVE_IO|(1U<<bit);assert(!valid(s));}
 assert(!strcmp(activityMode(LIVE_IO),"idle"));assert(!strcmp(activityMode(LIVE_IO|MOTOR_ACTIVE),"vibrating"));assert(!strcmp(activityMode(LIVE_IO|HELD|MOTOR_ACTIVE),"held"));assert(!strcmp(activityMode(LIVE_IO|BREATHING|MOTOR_ACTIVE),"breathing"));
 assert(!strcmp(activityMode(BREATHING|MOTOR_ACTIVE),"unknown"));
}
