// SPDX-License-Identifier: MIT
#pragma once
#include <Wire.h>
#include "HapticPulse.h"
#include "HapticBreathing.h"
#include "CreatureBreathing.h"
#include "src/haptic/Haptic_Driver.h"
// Reuse the Feather's live STEMMA QT bus; never restart Wire or cycle its rail.
struct FeatherHaptic {
 static constexpr uint8_t ADDRESS=0x4a,LEVELS[3]={20,64,127};
 Haptic_Driver driver;HapticPulse pulse;HapticBreathing breathing;CreatureBreathing creature;
 uint8_t level=2,chip=0,events=0,warning=0,mode=0,drive=0,commanded=0;
 bool present=false,ready=false,ioOk=false,driverBound=false,stopPending=false,stopTried=false,hasStopped=false,held=false;
 uint32_t pulses=0,errors=0,lastStopTry=0,lastStopped=0,lastBreathStep=0,lastFaultCheck=0,heldConfirmedAt=0,heldBegan=0;
 uint8_t strength()const{return LEVELS[level];}
 const char *levelName()const{return level==0?"LOW":level==1?"MED":"HIGH";}
 bool busy()const{return pulse.active||breathing.active||creature.active||held||stopPending;}
 bool quiet(uint32_t now)const{return !hasStopped||uint32_t(now-lastStopped)>=500;}
 uint8_t displayIntensity()const{return busy()?commanded:strength();}
 bool read(uint8_t reg,uint8_t &value){Wire.beginTransmission(ADDRESS);Wire.write(reg);if(Wire.endTransmission(false)!=0||Wire.requestFrom(ADDRESS,uint8_t(1))!=1)return false;value=Wire.read();return true;}
 // DA7280 IRQ_EVENT1/IRQ_EVENT_WARNING_DIAG: supply limiting is hardware
 // clipping, not a shutdown fault. Unknown/configuration/temperature warnings
 // still stop; diagnostic is freshly read on every refresh, never cached alone.
 bool faulted()const{return (events&0xda)||((events&0x20)&&(!warning||(warning&0x3f)));}
 bool refresh(){ioOk=read(0,chip)&&chip==0xba&&read(3,events)&&read(4,warning)&&read(0x22,mode)&&read(0x23,drive);if(!ioOk)errors++;return ioOk;}
 bool begin(){
  if(busy())return false;present=read(0,chip)&&chip==0xba;ready=false;if(!present)return false;driverBound=true;if(!driver.begin(Wire))return false;
  bool ok=driver.setVibrate(0)&&driver.setOperationMode(INACTIVE)&&driver.defaultMotor()&&driver.enableFreqTrack(false);
  if(!ok){driver.setVibrate(0);driver.setOperationMode(INACTIVE);errors++;return false;}
  uint8_t startup=0;if(!read(3,startup))return false;if(startup)driver.clearIrq(startup);
  commanded=0;ready=refresh()&&!faulted()&&(mode&7)==INACTIVE&&drive==0;return ready;
 }
 bool trigger(uint32_t now,uint32_t duration=HapticPulse::DURATION_MS){
  if(!ready||held||creature.active||breathing.active||stopPending||!quiet(now))return false;
  if(!refresh()||faulted()){ready=false;return false;}if(!pulse.start(now,duration))return false;stopTried=false;
  commanded=strength();
  if(!driver.setVibrate(commanded)||!driver.setOperationMode(DRO_MODE)||!refresh()||faulted()||drive!=commanded||(mode&7)!=DRO_MODE){stop();ready=false;errors++;return false;}
  pulses++;return true;
 }
 bool cycleAndTrigger(uint32_t now){if(busy()||!quiet(now)||!pulse.canStart(now))return false;level=(level+1)%3;return trigger(now);}
 bool startHeld(uint32_t now){
  if(!ready)return false;if(busy())stop();if(busy()||!ready)return false;
  if(!refresh()||faulted()){ready=false;return false;}
  held=true;heldBegan=heldConfirmedAt=lastFaultCheck=now;commanded=127;stopTried=false;
  if(!driver.setVibrate(commanded)||!driver.setOperationMode(DRO_MODE)||!refresh()||faulted()||drive!=commanded||(mode&7)!=DRO_MODE){stop();ready=false;errors++;return false;}
  pulses++;return true;
 }
 void confirmHeld(uint32_t now,bool down,bool valid){if(!held)return;if(!valid||!down){stop();return;}heldConfirmedAt=now;}
 bool startBreathing(uint32_t now,uint8_t peak,uint32_t seconds){
  if(!ready||busy()||!quiet(now)||!pulse.canStart(now)||!refresh()||faulted())return false;
  if(!breathing.start(now,peak,seconds))return false;
  commanded=0;lastBreathStep=lastFaultCheck=now;stopTried=false;
  if(!driver.setVibrate(0)||!driver.setOperationMode(INACTIVE)){stop();ready=false;errors++;return false;}
  pulses++;return true;
 }
 bool startCreature(uint32_t now,uint32_t seed){
  if(!ready||busy()||!quiet(now)||!refresh()||faulted())return false;
  creature.start(now,seed);commanded=0;lastBreathStep=lastFaultCheck=now;stopTried=false;
  if(!driver.setVibrate(0)||!driver.setOperationMode(INACTIVE)){stop();ready=false;errors++;return false;}
  pulses++;return true;
 }
 void stop(){
  breathing.stop();creature.stop();held=false;lastStopTry=millis();stopTried=true;
  if(!driverBound){pulse.stopped();stopPending=false;commanded=0;ready=false;return;}
  bool zero=driver.setVibrate(0),inactive=driver.setOperationMode(INACTIVE);
  bool confirmed=zero&&inactive&&refresh()&&drive==0&&(mode&7)==INACTIVE;
  stopPending=!confirmed;if(stopPending)pulse.active=true;else{pulse.stopped();commanded=0;lastStopped=millis();hasStopped=true;}
  if(!confirmed||faulted()){ready=false;errors++;}
 }
 void service(uint32_t now){
  if(stopPending){if(uint32_t(now-lastStopTry)>=100)stop();return;}
  if(held){
   if(uint32_t(now-heldConfirmedAt)>=250){stop();return;}
   if(uint32_t(now-lastFaultCheck)>=200){lastFaultCheck=now;if(!refresh()||faulted()||drive!=commanded||(mode&7)!=DRO_MODE){stop();ready=false;errors++;}}
   return;
  }
  if(breathing.active||creature.active){
   if(uint32_t(now-lastBreathStep)>=40){
    lastBreathStep=now;uint8_t next=creature.active?creature.update(now):breathing.update(now);if(!breathing.active&&!creature.active){stop();return;}
    if(next!=commanded){
     bool ok=driver.setVibrate(next)&&driver.setOperationMode(next?DRO_MODE:INACTIVE);
     if(!ok){stop();ready=false;errors++;return;}commanded=next;
    }
   }
   if(uint32_t(now-lastFaultCheck)>=200){lastFaultCheck=now;if(!refresh()||faulted()||drive!=commanded||(mode&7)!=(commanded?DRO_MODE:INACTIVE)){stop();ready=false;errors++;}}
   return;
  }
  if(pulse.due(now)&&(!stopTried||uint32_t(now-lastStopTry)>=100))stop();
 }
 void reportBreathing(){uint32_t now=millis();Serial.printf("BREATH active=%d peak=%u drive=%u elapsed_ms=%lu remaining_ms=%lu phase=%s\n",breathing.active,breathing.peak,commanded,(unsigned long)breathing.elapsed(now),(unsigned long)breathing.remaining(now),breathing.phase(now));}
 void reportCreature(bool enabled,uint32_t seed){uint32_t now=millis();Serial.printf("CREATURE enabled=%d active=%d seed=%lu peak=%u drive=%u phase=%s inhale_ms=%lu exhale_ms=%lu rest_ms=%lu cycle_elapsed_ms=%lu held=%d\n",enabled,creature.active,(unsigned long)seed,creature.peak,commanded,creature.phase(now),(unsigned long)creature.inhaleMilliseconds,(unsigned long)creature.exhaleMilliseconds,(unsigned long)creature.restMilliseconds,(unsigned long)uint32_t(now-creature.began),held);}
 void report(){refresh();Serial.printf("HAPTIC present=%d ready=%d io=%d address=4a chip=%02x button=D2 active=%d duration_ms=%lu at_ms=%lu start_ms=%lu amplitude=%u level=%s mode=%u drive=%u events=%02x pulses=%lu errors=%lu held=%d creature=%d warning_diag=%02x\n",present,ready,ioOk,chip,busy(),(unsigned long)((held||creature.active)?0:breathing.active?breathing.totalMilliseconds:pulse.duration),(unsigned long)millis(),(unsigned long)(held?heldBegan:creature.active?creature.began:breathing.active?breathing.began:pulse.began),strength(),levelName(),mode&7,drive,events,(unsigned long)pulses,(unsigned long)errors,held,creature.active,warning);}
};
