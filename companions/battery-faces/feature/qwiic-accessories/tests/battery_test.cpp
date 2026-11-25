#include "../firmware/FeatherFace/BatteryState.h"
#include <cassert>
#include <cmath>
#include <cstring>
int main(){
 BatteryState s;
 assert(!s.valid && s.trend==BatteryTrend::Unknown && s.hoursLeft<0);
 s.update(3.8f,55,0,0);assert(s.valid && s.percent==55 && s.trend==BatteryTrend::Settling && s.hoursLeft<0);
 s.update(3.8f,55,-10,30000);assert(s.trend==BatteryTrend::Discharging && fabsf(s.hoursLeft-5.5f)<.01f);
 s.update(4.1f,80,20,32000);assert(s.trend==BatteryTrend::Charging && s.hoursLeft<0);
 s.update(4.2f,105,0,34000);assert(s.percent==100 && s.trend==BatteryTrend::Steady && s.hoursLeft<0);
 s.update(NAN,50,2,36000);assert(!s.valid && s.hoursLeft<0 && s.trend==BatteryTrend::Unknown);
 s.update(0,50,2,38000);assert(!s.valid);
 s.update(3.9f,50,-.1f,40000);assert(s.valid && s.trend==BatteryTrend::Settling && s.hoursLeft<0);
 s.update(3.9f,50,-.1f,70000);assert(s.trend==BatteryTrend::Steady && s.hoursLeft<0);
 s.update(3.9f,50,NAN,72000);assert(s.valid && s.trend==BatteryTrend::Unknown && s.hoursLeft<0);
 // Startup grace and clock arithmetic survive millis wrap.
 BatteryState w;w.update(3.8f,50,-10,0xfffffff0U);w.update(3.8f,50,-10,40000);assert(w.trend==BatteryTrend::Discharging);
}
