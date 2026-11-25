#include "../firmware/WaveshareEyes/Round.h"
#include "../firmware/WaveshareEyes/Sound.h"
#include "../firmware/WaveshareEyes/Status.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <set>
#include <string>
using namespace waveshare_eyes;
int main(){
 // Reserved codec bits need not echo writes. Only ADC/PGA shutdown bits matter.
 assert(codecCaptureOff(0x6a)&&codecCaptureOff(0x7f));
 assert(!codecCaptureOff(0x02)&&!codecCaptureOff(0x40));

 uint16_t statusRow[360];unsigned pixels=0;
 for(int y=0;y<360;y++){
  std::fill(statusRow,statusRow+360,0);renderStatusRow(y,statusRow,true,4200,1,false);
  for(int x=0;x<360;x++)if(statusRow[x]){assert(y<100);assert((x-180)*(x-180)+(y-180)*(y-180)<176*176);++pixels;}
  std::fill(statusRow,statusRow+360,0);renderStatusRow(y,statusRow,false,-1,1,false);
  for(auto p:statusRow)assert(!p);
 }
 assert(pixels>500);char voltage[8];batteryText(-1,voltage);assert(std::string(voltage)=="--");
 batteryText(4200,voltage);assert(std::string(voltage)=="4.20V");
 assert(eyeAt(82,180)==1&&eyeAt(278,180)==2);
 assert(!eyeAt(180,180)&&!eyeAt(0,0)&&!eyeAt(180,310));
 uint8_t data[6]={0,1,0,82,0,180};auto t=decodeTouch(data,true);assert(t.down&&t.x==82&&t.y==180);
 data[2]=0x40;auto lifted=decodeTouch(data,true);assert(!lifted.down&&lifted.x==82&&lifted.y==180);data[2]=0;
 data[1]=0;assert(!decodeTouch(data,true).down);data[1]=1;assert(!decodeTouch(data,false).down);
 data[2]=15;assert(!decodeTouch(data,true).down);assert(!touchPacketValid(data,true));
 data[1]=2;assert(!touchPacketValid(data,true));
 for(unsigned id=1;id<=3;id++){
  feather_face::Companion c(id);uint8_t mono[1024];uint16_t row[362];std::set<unsigned> hashes;
  for(unsigned now=0;now<16000;now+=50){
   auto p=c.update(512,512,0,false,now,now>=6000&&now<6500?1:0);feather_face::drawCompanion(p,now,mono);
   unsigned hash=0;for(int y=0;y<360;y++){row[0]=123;row[361]=456;renderRow(mono,y,row+1,0xffff);assert(row[0]==123&&row[361]==456);
    for(int x=0;x<360;x++)if(row[x+1]){assert((x-180)*(x-180)+(y-180)*(y-180)<176*176);hash=hash*33+x+y*360;}}
   hashes.insert(hash);
  }
  assert(hashes.size()>30);
 }
 SoundTrigger trigger;assert(trigger.update(1000,1,true)==1);assert(!trigger.update(1100,1,true));assert(!trigger.update(1200,0,true));assert(!trigger.update(1250,2,true));assert(trigger.update(1600,1,true)==1);assert(!trigger.update(2000,2,false));
 for(unsigned eye=1;eye<=2;eye++){
  int peak=0;long sum=0;for(unsigned i=0;i<soundSamples;i++){int s=soundSample(i,eye);assert(std::abs(volumeSample(s,1))<=450);assert(volumeSample(s,2)==s);peak=std::max(peak,std::abs(s));sum+=s;}
  assert(peak>100&&peak<=900&&std::abs(sum)<100000);assert(soundSample(0,eye)==0&&soundSample(soundSamples-1,eye)==0);
 }
 puts("PASS round-screen fit, exact touch eye mapping, failed-read release, three personas, bounded quiet sound and retrigger suppression");
}
