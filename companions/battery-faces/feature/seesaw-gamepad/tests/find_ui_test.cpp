#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>
#include "../firmware/FeatherFace/MonitorPacket.h"
struct IPAddress {IPAddress(int,int,int,int){}};
constexpr unsigned MONITOR_PORT=4242;
struct {std::string text;void printf(const char *fmt,...){char line[2048];va_list ap;va_start(ap,fmt);vsnprintf(line,sizeof(line),fmt,ap);va_end(ap);text+=line;}} Serial;
struct Canvas {struct Rect{int x,y,w,h;};std::vector<Rect> fills;struct Round{int x,y,w,h,r,color;};std::vector<Round> rounds;std::string text;
 void drawRoundRect(int x,int y,int w,int h,int r,int color){rounds.push_back({x,y,w,h,r,color});}
 void setTextSize(int){}void setCursor(int,int){}void setTextColor(int){}void fillRect(int x,int y,int w,int h,int){fills.push_back({x,y,w,h});}void print(const char *s){text+=s;}
};
#include "../firmware/FaceMonitor/FindUi.h"
int main(){
 FindUi ui;ui.control.boot=42;MonitorReceiver receiver;
 for(unsigned i=0;i<FACE_COUNT;i++)if(faceEnabled(i)){receiver.peers[i].seen=true;receiver.peers[i].receivedAt=100;}
 accessory::Status s;s.targetBoot=10;s.sequence=1;s.flags=accessory::CAPABLE|accessory::READY;assert(ui.control.peers[0].receive(s,100));s.target=1;assert(ui.control.peers[1].receive(s,100));
 auto blue=tileRect(0,8),green=tileRect(1,8),pink=tileRect(2,8);
 assert(!ui.tap(blue.x+blue.w/2,blue.y+blue.h/2,100,false,receiver)); // Race/Log/Demo never start finds
 assert(!ui.tap(pink.x+pink.w/2,pink.y+pink.h/2,100,true,receiver));
 assert(ui.tap(blue.x+blue.w/2,blue.y+blue.h/2,100,true,receiver));assert(ui.control.requests[0].phase==accessory::FindController::Starting);
 assert(ui.tap(green.x+green.w/2,green.y+green.h/2,100,true,receiver));assert(ui.control.requests[1].phase==accessory::FindController::Starting);
 assert(ui.tap(100,25,200,false,receiver)); // global stop remains effective on every page
 assert(ui.control.requests[0].phase==accessory::FindController::Stopping&&ui.control.requests[1].phase==accessory::FindController::Stopping);
 FindUi activity;accessory::Status a;a.targetBoot=22;a.sequence=1;a.flags=accessory::CAPABLE|accessory::READY|accessory::LIVE_IO|accessory::BUTTON_READY|accessory::BUTTON_DOWN|accessory::BREATHING|accessory::MOTOR_ACTIVE|accessory::OUTLINE_VISIBLE|accessory::OUTLINE_ACTIVE|accessory::ioDriveBits(19);assert(activity.control.peers[0].receive(a,100));
 Canvas canvas;activity.card(canvas,0,100);assert(canvas.text.find("BREATH19 / Q:DOWN")!=std::string::npos);assert(canvas.fills.size()==3);assert(canvas.fills[0].x==24&&canvas.fills[0].y==212&&canvas.fills[0].w==256&&canvas.fills[0].h==22);assert(canvas.fills[2].y==234&&canvas.fills[2].h==4&&canvas.fills[2].w==256*19/127);
 assert(canvas.rounds.size()==2);auto border=canvas.rounds[0];assert(border.x==16&&border.y==74&&border.w==270&&border.h==134&&border.r==18&&border.color==FACE_INKS[0]);border=canvas.rounds[1];assert(border.x==17&&border.y==75&&border.w==268&&border.h==132&&border.r==17&&border.color==FACE_INKS[0]);
 auto outlined=activity.activity(0,100);assert(outlined.outline&&outlined.outlineActive);
 Serial.text.clear();activity.report(100);assert(Serial.text.find("io_known=1 mode=breathing drive=19 button_down=1 recent_press=0")!=std::string::npos);
 auto state=activity.activity(0,100);assert(state.known&&state.drive==19&&state.buttonDown);assert(!strcmp(state.text,"BREATH19 / Q:DOWN"));assert(!strcmp(state.mode,"breathing"));
 a.sequence++;a.flags=accessory::LIVE_IO|accessory::BUTTON_READY|accessory::RECENT_PRESS|accessory::HELD|accessory::MOTOR_ACTIVE|accessory::OUTLINE_VISIBLE|accessory::ioDriveBits(40);assert(activity.control.peers[0].receive(a,200));state=activity.activity(0,200);assert(!strcmp(state.text,"HOLD40 / Q:PRESS")&&state.recentPress);
 Canvas afterglow;activity.card(afterglow,0,200);assert(afterglow.rounds.size()==2&&afterglow.rounds[0].color==0x4208&&afterglow.rounds[1].color==0x4208);state=activity.activity(0,200);assert(state.outline&&!state.outlineActive);
 state=activity.activity(0,1700);assert(!state.recentPress&&!strcmp(state.text,"HOLD40 / Q:UP"));
 state=activity.activity(0,5200);assert(!state.known&&state.drive==0&&!state.buttonDown&&!state.recentPress&&!strcmp(state.mode,"unknown")&&!strcmp(state.text,"ACCESSORY OFFLINE"));
 Canvas offline;activity.card(offline,0,5200);assert(offline.text.find("ACCESSORY OFFLINE")!=std::string::npos);assert(offline.text.find("HOLD40")==std::string::npos&&offline.fills.size()==2&&offline.rounds.empty());assert(!state.outline&&!state.outlineActive);
 Serial.text.clear();activity.report(5200);assert(Serial.text.find("io_known=0 mode=unknown drive=0 button_down=0 recent_press=0")!=std::string::npos);
 a.sequence++;a.flags=accessory::CAPABLE|accessory::READY;assert(activity.control.peers[0].receive(a,5300));state=activity.activity(0,5300);assert(!state.known&&state.drive==0&&!strcmp(state.text,"ACCESSORY --"));
 Canvas legacy;activity.card(legacy,0,5300);assert(legacy.rounds.empty());
 a.sequence++;a.flags=accessory::LIVE_IO|accessory::MOTOR_ACTIVE|accessory::ioDriveBits(20);assert(activity.control.peers[0].receive(a,5400));state=activity.activity(0,5400);assert(!strcmp(state.text,"BUZZ20 / Q:--"));
 Canvas absent;activity.card(absent,0,5400);assert(absent.rounds.empty());Serial.text.clear();activity.report(5400);assert(Serial.text.find("outline=0 outline_active=0")!=std::string::npos);
 FindUi stale;assert(!stale.tap(blue.x+blue.w/2,blue.y+blue.h/2,100,true,receiver));
}
