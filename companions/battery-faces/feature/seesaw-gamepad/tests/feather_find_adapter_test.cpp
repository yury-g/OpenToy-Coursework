#include <cassert>
#include <vector>
#include <cstring>
#include "../firmware/FeatherFace/FindState.h"
unsigned testFace=1;unsigned localFace(){return testFace;}bool isRetiredGreenDevice(){return false;}
int peerMux=0;bool gatewayReady=false,radioReady=true,radioPaused=false;
#define portENTER_CRITICAL(x) ((void)(x))
#define portEXIT_CRITICAL(x) ((void)(x))
constexpr unsigned MONITOR_PORT=4242;uint32_t esp_random(){return 123;}
struct IPAddress {int last;IPAddress(int,int,int,int n):last(n){}bool operator==(const IPAddress &o)const{return last==o.last;}};
struct {unsigned softAPgetStationNum(){return 1;}} WiFi;
struct {int parsePacket(){return 0;}IPAddress remoteIP(){return IPAddress(192,168,4,2);}int read(uint8_t*,size_t){return 0;}int read(){return 0;}int available(){return 0;}void beginPacket(IPAddress,unsigned){}void write(const uint8_t*,size_t){}void endPacket(){}} monitorUdp;
unsigned pulses=0,stops=0,radioSends=0;bool capable=true,pulseBusy=false;
bool accessoryFindPulse(uint32_t){if(pulseBusy)return false;pulses++;return capable;}void accessoryFindStop(){stops++;}bool accessoryFindCapable(){return capable;}
uint32_t accessoryIoFlags(uint32_t){return 0;}
int esp_now_send(const uint8_t *dest,const uint8_t*,size_t){assert(accessory::targetMac(dest));radioSends++;return 0;}
#include "../firmware/FeatherFace/FeatherFind.h"
int main(){
 findService(0);assert(!findActive());assert(radioSends==1);
 accessory::Command c;c.target=1;c.targetBoot=123;c.controllerBoot=33;c.sequence=1;c.session=1;c.op=(uint32_t)accessory::Op::Start;
 assert(!findReceiveEspNow(RETIRED_GREEN_MAC,&c,sizeof(c),10));assert(!findReceiveEspNow(GREEN_MAC,&c,sizeof(c),10));
 PeerPacket battery;assert(!findReceiveEspNow(BLUE_MAC,&battery,sizeof(battery),10));
 assert(findReceiveEspNow(BLUE_MAC,&c,sizeof(c),10));assert(pulses==0);findService(10);assert(pulses==1&&findActive());
 assert(findLocalAcknowledge());assert(stops==1&&!findActive());assert(!findLocalAcknowledge());
 c.sequence=2;c.session=2;assert(findReceiveEspNow(BLUE_MAC,&c,sizeof(c),20));assert(pulses==1);pulseBusy=true;findService(20);assert(pulses==1&&findActive());pulseBusy=false;findService(21);assert(pulses==2&&findActive());findService(6020);assert(!findActive()&&stops==2);
 c.sequence=3;c.session=3;pulseBusy=true;assert(findReceiveEspNow(BLUE_MAC,&c,sizeof(c),7000));findService(7000);assert(findActive()&&pulses==2);findService(13000);assert(!findActive()&&stops==3);pulseBusy=false;
 testFace=0;accessory::Status status;status.target=1;status.targetBoot=4;status.sequence=1;status.flags=accessory::CAPABLE|accessory::READY;
 assert(!findReceiveEspNow(RETIRED_GREEN_MAC,&status,sizeof(status),6100));assert(!findReceiveEspNow(BLUE_MAC,&status,sizeof(status),6100));assert(findReceiveEspNow(GREEN_MAC,&status,sizeof(status),6100));findService(6100);assert(findGreen.locatable(6100));
}
