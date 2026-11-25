// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
inline bool freshCameraCapture(int64_t now,int64_t seconds,int64_t micros){
 if(now<0||seconds<0||micros<0||micros>=1000000||seconds>INT64_MAX/1000000-1)return false;
 int64_t captured=seconds*1000000+micros;return captured<=now&&now-captured<=500000;
}
