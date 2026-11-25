// SPDX-License-Identifier: MIT
// Export the pure eye renderer as a 128x64 PBM for documentation, without hardware.
#include "../projects/xiao-class/firmware/ToyBLE/EyePixels.h"
#include "../projects/xiao-class/firmware/ToyBLE/DoodlePixels.h"
#include <cstdio>
#include <cstring>
int main(int argc,char **argv){
 FaceState s;
 if(argc!=2)return 2;
 bool pixels[64][128]={};
 if(!strncmp(argv[1],"doodle ",7)&&parseDoodle(argv[1]+7,s)){
  uint8_t buffer[1024];drawDoodle(doodleTarget(s),s.expression,buffer,0,0,0);
  for(unsigned y=0;y<64;y++)for(unsigned x=0;x<128;x++)pixels[y][x]=(buffer[(y/8)*128+x]>>(y%8))&1;
 }else {
 if(strncmp(argv[1],"eyes ",5)||!parseEyes(argv[1]+5,s))return 2;
 for(unsigned eye=0;eye<2;eye++)for(int py=0;py<32;py++)for(int px=0;px<28;px++){
  int x=(eye?78:22)+s.x*10/100+px,y=eyeWhiteY(s)+py;
  if(x>=0&&x<128&&y>=0&&y<64&&eyeLayerPixel(s,eye,px,py))pixels[y][x]=true;
 }
 }
 puts("P1\n128 64");
 for(auto &row:pixels){for(bool p:row)printf("%d ",p);puts("");}
}
