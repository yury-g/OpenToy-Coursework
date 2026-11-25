// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
#include <string.h>
// Bound every segment/trailer to the authenticated received byte count before
// invoking the SDK validator, which can read the entire inactive partition.
template<class Reader> bool imageBounds(uint32_t total,Reader read){
 uint8_t h[24];if(total<24||!read(0,h,24)||h[0]!=0xe9||h[1]<1||h[1]>16||h[23]>1)return false;
 uint32_t cursor=24;
 for(unsigned i=0;i<h[1];i++){
  uint8_t s[8];if(cursor>total||total-cursor<8||!read(cursor,s,8))return false;
  uint32_t bytes=(uint32_t)s[4]|(uint32_t)s[5]<<8|(uint32_t)s[6]<<16|(uint32_t)s[7]<<24;
  cursor+=8;if(bytes>total-cursor)return false;cursor+=bytes;
 }
 // Image checksum occupies the last byte of the next 16-byte boundary.
 uint32_t trailer=16-cursor%16;if(h[23])trailer+=32;
 return cursor<=total&&trailer<=total-cursor;
}
