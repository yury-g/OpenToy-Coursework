#include <cassert>
#include <cmath>
#include <cstdio>
#include "../firmware/FaceMonitor/RuntimeObserver.h"
PeerBattery source(float pct,float v,uint32_t seq,uint32_t t,uint32_t uptime=0){PeerBattery p;p.seen=true;p.receivedAt=t;p.packet.valid=1;p.packet.percent=pct;p.packet.volts=v;p.packet.seq=seq;p.packet.uptime=uptime?uptime:seq*2000;return p;}
void feed(RuntimeObserver &r,uint32_t t,float pct=95,float v=4.1f){r.tick(t);for(unsigned k=0;k<4;k++)r.observe(k,source(pct,v,t/2000+1,t),t);}
int main(){
 RuntimeObserver r;feed(r,0);assert(r.ready(0)==4);assert(r.state.phase==RuntimeObserver::WAITING);
 assert(!r.start(9000));feed(r,10000);assert(r.start(10000));assert(r.state.phase==RuntimeObserver::RUNNING);
 for(uint32_t t=12000;t<=610000;t+=2000)feed(r,t,95-(t-10000)/60000.0f,4.1f-(t-10000)/6000000.0f);
 auto rate=r.rate(0,610000);assert(rate.valid&&fabsf(rate.pph+60)<.1f);assert(r.etaMinutes(0,610000)>0);
 assert(r.state.tracks[0].observedSeconds>=599);assert(r.state.tracks[0].count>5);
 r.tick(630000);assert(!r.rate(0,630000).valid);assert(r.linkLost(630000));assert(!r.state.tracks[1].lowAt);
 r.observe(0,source(84,4,999,630000,100),630000);assert(r.state.tracks[0].resets==1);assert(!r.rate(0,630000).valid);assert(r.state.tracks[0].interrupted);
 // Lost source, rising charge, duplicates and stale forwarded packets cannot produce runtime/ETA.
 RuntimeObserver d;feed(d,0);d.start(0);for(uint32_t t=2000;t<=10000;t+=2000){d.tick(t);d.observe(0,source(95,4.1,1,t,2000),t);}assert(d.state.tracks[0].observedSeconds==0);assert(!d.rate(0,10000).valid);
 auto stale=source(90,4,2,0);d.observe(0,stale,10000);assert(d.state.tracks[0].lastPercent==95);
 RuntimeObserver rise;for(uint32_t t=0;t<=400000;t+=2000)feed(rise,t,80+t/600000.0f);assert(rise.rate(0,400000).pph>0);assert(rise.etaMinutes(0,400000)<0);
 // Low is observed sustained voltage, not radio silence or one sag.
 RuntimeObserver low;feed(low,0);assert(low.start(0));for(uint32_t t=2000;t<=62000;t+=2000)feed(low,t,5,3.29);assert(low.state.tracks[0].lowAt==62);assert(low.state.tracks[0].lowAt==low.state.tracks[1].lowAt);
 RuntimeObserver sag;feed(sag,0);sag.start(0);feed(sag,2000,5,3.29);feed(sag,62000,5,3.29);assert(!sag.state.tracks[0].lowAt);
 // Compact history preserves bounds and a long timespan.
 RuntimeObserver longRun;for(uint32_t t=0;t<=24*3600000U;t+=2000)feed(longRun,t,50,3.8);assert(longRun.state.tracks[0].count<=121);assert(longRun.state.tracks[0].points[longRun.state.tracks[0].count-1].seconds>23*3600);
 // Snapshot restores only as interrupted; no rate continuity across power loss.
 auto saved=r.state;RuntimeObserver restored;assert(restored.restore(saved));assert(restored.state.phase==RuntimeObserver::INTERRUPTED);assert(!restored.rate(0,0).valid);saved.tracks[0].count=122;assert(!restored.restore(saved));
 RuntimeObserver wrap;feed(wrap,0xfffff000);feed(wrap,0x000007d0);assert(wrap.state.clockSeconds==6);
 // Radio timestamps jitter around two seconds; subsecond fractions must accumulate.
 RuntimeObserver jitter;jitter.tick(0);for(unsigned k=0;k<4;k++)jitter.observe(k,source(95,4.1,1,0),0);
 for(uint32_t n=1;n<=200;n++){uint32_t t=n*1990;jitter.tick(t);jitter.observe(0,source(95,4.1,n+1,t),t);}assert(jitter.state.tracks[0].observedSeconds==398);
 RuntimeObserver jitterRate;for(uint32_t n=0;n<=400;n++){uint32_t t=n*1990;jitterRate.tick(t);jitterRate.observe(0,source(95-t/60000.0f,4.1-t/6000000.0f,n+1,t),t);}assert(jitterRate.rate(0,796000).valid);assert(fabsf(jitterRate.rate(0,796000).pph+60)<.1f);
 RuntimeObserver order;order.tick(0);order.observe(0,source(95,4.1,1,0,1000),0);order.tick(4000);order.observe(0,source(94,4.0,3,4000,5000),4000);order.observe(0,source(93,3.9,2,2000,3000),4001);assert(order.state.tracks[0].resets==0&&order.state.tracks[0].lastPercent==94);
 RuntimeObserver partial;feed(partial,0);partial.tick(1000);assert(partial.start(1000));feed(partial,2000);assert(partial.state.tracks[0].runObservedSeconds==1);
 RuntimeObserver gaps;for(unsigned n=0;n<=97;n++)gaps.addPlot(0,n*60,95,4.1,n<2);assert(RuntimeObserver::gap(gaps.state.tracks[0].points[1]));
 auto corrupt=r.state;corrupt.tracks[0].lastPercent=1000000;RuntimeObserver bad;assert(!bad.restore(corrupt));
 RuntimeObserver bg;feed(bg,0);bg.start(0);feed(bg,60000);bg.stop();unsigned old=bg.state.tracks[0].count;feed(bg,120000);assert(bg.state.tracks[0].count==old);assert(bg.background.tracks[0].count>=3);RuntimeObserver bgRestore;assert(bgRestore.restoreBackground(bg.background));feed(bgRestore,1000);assert(bgRestore.background.tracks[0].count>=3);
 RuntimeObserver recovered;auto bgSaved=bg.background;bgSaved.tracks[0].observedSeconds=600;bgSaved.tracks[0].resets=2;assert(recovered.restoreBackground(bgSaved));feed(recovered,1000);assert(recovered.background.tracks[0].observedSeconds>=600);assert(recovered.background.tracks[0].resets>=2);
 RuntimeObserver alarm;for(uint32_t t=0;t<=62000;t+=2000)feed(alarm,t,5,3.29);assert(alarm.background.tracks[0].lowAt>0);feed(alarm,64000,10,3.5);assert(alarm.background.tracks[0].lowAt==0);
 puts("runtime observer: rates, gaps, source reset, low endpoint, compaction, restore, clock wrap PASS");
}
