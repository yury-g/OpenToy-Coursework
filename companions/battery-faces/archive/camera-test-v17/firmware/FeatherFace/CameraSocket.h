// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <sys/socket.h>
#include <errno.h>
// Nonblocking sends keep a real wall-clock bound even under TCP backpressure.
// NetworkClient::write has internal multi-second retries outside caller control.
template<class Now,class Pause> bool writeCameraSocket(int fd,const uint8_t *data,size_t size,uint32_t start,uint32_t budget,Now now,Pause pause){
 if(fd<0||(!data&&size))return false;size_t at=0;
 while(at<size&&uint32_t(now()-start)<budget){
  size_t n=size-at;if(n>1024)n=1024;
  int sent=::send(fd,data+at,n,MSG_DONTWAIT);
  if(sent>0)at+=size_t(sent);
  else if(sent<0&&errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR)return false;
  else pause();
 }
 return at==size;
}
