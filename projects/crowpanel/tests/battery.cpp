#include <cassert>
#include "../firmware/main/Battery.h"
int main(){
 uint8_t bytes[12]={0x40,0x0b,0,0,0xa3,0x0f,0,0,75,1,1,0}; // 2880 mV ADC, 4003 mV battery
 assert(crowpanel::batteryMillivolts(bytes,true)==4003);
 assert(crowpanel::batteryMillivolts(bytes,false)==-1);
 bytes[8]=255;assert(crowpanel::batteryMillivolts(bytes,true)==-1);
 bytes[8]=75;bytes[4]=0;bytes[5]=0;assert(crowpanel::batteryMillivolts(bytes,true)==-1);
}
