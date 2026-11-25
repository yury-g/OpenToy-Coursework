// SPDX-License-Identifier: MIT
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <esp_ota_ops.h>
#include <esp_mac.h>
#include <atomic>
#include "Protocol.h"
#include "Ota.h"
#include "BuildConfig.h"
#ifndef TOY_BUILD
#define TOY_BUILD "A"
#endif
#ifndef TOY_FAIL_BOOT
#define TOY_FAIL_BOOT 0
#endif
const char *NUS="6e400001-b5a3-f393-e0a9-e50e24dcca9e";
const char *UART_RX_UUID="6e400002-b5a3-f393-e0a9-e50e24dcca9e";
const char *UART_TX_UUID="6e400003-b5a3-f393-e0a9-e50e24dcca9e";
const char *OTA_SERVICE="8e7c0001-54ad-4da3-97f4-754ac67c0a00";
const char *OTA_PACKET="8e7c0002-54ad-4da3-97f4-754ac67c0a00";
const char *OTA_STATUS="8e7c0003-54ad-4da3-97f4-754ac67c0a00";
struct Message {uint32_t generation;uint16_t len;uint8_t channel;uint8_t bytes[244];};
QueueHandle_t messages;
std::atomic<uint32_t> sessionGeneration{0};
std::atomic<bool> advertisingReady{false};
std::atomic<bool> connected{false},resetSession{false},overflow{false};
BLECharacteristic *tx,*otaStatus;
BLEServer *bleServer=nullptr;
std::atomic<bool> notifyFailed{false};
std::atomic<unsigned> notifyErrors{0};
ToyState state;
FrameParser parser,serialParser;
ToyOta ota;
uint32_t lastByte=0,lastHeartbeat=0,epoch=0,sequence=0,rebootAt=0,bootAt=0;
bool confirmed=false;
// Delay Arduino's default immediate OTA confirmation until application self-tests.
extern "C" bool verifyRollbackLater(){return true;}
class ServerCallbacks:public BLEServerCallbacks{
 void onConnect(BLEServer*)override{sessionGeneration++;connected=true;resetSession=true;}
#if defined(CONFIG_NIMBLE_ENABLED)
 void onConnect(BLEServer *s,struct ble_gap_conn_desc *d)override{s->requestConnParams(d->conn_handle,12,24,0,400);}
#endif
 void onDisconnect(BLEServer*)override{sessionGeneration++;connected=false;resetSession=true;advertisingReady=false;}
};
class ReceiveCallbacks:public BLECharacteristicCallbacks{
 uint8_t channel;
public: explicit ReceiveCallbacks(uint8_t c):channel(c){}
 void onWrite(BLECharacteristic *c)override{
  String v=c->getValue();Message m{};unsigned framesHandled=0;
  if(v.length()>sizeof(m.bytes)||!v.length()){overflow=true;return;}
  m.generation=sessionGeneration.load();m.channel=channel;m.len=v.length();memcpy(m.bytes,v.c_str(),m.len);
  if(xQueueSend(messages,&m,0)!=pdTRUE)overflow=true;
 }
};
class TxCallbacks:public BLECharacteristicCallbacks{
 void onStatus(BLECharacteristic*,Status s,uint32_t)override{if(s==ERROR_GATT){notifyFailed=true;notifyErrors++;}}
};
unsigned peerMtu(){unsigned mtu=connected&&bleServer?bleServer->getPeerMTU(bleServer->getConnId()):23;return mtu>=23?mtu:23;}
void notifyText(const char *text){
 tx->setValue(text);
 if(connected){size_t n=strlen(text),chunk=min((size_t)180,(size_t)peerMtu()-3);
  for(size_t i=0;i<n;i+=chunk){size_t count=min(chunk,n-i);tx->setValue((uint8_t*)text+i,count);
   for(unsigned retry=0;retry<3;retry++){notifyFailed=false;tx->notify();if(!notifyFailed)break;delay(10);}
   delay(10);
  }
  tx->setValue(text);
 }
}
void report(const char *result){
 char b[240];snprintf(b,sizeof(b),"%s build=%s seq=%lu period=%u duty=%u mode=%s pin=%d ota=%d bytes=%lu mtu=%u txerr=%u\n",result,TOY_BUILD,(unsigned long)sequence,state.period,state.duty,modeName(state.mode),digitalRead(21),ota.busy(),(unsigned long)ota.offset(),peerMtu(),notifyErrors.load());
 if(Serial.availableForWrite()>=(int)strlen(b))Serial.print(b);notifyText(b);
}
void otaReply(){otaStatus->setValue(ota.response);if(connected)otaStatus->notify();}
void setup(){
 Serial.begin(115200);Serial.setTxTimeoutMs(1);delay(600);pinMode(21,OUTPUT);digitalWrite(21,HIGH);
 Serial.printf("BOOT ToyBLE build=%s chip=%s mac=%llx partition=%s fail_test=%d\n",TOY_BUILD,ESP.getChipModel(),(unsigned long long)ESP.getEfuseMac(),esp_ota_get_running_partition()->label,TOY_FAIL_BOOT);
 if(TOY_FAIL_BOOT){Serial.println("TEST pending image restarts before validation");Serial.flush();delay(1000);ESP.restart();}
 ToyState test;bool ok=applyCommand("period 250",test)==ACCEPTED&&test.period==250&&applyCommand("period 99",test)==INVALID&&test.period==250;
 messages=xQueueCreate(16,sizeof(Message));
 if(!ok||!messages||strcmp(ESP.getChipModel(),"ESP32-S3")){Serial.println("SELFTEST FAIL");delay(1000);esp_ota_mark_app_invalid_rollback_and_reboot();ESP.restart();}
 uint8_t nameMac[6];esp_read_mac(nameMac,ESP_MAC_EFUSE_FACTORY);char name[24];snprintf(name,sizeof(name),"XIAO Toy %02X%02X",nameMac[4],nameMac[5]);
 if(!BLEDevice::init(name)){Serial.println("BLE INIT FAIL");delay(500);ESP.restart();}
 BLEDevice::setMTU(247);
 BLEServer *server=bleServer=BLEDevice::createServer();server->setCallbacks(new ServerCallbacks());
 uint8_t mac[6];esp_read_mac(mac,ESP_MAC_EFUSE_FACTORY);char identity[18];snprintf(identity,sizeof(identity),"%02x:%02x:%02x:%02x:%02x:%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
 BLEService *dis=server->createService(BLEUUID((uint16_t)0x180A));auto *serial=dis->createCharacteristic(BLEUUID((uint16_t)0x2A25),BLECharacteristic::PROPERTY_READ);serial->setValue(identity);dis->start();
 BLEService *uart=server->createService(NUS);
 tx=uart->createCharacteristic(UART_TX_UUID,BLECharacteristic::PROPERTY_READ|BLECharacteristic::PROPERTY_NOTIFY);tx->addDescriptor(new BLE2902());tx->setCallbacks(new TxCallbacks());
 BLECharacteristic *rx=uart->createCharacteristic(UART_RX_UUID,BLECharacteristic::PROPERTY_WRITE|BLECharacteristic::PROPERTY_WRITE_NR);rx->setCallbacks(new ReceiveCallbacks(0));uart->start();
 BLEService *os=server->createService(OTA_SERVICE);
 auto *op=os->createCharacteristic(OTA_PACKET,BLECharacteristic::PROPERTY_WRITE|BLECharacteristic::PROPERTY_WRITE_NR);op->setCallbacks(new ReceiveCallbacks(1));
 otaStatus=os->createCharacteristic(OTA_STATUS,BLECharacteristic::PROPERTY_READ|BLECharacteristic::PROPERTY_NOTIFY);otaStatus->addDescriptor(new BLE2902());ota.init();otaReply();os->start();
 auto *adv=BLEDevice::getAdvertising();adv->addServiceUUID(NUS);adv->setScanResponse(true);adv->setMinPreferred(0x06);adv->setMaxPreferred(0x12);if(!adv->start()){Serial.println("BLE ADV FAIL");delay(500);ESP.restart();}
 epoch=millis();bootAt=millis();Serial.println("SELFTEST PASS BLE advertising; WiFi unused");report("READY");
}
void loop(){
 if(resetSession.exchange(false)){parser.reset();xQueueReset(messages);ota.abort();ota.init();otaReply();Serial.println(connected?"BLE CONNECT":"BLE DISCONNECT; advertising restart");if(!connected)BLEDevice::startAdvertising();}
 if(overflow.exchange(false)){xQueueReset(messages);parser.reset();ota.abort();strcpy(ota.response,"ERR queue");otaReply();report("ERR queue");}
 if(parser.pending()&&millis()-lastByte>1000)parser.reset();
 Message m{};unsigned framesHandled=0;
 // Bound work per loop so a client cannot starve runtime/heartbeat.
 for(int batch=0;batch<8 && framesHandled<4 && xQueueReceive(messages,&m,0)==pdTRUE;batch++){
  if(!connected || m.generation!=sessionGeneration.load())continue;
  if(m.channel==1){bool reboot=ota.packet(m.bytes,m.len);otaReply();if(reboot)rebootAt=millis()+700;continue;}
  Frame f;for(unsigned i=0;i<m.len;++i){if(!connected||m.generation!=sessionGeneration.load()){parser.reset();break;}if(framesHandled>=4){parser.reset();report("ERR rate");break;}lastByte=millis();if(!parser.feed(m.bytes[i],f))continue;framesHandled++;if(!connected||m.generation!=sessionGeneration.load()){parser.reset();break;}
   if(f.kind==BAD_FRAME){report("ERR frame");continue;}
   if(f.kind==BUTTON){applyButton(f.button,f.pressed,state);sequence++;epoch=millis();report("OK button");continue;}
   Result r=applyCommand(f.text,state);
   if(r==INVALID)report("ERR command");else{if(r==ACCEPTED){sequence++;epoch=millis();}report(r==STATUS_QUERY?"STATUS":"OK");}
  }
 }
 for(unsigned serialBytes=0;serialBytes<128&&framesHandled<4&&Serial.available();serialBytes++){Frame f;if(serialParser.feed(Serial.read(),f)&&f.kind==TEXT){framesHandled++;Result r=applyCommand(f.text,state);if(r==ACCEPTED){sequence++;epoch=millis();}report(r==INVALID?"ERR command":r==STATUS_QUERY?"STATUS":"OK");}lastByte=millis();}
 bool on=state.mode==ON||(state.mode==BLINK&&((millis()-epoch)%state.period)<state.period*state.duty/100);
 digitalWrite(21,on?LOW:HIGH);
 if(ota.tick())otaReply();
 if(!confirmed&&millis()-bootAt>2500&&(BLEDevice::getAdvertising()->isAdvertising()||connected)){esp_err_t e=esp_ota_mark_app_valid_cancel_rollback();if(e==ESP_OK){confirmed=true;Serial.printf("BOOT_VALID build=%s result=%d\n",TOY_BUILD,e);}else{Serial.printf("BOOT_VALID FAIL %d\n",e);delay(500);ESP.restart();}}
 if(!confirmed&&millis()-bootAt>10000){Serial.println("BLE STARTUP TIMEOUT; unconfirmed reboot");Serial.flush();ESP.restart();}
 if(millis()-lastHeartbeat>2000){lastHeartbeat=millis();report("HEARTBEAT");}
 if(rebootAt&&((int32_t)(millis()-rebootAt)>=0)){Serial.println("OTA REBOOT");Serial.flush();ESP.restart();}
 delay(1);
}
