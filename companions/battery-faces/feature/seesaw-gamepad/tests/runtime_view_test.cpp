#include <cassert>
#include <cstdio>
#include "../firmware/FaceMonitor/RuntimeView.h"
int main(){
 RuntimeObserver real;real.tick(0);RuntimeView view;
 view.tap(850,30,0,real);assert(view.racePage&&!view.demoPage);
 view.tap(700,680,0,real);assert(!view.confirmation&&real.state.phase==RuntimeObserver::WAITING);
 auto before=real.state;view.tap(1100,30,0,real);assert(view.demoPage&&view.demo.state.session==1);
 assert(!memcmp(&before,&real.state,sizeof(before)));assert(view.demo.state.tracks[0].count>1);
 view.tap(700,680,1,real);assert(!view.demoPage&&real.state.phase==RuntimeObserver::WAITING);
 for(unsigned k=0;k<4;k++){PeerBattery p;p.seen=true;p.packet.valid=1;p.packet.percent=95;p.packet.volts=4.15;p.packet.seq=1;real.observe(k,p,0);}
 view.tap(700,680,0,real);assert(view.confirmation==1);assert(real.state.phase==RuntimeObserver::WAITING);
 for(unsigned k=0;k<4;k++)real.state.tracks[k].lastPercent=85; // normal voltage relaxation after unplug
 assert(view.tap(700,680,100,real)==1);assert(real.state.phase==RuntimeObserver::RUNNING);
 view.tap(700,680,200,real);assert(view.confirmation==2);assert(real.state.phase==RuntimeObserver::RUNNING);
 assert(view.tap(700,680,300,real)==1);assert(real.state.phase==RuntimeObserver::STOPPED);
 puts("runtime view: readiness, explicit start/stop and demo isolation PASS");
}
