// SPDX-License-Identifier: MIT
#pragma once
#include "../FeatherFace/FindState.h"
namespace accessory {
struct FindController {
 enum Phase {Idle,Starting,Active,Stopping};
 struct Request {Phase phase=Idle;Command command;uint32_t started=0,changed=0,lastSent=0;bool sent=false;};
 AccessoryPeer peers[2];Request requests[2];uint32_t boot=1,nextSequence[2]={};
 uint32_t sequence(unsigned i){uint32_t ack=peers[i].status.ackSequence;if(newer(ack,nextSequence[i]))nextSequence[i]=ack;if(!++nextSequence[i])++nextSequence[i];return nextSequence[i];}
 void stop(unsigned i,uint32_t now){
  auto &r=requests[i];if(r.phase==Stopping)return;if(!peers[i].seen){r.phase=Idle;return;}
  r.phase=Stopping;r.changed=now;r.sent=false;r.command=Command{};r.command.target=i;r.command.targetBoot=peers[i].status.targetBoot;r.command.controllerBoot=boot;r.command.sequence=sequence(i);r.command.session=peers[i].status.session?peers[i].status.session:r.command.sequence;r.command.op=(uint32_t)Op::Stop;
 }
 // Explicit bench diagnostic: create one leased START without a renewal request.
 bool oneShotLease(unsigned i,uint32_t now,Command &out){
  if(!supported(i)||requests[i].phase!=Idle||!peers[i].locatable(now)||(peers[i].status.flags&FINDING))return false;
  out=Command{};out.target=i;out.targetBoot=peers[i].status.targetBoot;out.controllerBoot=boot;out.sequence=sequence(i);out.session=out.sequence;out.op=(uint32_t)Op::Start;return true;
 }
 bool toggle(unsigned i,uint32_t now){
  if(!supported(i))return false;
  if(requests[i].phase!=Idle||(peers[i].fresh(now)&&(peers[i].status.flags&FINDING))){stop(i,now);return true;}
  if(!peers[i].locatable(now))return false;
  auto &r=requests[i];r=Request{};r.phase=Starting;r.started=r.changed=now;r.command.target=i;r.command.targetBoot=peers[i].status.targetBoot;r.command.controllerBoot=boot;r.command.sequence=sequence(i);r.command.session=r.command.sequence;r.command.op=(uint32_t)Op::Start;return true;
 }
 void stopAll(uint32_t now){for(unsigned i=0;i<2;i++)if(requests[i].phase!=Idle||(peers[i].fresh(now)&&(peers[i].status.flags&FINDING)))stop(i,now);}
 bool busy(uint32_t now)const{for(unsigned i=0;i<2;i++)if(requests[i].phase!=Idle||(peers[i].fresh(now)&&(peers[i].status.flags&FINDING)))return true;return false;}
 bool next(unsigned i,uint32_t now,Command &out){
  auto &r=requests[i];auto &p=peers[i];if(r.phase==Idle)return false;
  bool same=p.fresh(now)&&p.status.targetBoot==r.command.targetBoot&&p.status.controllerBoot==boot&&p.status.session==r.command.session;
  if(r.phase==Starting&&same&&(p.status.flags&FINDING)){r.phase=Active;r.changed=now;r.lastSent=now;}
  if(r.phase==Active){
   if(same&&!(p.status.flags&FINDING)){r.phase=Idle;return false;}
   if(!same||uint32_t(now-r.started)>=MAX_SESSION_MS)stop(i,now);
   else if(uint32_t(now-r.lastSent)>=RENEW_MS){r.command.op=(uint32_t)Op::Renew;r.command.sequence=sequence(i);r.sent=false;}
  }
  if(r.phase==Starting&&uint32_t(now-r.started)>=4000)stop(i,now);
  if(r.phase==Stopping){
   if((p.fresh(now)&&!(p.status.flags&FINDING)&&!newer(r.command.sequence,p.status.ackSequence))||uint32_t(now-r.changed)>=LEASE_MS){r.phase=Idle;return false;}
  }
  uint32_t interval=r.phase==Active?RENEW_MS:500;
  if(r.sent&&uint32_t(now-r.lastSent)<interval)return false;
  out=r.command;r.sent=true;r.lastSent=now;return true;
 }
};
}
