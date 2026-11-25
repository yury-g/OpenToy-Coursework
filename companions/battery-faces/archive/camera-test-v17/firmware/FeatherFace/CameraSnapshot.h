// SPDX-License-Identifier: MIT
#pragma once
#include "CameraProtocol.h"
struct CameraSnapshotMeta{uint32_t sequence,uptime,session,checksum,age;unsigned source=0,format=1;};
inline void encodeCameraSnapshot(uint8_t *b,const CameraSnapshotMeta &m){
 memcpy(b,"FCAS",4);facecamera::write32(b+4,1|(m.source<<8)|((m.format-1)<<16));facecamera::write32(b+8,m.sequence);facecamera::write32(b+12,m.uptime);facecamera::write32(b+16,m.session);facecamera::write32(b+20,m.checksum);facecamera::write32(b+24,m.age);facecamera::write32(b+28,facecamera::FRAME_BYTES);
}
inline bool decodeCameraSnapshot(const void *data,size_t size,CameraSnapshotMeta &m){
 if(!data||size!=32)return false;auto b=(const uint8_t*)data;
 if(memcmp(b,"FCAS",4)||(facecamera::read32(b+4)&0xfffefeffu)!=1||facecamera::read32(b+28)!=facecamera::FRAME_BYTES||facecamera::read32(b+24)>=facecamera::FRAME_TTL_MS)return false;
 m={facecamera::read32(b+8),facecamera::read32(b+12),facecamera::read32(b+16),facecamera::read32(b+20),facecamera::read32(b+24),unsigned(b[5]),unsigned(b[6])+1};return true;
}
