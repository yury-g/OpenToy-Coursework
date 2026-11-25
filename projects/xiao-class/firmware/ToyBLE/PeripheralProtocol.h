// SPDX-License-Identifier: MIT
#pragma once
#include "Protocol.h"
#include "FaceProtocol.h"
enum Screen {EYES,CLOSED,TEST_SCREEN,SCREEN_OFF};
struct PeripheralState {Screen screen=EYES;unsigned contrast=127,threshold=80000;bool touch=true;FaceState face;};
inline const char *screenName(Screen s){return s==EYES?"eyes":s==CLOSED?"closed":s==TEST_SCREEN?"test":"off";}
inline Result peripheralCommand(const char *cmd,PeripheralState &s){
 if(!strncmp(cmd,"eyes ",5)){if(!parseEyes(cmd+5,s.face))return INVALID;s.screen=EYES;return ACCEPTED;}
 if(!strncmp(cmd,"face ",5)){if(!parseFace(cmd+5,s.face))return INVALID;s.screen=EYES;return ACCEPTED;}
 if(!strcmp(cmd,"screen eyes")){s.screen=EYES;return ACCEPTED;}
 if(!strcmp(cmd,"screen closed")){s.screen=CLOSED;return ACCEPTED;}
 if(!strcmp(cmd,"screen test")){s.screen=TEST_SCREEN;return ACCEPTED;}
 if(!strcmp(cmd,"screen off")){s.screen=SCREEN_OFF;return ACCEPTED;}
 if(!strncmp(cmd,"contrast ",9))return boundedNumber(cmd+9,0,255,s.contrast)?ACCEPTED:INVALID;
 if(!strncmp(cmd,"touch threshold ",16))return boundedNumber(cmd+16,1000,2000000,s.threshold)?ACCEPTED:INVALID;
 if(!strcmp(cmd,"touch on")){s.touch=true;return ACCEPTED;}
 if(!strcmp(cmd,"touch off")){s.touch=false;return ACCEPTED;}
 return INVALID;
}
