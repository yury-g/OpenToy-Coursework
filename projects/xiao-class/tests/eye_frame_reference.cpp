#include "../firmware/ToyBLE/EyePixels.h"
#include <cstdio>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <initializer_list>
uint32_t frameHash(const FaceState &s){
 uint8_t buffer[1024]={};
 for(unsigned eye=0;eye<2;eye++)for(int py=0;py<32;py++)for(int px=0;px<28;px++){
  int x=(eye?78:22)+s.x*10/100+px,y=eyeWhiteY(s)+py;
  if(x>=0&&x<128&&y>=0&&y<64&&eyeLayerPixel(s,eye,px,py))buffer[(y/8)*128+x]|=1<<(y%8);
 }
 uint32_t hash=2166136261UL;for(uint8_t b:buffer){hash^=b;hash*=16777619UL;}return hash;
}
int main(){
 for(const char *args:{"0 0 0 0 0 0 bottom neutral","0 -100 0 0 0 0 bottom neutral","0 100 0 0 0 0 bottom neutral","0 0 -100 0 0 0 bottom neutral","0 0 100 0 0 0 bottom neutral","0 0 0 -100 0 0 bottom neutral","0 0 0 100 0 0 bottom neutral","0 0 0 0 100 0 bottom neutral","0 0 0 0 0 100 bottom neutral","0 0 0 0 50 50 bottom neutral","0 0 0 0 50 50 top neutral","0 0 0 0 100 100 bottom neutral","0 -50 100 100 100 50 bottom happy","0 0 0 0 0 0 bottom angry","0 0 0 0 0 0 bottom surprised"}){
  FaceState s;assert(parseEyes(args,s));printf("eyes %s\t%08x\n",args,frameHash(s));
 }
}
