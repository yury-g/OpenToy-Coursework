// SPDX-License-Identifier: MIT
#pragma once
#include "CameraProtocol.h"
#include "PeerBattery.h"
inline void buildCameraControl(uint8_t *p,unsigned format){memcpy(p,"FCFG",4);p[4]=1;p[5]=uint8_t(format);p[6]=p[7]=0;}
inline bool decodeCameraControl(const uint8_t *mac,const uint8_t *p,size_t n,unsigned &format){
 if(!mac||!p||n!=8||memcmp(p,"FCFG",4)||p[4]!=1||(p[5]!=1&&p[5]!=2)||p[6]||p[7])return false;
 for(unsigned i=0;i<FACE_COUNT;i++)if(faceEnabled(i)&&i!=7&&!memcmp(mac,FACE_MACS[i],6)){format=p[5];return true;}return false;
}
