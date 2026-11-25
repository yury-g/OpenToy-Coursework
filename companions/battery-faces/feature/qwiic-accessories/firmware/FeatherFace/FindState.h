// SPDX-License-Identifier: MIT
#pragma once
#include "AccessoryProtocol.h"
namespace accessory {
struct FindNode {
 uint32_t target=0,boot=0,lastSequence=0,controller=0,session=0,started=0,renewed=0,lastPulse=0,statusSequence=0;
 bool active=false,hasPulsed=false;Reason reason=Reason::Idle;
 void begin(uint32_t id,uint32_t nonce){*this=FindNode{};target=id;boot=nonce?nonce:1;}
 bool close(Reason why){bool was=active;active=false;reason=why;return was;}
 bool tick(uint32_t now,bool ready){
  if(!active)return false;
  if(!ready)return close(Reason::NotReady);
  if(uint32_t(now-started)>=MAX_SESSION_MS)return close(Reason::SessionLimit);
  if(uint32_t(now-renewed)>=LEASE_MS)return close(Reason::LeaseExpired);
  return false;
 }
 bool apply(const Command &c,uint32_t now,bool ready){
  tick(now,ready);
  if(!valid(c)||c.target!=target||c.targetBoot!=boot||!newer(c.sequence,lastSequence))return false;
  // A single monotonic command floor survives controller reconnect/reboot until this target reboots.
  lastSequence=c.sequence;
  if(c.op==(uint32_t)Op::Start){
   if(!ready||active||(c.session==session&&c.controllerBoot==controller)){if(!active)reason=ready?Reason::Rejected:Reason::NotReady;return false;}
   controller=c.controllerBoot;session=c.session;started=renewed=now;hasPulsed=false;active=true;reason=Reason::Started;return true;
  }
  if(c.op==(uint32_t)Op::Stop){close(Reason::Stopped);return true;}
  if(!active||c.controllerBoot!=controller||c.session!=session)return false;
  renewed=now;reason=Reason::Renewed;return true;
 }
 bool acknowledge(){return active?close(Reason::Found):false;}
 bool pulseDue(uint32_t now)const{return active&&(!hasPulsed||uint32_t(now-lastPulse)>=PULSE_INTERVAL_MS);}
 void pulsed(uint32_t now){hasPulsed=true;lastPulse=now;}
 Status status(uint32_t now,bool ready){Status s;s.target=target;s.targetBoot=boot;s.sequence=++statusSequence;if(!s.sequence)s.sequence=++statusSequence;s.flags=CAPABLE|(ready?READY:0)|(active?FINDING:0);s.controllerBoot=controller;s.ackSequence=lastSequence;s.session=session;s.reason=(uint32_t)reason;s.remainingMs=active?MAX_SESSION_MS-uint32_t(now-started):0;return s;}
};
struct AccessoryPeer {
 Status status;uint32_t receivedAt=0,previousBoot=0;bool seen=false;
 bool receive(const Status &s,uint32_t now){
  if(!valid(s))return false;
  if(seen){if(s.target!=status.target)return false;if(s.targetBoot==status.targetBoot){if(!newer(s.sequence,status.sequence))return false;}else{if(s.targetBoot==previousBoot)return false;previousBoot=status.targetBoot;}}
  status=s;receivedAt=now;seen=true;return true;
 }
 uint32_t age(uint32_t now)const{uint32_t elapsed=now-receivedAt;return elapsed>86400000||status.sourceAgeMs>86400000-elapsed?86400000:elapsed+status.sourceAgeMs;}
 bool fresh(uint32_t now)const{return seen&&age(now)<FRESH_MS;}
 bool locatable(uint32_t now)const{return fresh(now)&&(status.flags&(CAPABLE|READY))==(CAPABLE|READY);}
};
}
