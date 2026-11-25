// SPDX-License-Identifier: MIT
#pragma once
#include <cstdarg>
#include <cstdio>
#include <cstddef>
namespace feather_water {
// One producer/writer: the Arduino loop (including setup). Drop new reports
// while a previous one is pending; never wait for USB to make room.
class Telemetry {
 char bytes[1024];size_t used=0,sent=0;
public:
 bool pending() const{return sent<used;}
 void append(const char* format,...) {
  va_list args;va_start(args,format);
  int n=vsnprintf(bytes+used,sizeof(bytes)-used,format,args);va_end(args);
  if(n>0&&size_t(n)<sizeof(bytes)-used)used+=size_t(n);
 }
 template<class Port> void pump(Port& port) {
  if(!port){used=sent=0;return;}
  if(!pending())return;
  int space=port.availableForWrite();if(space<=0)return;
  size_t n=used-sent;if(n>size_t(space))n=size_t(space);
  // No other application writer may consume space between this check/write.
  sent+=port.write(reinterpret_cast<const unsigned char*>(bytes+sent),n);
  if(sent==used)used=sent=0;
 }
};
}
