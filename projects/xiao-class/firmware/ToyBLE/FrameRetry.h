// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
struct FrameRetry {
 bool failed=false;uint32_t lastFailure=0;
 bool allowed(uint32_t now)const{return !failed||(uint32_t)(now-lastFailure)>=500;}
 void completed(bool ok,uint32_t now){failed=!ok;if(!ok)lastFailure=now;}
};
