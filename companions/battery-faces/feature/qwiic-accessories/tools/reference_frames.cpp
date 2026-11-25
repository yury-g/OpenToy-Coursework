#include "../firmware/FeatherFace/DoodlePixels.h"
#include <cstdio>
#include <string>
#include <iostream>
int main(){std::string line;while(std::getline(std::cin,line)){FaceState s;if(line.rfind("doodle ",0)||!parseDoodle(line.c_str()+7,s))return 1;uint8_t buffer[1024];drawDoodle(doodleTarget(s),s.expression,buffer,0,0,0);uint32_t h=2166136261U;for(uint8_t b:buffer)h=(h^b)*16777619U;printf("%08x\n",h);}return 0;}
