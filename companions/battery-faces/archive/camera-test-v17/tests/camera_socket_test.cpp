#include "../firmware/FeatherFace/CameraSocket.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <chrono>
#include <cassert>
#include <cstring>
#include <vector>
uint32_t nowMs(){return uint32_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
int main(){alarm(3);int pair[2];int listener=socket(AF_INET,SOCK_STREAM,0);assert(listener>=0);sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);assert(bind(listener,(sockaddr*)&address,sizeof(address))==0);assert(listen(listener,1)==0);socklen_t length=sizeof(address);assert(getsockname(listener,(sockaddr*)&address,&length)==0);pair[0]=socket(AF_INET,SOCK_STREAM,0);assert(connect(pair[0],(sockaddr*)&address,sizeof(address))==0);pair[1]=accept(listener,nullptr,nullptr);assert(pair[1]>=0);close(listener);uint8_t data[8]={1,2,3,4,5,6,7,8},out[8]={};auto pause=[](){usleep(1000);};assert(writeCameraSocket(pair[0],data,8,nowMs(),100,nowMs,pause));assert(read(pair[1],out,8)==8&&!memcmp(data,out,8));int size=1024;setsockopt(pair[0],SOL_SOCKET,SO_SNDBUF,&size,sizeof(size));std::vector<uint8_t> large(1024*1024);auto start=nowMs();assert(!writeCameraSocket(pair[0],large.data(),large.size(),start,25,nowMs,pause));assert(uint32_t(nowMs()-start)<250);close(pair[0]);close(pair[1]);}
