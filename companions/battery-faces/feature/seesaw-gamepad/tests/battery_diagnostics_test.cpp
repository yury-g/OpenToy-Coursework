#include "../firmware/FeatherFace/BatteryDiagnostics.h"
#include <cassert>
#include <cmath>
int main(){
 BatteryState b;
 assert(batteryPose(b).expression==NERVOUS);
 b.update(3.6f,5,-5,0);assert(batteryPose(b).expression==ANGRY && batteryPose(b).left>0);
 b.percent=30;assert(batteryPose(b).expression==SORRY);
 b.percent=50;assert(batteryPose(b).expression==NEUTRAL);
 b.percent=70;assert(batteryPose(b).expression!=HAPPY);
 b.percent=70.01;assert(batteryPose(b).expression==HAPPY);
 BatteryDiagnostics d;b.percent=20;b.volts=3.6;
 d.update(b,0);d.update(b,119999);assert(!d.warmReady && !d.windowReady);
 for(unsigned t=120000;t<=420000;t+=60000){b.percent=20+(t-120000)/60000.f;b.volts=3.6f+(t-120000)/60000.f*.01f;d.update(b,t);}
 assert(d.windowReady && d.evidence==ChargeEvidence::Gaining);
 assert(fabsf(d.deltaPercent-5)<.01f && fabsf(d.deltaVolts-.05f)<.001f);
 assert(d.hoursToFull(b)>1 && d.hoursLeft(b)<0);
 BatteryDiagnostics flat;b.percent=50;b.volts=3.8;
 for(unsigned t=0;t<=420000;t+=60000)flat.update(b,t);
 assert(flat.evidence==ChargeEvidence::Holding);
 b.valid=false;flat.update(b,422000);assert(!flat.warmReady && !flat.windowReady);
 BatteryDiagnostics full;b.valid=true;b.percent=99;b.volts=4.18;
 for(unsigned t=0;t<=420000;t+=60000)full.update(b,t);
 assert(full.evidence==ChargeEvidence::NearFull);
 // Declining battery and a missing sample must not look like charging.
 BatteryDiagnostics drop;
 for(unsigned t=0;t<=420000;t+=60000){b.percent=80-t/60000.f;b.volts=4.1f-t/60000.f*.01f;drop.update(b,t);}
 assert(drop.evidence==ChargeEvidence::Losing && drop.hoursLeft(b)>0 && drop.hoursToFull(b)<0);
 BatterySession s;b.percent=15;b.volts=3.65;assert(!s.start(TestKind::Runtime,b,0));
 b.percent=80;assert(s.start(TestKind::Runtime,b,100));
 b.percent=70;s.update(b,600100);assert(s.record.elapsedSeconds==600 && fabsf(s.usedPercent()-10)<.01f);
 b.percent=20;s.update(b,660100);assert(!s.running() && s.record.outcome==TestOutcome::LowStop);
 b.percent=50;assert(s.start(TestKind::Watch,b,0));b.valid=false;s.update(b,1000);assert(s.record.outcome==TestOutcome::Interrupted);
 b.valid=true;b.percent=80;assert(s.start(TestKind::Runtime,b,0));s.update(b,1000,true);assert(s.record.outcome==TestOutcome::UsbStop);
 // Stop/resume persistence cannot silently continue a pre-reboot test.
 b.valid=true;assert(s.start(TestKind::Watch,b,0));SessionRecord r=s.record;BatterySession loaded;loaded.restore(r);assert(!loaded.running() && loaded.record.outcome==TestOutcome::Interrupted);
 BatteryDiagnostics wrap;b.percent=50;b.volts=3.8;
 uint32_t start=0xffff0000U;wrap.update(b,start);for(uint32_t dt=60000;dt<=420000;dt+=60000)wrap.update(b,start+dt);
 assert(wrap.windowReady && wrap.evidence==ChargeEvidence::Holding);
}
