#include "../firmware/FeatherFace/BatteryLed.h"
#include <cassert>
int main(){
 BatteryState b;BatteryDiagnostics d;
 auto c=batteryLed(b,d,100);assert(c.r>0 && c.b>0 && c.g==0);
 b.valid=true;b.percent=10;c=batteryLed(b,d,500);assert(c.r>0 && c.g==0 && c.b==0);
 b.percent=50;c=batteryLed(b,d,500);assert(c.r>0 && c.g>0 && c.b==0);
 b.percent=86;c=batteryLed(b,d,500);assert(c.g>0 && c.r==0 && c.b==0);
 for(unsigned t=0;t<6000;t+=10){c=batteryLed(b,d,t);assert(c.r<=16 && c.g<=16 && c.b<=16);}
 auto a=batteryLed(b,d,0);c=batteryLed(b,d,1500);assert(a.g!=c.g);
}
