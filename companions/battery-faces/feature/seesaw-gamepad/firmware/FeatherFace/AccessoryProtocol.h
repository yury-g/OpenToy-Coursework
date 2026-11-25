// SPDX-License-Identifier: MIT
#pragma once
#include "PeerBattery.h"
// Separate protocol; never extend or reinterpret FACE v3 battery packets.
// MAC and private-AP filtering are routing guards, not cryptographic authentication.
namespace accessory {
constexpr uint32_t MAGIC_COMMAND=0x46494e43,MAGIC_STATUS=0x46494e53,VERSION=1;
constexpr uint32_t LEASE_MS=6000,RENEW_MS=2000,MAX_SESSION_MS=120000,PULSE_MS=180,PULSE_INTERVAL_MS=1500,FRESH_MS=5000;
enum class Op:uint32_t {Start=1,Renew=2,Stop=3};
enum Flag:uint32_t {CAPABLE=1,READY=2,FINDING=4,BUTTON_READY=8,BUTTON_DOWN=16,MOTOR_ACTIVE=32,BREATHING=64,HELD=128,BREATH_ENABLED=256,RECENT_PRESS=512,LIVE_IO=1024,OUTLINE_VISIBLE=2048,OUTLINE_ACTIVE=4096};
constexpr uint32_t IO_DRIVE_SHIFT=16,IO_DRIVE_MASK=127U<<IO_DRIVE_SHIFT;
constexpr uint32_t ALLOWED_STATUS_FLAGS=CAPABLE|READY|FINDING|BUTTON_READY|BUTTON_DOWN|MOTOR_ACTIVE|BREATHING|HELD|BREATH_ENABLED|RECENT_PRESS|LIVE_IO|OUTLINE_VISIBLE|OUTLINE_ACTIVE|IO_DRIVE_MASK;
inline uint32_t ioDriveBits(uint32_t drive){return (drive>127?127:drive)<<IO_DRIVE_SHIFT;}
inline unsigned ioDrive(uint32_t flags){return (flags&IO_DRIVE_MASK)>>IO_DRIVE_SHIFT;}
inline bool ioKnown(uint32_t flags){return flags&LIVE_IO;}
inline const char *activityMode(uint32_t flags){return !ioKnown(flags)?"unknown":flags&BREATHING?"breathing":flags&HELD?"held":flags&MOTOR_ACTIVE?"vibrating":"idle";}
enum class Reason:uint32_t {Idle=0,Started=1,Renewed=2,Stopped=3,Found=4,LeaseExpired=5,SessionLimit=6,NotReady=7,Rejected=8};
struct Command {uint32_t magic=MAGIC_COMMAND,version=VERSION,target=0,targetBoot=0,controllerBoot=0,sequence=0,session=0,op=0;};
struct Status {uint32_t magic=MAGIC_STATUS,version=VERSION,target=0,targetBoot=0,sequence=0,flags=0,controllerBoot=0,ackSequence=0,session=0,reason=0,remainingMs=0,sourceAgeMs=0;};
static_assert(sizeof(Command)==32&&sizeof(Status)==48,"Accessory wire format");
inline bool supported(uint32_t target){return target<2;}
inline bool newer(uint32_t a,uint32_t b){return int32_t(a-b)>0;}
inline bool targetMac(const uint8_t *mac){return mac&&(!memcmp(mac,BLUE_MAC,6)||!memcmp(mac,GREEN_MAC,6));}
inline bool valid(const Command &c){return c.magic==MAGIC_COMMAND&&c.version==VERSION&&supported(c.target)&&c.targetBoot&&c.controllerBoot&&c.sequence&&c.session&&c.op>=1&&c.op<=3;}
inline bool valid(const Status &s){return s.magic==MAGIC_STATUS&&s.version==VERSION&&supported(s.target)&&s.targetBoot&&s.sequence&&!(s.flags&~ALLOWED_STATUS_FLAGS)&&s.reason<=8&&s.remainingMs<=MAX_SESSION_MS&&s.sourceAgeMs<=86400000;}
}
