// SPDX-License-Identifier: MIT
#pragma once
#include "Tracking.h"
#include "Strokes.h"
#include <cstddef>
namespace copy_face {
constexpr int previewWidth=320,previewHeight=180;
struct PreviewImage {
 const uint16_t* pixels=nullptr;unsigned width=0,height=0,stride=0;
 size_t pixelCount=0;uint32_t captured=0;
};
inline bool validImage(const PreviewImage& im,uint32_t now) {
 return im.pixels&&im.width>0&&im.height>0&&im.width<=4096&&im.height<=4096&&
        im.stride>=im.width&&im.stride<=4096&&
        im.pixelCount>=size_t(im.height-1)*im.stride+im.width&&uint32_t(now-im.captured)<=250;
}
inline Point mapPreview(Point p,const PreviewImage& im,bool mirror) {
 if(!im.width||!im.height)return {};
 float scale=std::min(float(previewWidth)/im.width,float(previewHeight)/im.height);
 float w=im.width*scale,h=im.height*scale;
 return {(previewWidth-w)/2+(mirror?1-p.x:p.x)*w,(previewHeight-h)/2+p.y*h};
}
struct OverlaySelection {bool tracked=false;Detection face;};
inline OverlaySelection overlaySelection(const PreviewImage& im,const Tracker& tracker,uint32_t now) {
 OverlaySelection out;uint32_t captured;
 if(validImage(im,now)&&tracker.selected(out.face,captured,now)&&captured==im.captured)out.tracked=true;
 return out;
}
inline void renderDeveloper(const PreviewImage& im,const Tracker& tracker,uint32_t now,bool mirror,uint16_t* out) {
 std::fill_n(out,previewWidth*previewHeight,0x1082);
 if(!validImage(im,now))return;
 float scale=std::min(float(previewWidth)/im.width,float(previewHeight)/im.height);
 int w=int(im.width*scale),h=int(im.height*scale),left=(previewWidth-w)/2,top=(previewHeight-h)/2;
 for(int y=0;y<h;y++)for(int x=0;x<w;x++) {
  unsigned sx=std::min(im.width-1,unsigned(x/scale)),sy=std::min(im.height-1,unsigned(y/scale));
  if(mirror)sx=im.width-1-sx;
  out[(top+y)*previewWidth+left+x]=im.pixels[sy*im.stride+sx];
 }
 auto selected=overlaySelection(im,tracker,now);if(!selected.tracked)return;
 const auto& d=selected.face;
 auto line=[&](Point a,Point b,float r=1){stroke(out,previewWidth,previewHeight,mapPreview(a,im,mirror),mapPreview(b,im,mirror),r,0x07e0);};
 line({d.x,d.y},{d.x+d.w,d.y});line({d.x+d.w,d.y},{d.x+d.w,d.y+d.h});
 line({d.x+d.w,d.y+d.h},{d.x,d.y+d.h});line({d.x,d.y+d.h},{d.x,d.y});
 for(const auto& e:d.eyes)if(validEye(e))for(unsigned i=0;i<6;i++)line(e.contour[i],e.contour[(i+1)%6]);
 if(d.keypointsValid)for(auto point:d.keypoints)if(imagePoint(point))line(point,point,2);
 auto shape=normalizeLips(d.lips);
 if(shape.valid)for(unsigned i=0;i<lipPoints;i++) {
  if(i){line(d.lips.upper[i-1],d.lips.upper[i],.75f);line(d.lips.lower[i-1],d.lips.lower[i],.75f);}
  if(contact(shape,i)) {
   Point p{(d.lips.upper[i].x+d.lips.lower[i].x)/2,(d.lips.upper[i].y+d.lips.lower[i].y)/2};
   line(p,p,2); // Dots distinguish estimated contact from contour edges.
  }
 }
}
// Generated fixture, never camera imagery. Kept in RAM only.
inline void syntheticCamera(const Frame& frame,uint16_t* out) {
 for(int y=0;y<previewHeight;y++)for(int x=0;x<previewWidth;x++)out[y*previewWidth+x]=((x/20+y/20)%2)?0x2104:0x18e3;
 if(!frame.count)return;
 const auto& d=frame.faces[0];
 float cx=(d.x+d.w/2)*previewWidth,cy=(d.y+d.h/2)*previewHeight,rx=d.w*previewWidth/2,ry=d.h*previewHeight/2;
 for(int y=0;y<previewHeight;y++)for(int x=0;x<previewWidth;x++) {
  float u=(x-cx)/rx,v=(y-cy)/ry;if(u*u+v*v<1)out[y*previewWidth+x]=0x9cd3;
 }
 auto p=[](Point v){return Point{v.x*previewWidth,v.y*previewHeight};};
 for(const auto& e:d.eyes)for(unsigned i=0;i<6;i++)stroke(out,previewWidth,previewHeight,p(e.contour[i]),p(e.contour[(i+1)%6]),1,0x2104);
 for(unsigned i=1;i<lipPoints;i++) {
  stroke(out,previewWidth,previewHeight,p(d.lips.upper[i-1]),p(d.lips.upper[i]),1,0x2104);
  stroke(out,previewWidth,previewHeight,p(d.lips.lower[i-1]),p(d.lips.lower[i]),1,0x2104);
 }
 stroke(out,previewWidth,previewHeight,{cx,cy-3},{cx-3,cy+13},1,0x632c);
}
}
