#include "../firmware/FeatherFace/BatteryMicroTrend.h"
#include <cassert>
#include <cmath>
int main(){
 BatteryMicroTrend t;BatteryState b;b.update(3.6f,10,0,0);
 t.update(b,0);assert(!t.available() && t.count==1);
 for(unsigned n=1;n<=60;n++){b.volts=3.6f+n*.0005f;b.percent=10+n*.005f;t.update(b,n*2000);}
 assert(t.available() && t.seconds==120 && fabsf(t.deltaVolts-.03f)<.0001f && fabsf(t.deltaPercent-.3f)<.0001f);
 assert(t.direction()==1);
 for(unsigned n=61;n<=120;n++){b.volts=3.63f-(n-60)*.001f;b.percent=10.3f-(n-60)*.01f;t.update(b,n*2000);}
 assert(t.direction()==-1 && t.count==61 && t.seconds==120);
 b.valid=false;t.update(b,242000);assert(!t.available() && t.count==0);
 BatteryMicroTrend w;b.valid=true;b.volts=3.6;b.percent=10;uint32_t start=0xffff0000U;
 for(unsigned n=0;n<=60;n++)w.update(b,start+n*2000);
 assert(w.seconds==120 && w.direction()==0);
}
