// SPDX-License-Identifier: MIT
#pragma once
struct TileRect {int x,y,w,h;};
// Choose columns to maximize the uniformly scaled 616x304 card area.
inline TileRect tileRect(unsigned index,unsigned count){
 if(!count || index>=count)return {16,72,0,0};
 unsigned best=1;float bestScale=0;
 for(unsigned cols=1;cols<=count;cols++){
  unsigned rows=(count+cols-1)/cols;
  float sx=(1248.0f-(cols-1)*16)/cols/616,sy=(632.0f-(rows-1)*16)/rows/304;
  float scale=sx<sy?sx:sy;if(scale>bestScale){bestScale=scale;best=cols;}
 }
 unsigned rows=(count+best-1)/best;
 int w=(1248-int(best-1)*16)/int(best),h=(632-int(rows-1)*16)/int(rows);
 unsigned row=index/best,col=index%best,inRow=(row==rows-1?count-row*best:best);
 int offset=(1248-(int(inRow)*w+int(inRow-1)*16))/2;
 return {16+offset+int(col)*(w+16),72+int(row)*(h+16),w,h};
}
