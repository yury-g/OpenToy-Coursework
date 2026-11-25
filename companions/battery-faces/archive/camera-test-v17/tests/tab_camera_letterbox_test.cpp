#define TAB_CAMERA_HOST_TEST
#include "../firmware/FaceMonitor/TabCamera.h"
#include <cassert>
#include <vector>
int main(){std::vector<uint16_t> image(1280*720,0xf800);uint8_t pixels[4800];assert(tabcamera_detail::letterbox(image.data(),image.size(),pixels,sizeof(pixels)));for(int y=0;y<60;y++)for(int x=0;x<80;x++)assert(pixels[y*80+x]==(y>=7&&y<52?0xe0:0));image.assign(image.size(),0x07e0);assert(tabcamera_detail::letterbox(image.data(),image.size(),pixels,sizeof(pixels)));assert(pixels[7*80]==0x1c);image.assign(image.size(),0x001f);assert(tabcamera_detail::letterbox(image.data(),image.size(),pixels,sizeof(pixels)));assert(pixels[7*80]==3);assert(!tabcamera_detail::letterbox(image.data(),100,pixels,sizeof(pixels)));}
