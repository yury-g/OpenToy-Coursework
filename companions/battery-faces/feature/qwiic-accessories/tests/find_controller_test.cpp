#include <cassert>
#include <vector>
#include "../firmware/FaceMonitor/FindController.h"
using namespace accessory;
int main(){
 FindController c;c.boot=99;assert(!c.toggle(0,0));Status s;s.targetBoot=12;s.sequence=1;s.flags=CAPABLE|READY;s.ackSequence=100;assert(c.peers[0].receive(s,100));assert(c.toggle(0,100));Command out;assert(c.next(0,100,out));assert(out.sequence==101&&out.op==(uint32_t)Op::Start);assert(!c.next(0,200,out));assert(c.next(0,600,out));assert(out.sequence==101); // retry does not open another session
 s.sequence++;s.flags|=FINDING;s.controllerBoot=99;s.session=101;s.ackSequence=101;assert(c.peers[0].receive(s,700));assert(!c.next(0,700,out));assert(c.next(0,2700,out));assert(out.op==(uint32_t)Op::Renew&&out.sequence==102);
 s.sequence++;s.flags=CAPABLE|READY;s.reason=(uint32_t)Reason::Found;assert(c.peers[0].receive(s,2800));assert(!c.next(0,2800,out));assert(!c.busy(2800));assert(!c.next(0,5000,out));
 assert(c.toggle(0,3000));assert(c.next(0,3000,out));uint32_t session=out.session;s.sequence++;s.flags|=FINDING;s.session=session;s.ackSequence=out.sequence;assert(c.peers[0].receive(s,3100));assert(!c.next(0,3100,out));c.stopAll(3200);assert(c.next(0,3200,out));assert(out.op==(uint32_t)Op::Stop);s.sequence++;s.flags=CAPABLE|READY;s.ackSequence=out.sequence;assert(c.peers[0].receive(s,3300));assert(!c.next(0,3300,out));assert(!c.busy(3300));
 assert(c.toggle(0,3400));assert(c.next(0,3400,out));assert(c.next(0,7500,out));assert(out.op==(uint32_t)Op::Stop); // no start acknowledgement, never renew indefinitely
 FindController once;once.boot=77;Status oneStatus;oneStatus.targetBoot=20;oneStatus.sequence=1;oneStatus.flags=CAPABLE|READY;oneStatus.ackSequence=8;assert(once.peers[0].receive(oneStatus,0));
 assert(!once.oneShotLease(2,0,out));assert(once.oneShotLease(0,0,out));assert(out.sequence==9&&out.session==9&&out.op==(uint32_t)Op::Start);assert(once.requests[0].phase==FindController::Idle);
 oneStatus.sequence++;oneStatus.flags|=FINDING;oneStatus.controllerBoot=77;oneStatus.session=9;oneStatus.ackSequence=9;assert(once.peers[0].receive(oneStatus,100));assert(once.busy(100));assert(!once.oneShotLease(0,100,out));
 for(unsigned t=0;t<=8000;t+=500)assert(!once.next(0,t,out)); // even fresh status can never enroll this diagnostic into renewal
 once.stopAll(1000);assert(once.next(0,1000,out)&&out.op==(uint32_t)Op::Stop); // global stop still routes
 FindController longRun;longRun.boot=9;s=Status{};s.targetBoot=10;s.sequence=1;s.flags=CAPABLE|READY;assert(longRun.peers[0].receive(s,0));assert(longRun.toggle(0,0));assert(longRun.next(0,0,out));s.session=out.session;s.controllerBoot=9;s.flags|=FINDING;
 for(uint32_t t=1000;t<120000;t+=1000){s.sequence++;assert(longRun.peers[0].receive(s,t));longRun.next(0,t,out);}
 assert(longRun.next(0,120000,out)&&out.op==(uint32_t)Op::Stop);
}
