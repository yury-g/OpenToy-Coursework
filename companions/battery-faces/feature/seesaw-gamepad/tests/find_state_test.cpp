#include <cassert>
#include <cstdio>
#include "../firmware/FeatherFace/FindState.h"
using namespace accessory;
Command command(FindNode &n,uint32_t seq,Op op=Op::Start,uint32_t session=1){Command c;c.target=1;c.targetBoot=n.boot;c.controllerBoot=44;c.sequence=seq;c.session=session;c.op=(uint32_t)op;return c;}
int main(){
 static_assert(sizeof(Command)!=sizeof(PeerPacket));static_assert(sizeof(Status)!=sizeof(PeerPacket));
 FindNode n;n.begin(1,123);auto c=command(n,1);assert(!n.apply(c,0,false));assert(!n.active);
 c.sequence=2;assert(n.apply(c,10,true));assert(n.active&&n.pulseDue(10));n.pulsed(10);assert(!n.pulseDue(1509));assert(n.pulseDue(1510));
 assert(!n.apply(c,100,true));c.sequence=3;c.op=(uint32_t)Op::Renew;assert(n.apply(c,5010,true));assert(n.active);assert(!n.tick(11009,true));assert(n.tick(11010,true));assert(!n.active);
 c.sequence=4;c.op=(uint32_t)Op::Start;assert(!n.apply(c,11020,true));assert(!n.active); // closed session never restarts
 c=command(n,5,Op::Renew,2);assert(!n.apply(c,12000,true));
 c=command(n,6,Op::Start,2);assert(n.apply(c,12000,true));assert(n.acknowledge());assert(!n.active);c.sequence=7;assert(!n.apply(c,12010,true));
 c=command(n,8,Op::Start,3);c.targetBoot++;assert(!n.apply(c,13000,true));c.targetBoot=n.boot;assert(n.apply(c,13000,true));
 for(uint32_t now=15000;now<133000;now+=2000){c.sequence++;c.op=(uint32_t)Op::Renew;assert(n.apply(c,now,true));n.tick(now,true);}
 assert(n.tick(133000,true));assert(!n.active);c.sequence++;assert(!n.apply(c,133001,true));
 c=command(n,100,Op::Start,4);assert(n.apply(c,134000,true));assert(n.tick(134001,false));assert(!n.active);
 FindNode wrap;wrap.begin(0,8);c=command(wrap,1);c.target=0;assert(wrap.apply(c,0xfffffff0U,true));assert(wrap.tick(5990,true));assert(!wrap.active);
 AccessoryPeer p;Status s;s.target=1;s.targetBoot=123;s.sequence=1;s.flags=CAPABLE|READY;assert(p.receive(s,100));assert(p.locatable(100));assert(!p.locatable(5100));assert(!p.receive(s,110));s.sequence=2;s.sourceAgeMs=5001;assert(p.receive(s,120));assert(!p.locatable(120));
 s.targetBoot=124;s.sequence=1;s.sourceAgeMs=0;assert(p.receive(s,130));s.targetBoot=123;s.sequence=10;assert(!p.receive(s,140));
 auto bad=command(n,200);bad.target=2;assert(!valid(bad));assert(!targetMac(RETIRED_GREEN_MAC));assert(targetMac(GREEN_MAC));assert(targetMac(BLUE_MAC));
 puts("Find state: leases, max session, ack, replay, boot nonce, wrap, freshness PASS");
}
