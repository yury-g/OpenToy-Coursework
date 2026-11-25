// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
struct HapticPulse {
 static constexpr uint32_t DURATION_MS=180,COOLDOWN_MS=700;
 bool active=false,hasStarted=false;uint32_t began=0,duration=DURATION_MS;
 bool canStart(uint32_t now)const{uint32_t cooldown=duration+500;if(cooldown<COOLDOWN_MS)cooldown=COOLDOWN_MS;return !active&&(!hasStarted||uint32_t(now-began)>=cooldown);}
 bool start(uint32_t now,uint32_t length=DURATION_MS){if(length<50||length>800||!canStart(now))return false;active=true;hasStarted=true;began=now;duration=length;return true;}
 bool due(uint32_t now)const{return active&&uint32_t(now-began)>=duration;}
 void stopped(bool confirmed=true){if(confirmed)active=false;}
};
