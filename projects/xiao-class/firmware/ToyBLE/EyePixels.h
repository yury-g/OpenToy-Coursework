// SPDX-License-Identifier: MIT
#pragma once
#include "FaceProtocol.h"
inline int eyeWhiteY(const FaceState &s){return 16+s.y*14/100;}
inline bool eyeWhitePixel(Expression expression,unsigned eye,int x,int y){
 if(x<0||x>=28||y<0||y>=32)return false;
 if(expression==SURPRISED){int dx=x-14,dy=y-16;return dx*dx+dy*dy<=196;}
 int dx=x<8?x-8:x>19?x-19:0,dy=y<8?y-8:y>23?y-23:0;
 if(dx*dx+dy*dy>64)return false;
 if(expression==HAPPY&&y<16)return false;
 if(expression==ANGRY&&y*28<(eye?28-x:x)*12)return false;
 return true;
}
inline bool eyeLayerPixel(const FaceState &s,unsigned eye,int x,int y){
 if(x<0||x>=28||y<0||y>=32)return false;
 unsigned lid=eye?s.right:s.left;
 if(lid==100)return y==16;
 if(!eyeWhitePixel(s.expression,eye,x,y))return false;
 int dx=x-(14+s.pupilX*6/100),dy=y-(16+s.pupilY*7/100);
 if(s.pupils&&dx*dx+dy*dy<=16)return false;
 unsigned height=32*lid/100;
 if(height&&(s.bottomLids?y>=32-(int)height:y<(int)height))return false;
 return true;
}
