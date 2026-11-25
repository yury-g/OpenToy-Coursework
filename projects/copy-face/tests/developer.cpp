// SPDX-License-Identifier: MIT
#include "../core/DeveloperView.h"
#include <cassert>
#include <vector>
#include <cstdio>
using namespace copy_face;
int main() {
 std::vector<uint16_t> source(640*360,0xf800),output(previewWidth*previewHeight+2,0xdead);
 PreviewImage image{source.data(),640,360,640,source.size(),100};
 Tracker tracker;renderDeveloper(image,tracker,100,false,output.data()+1);
 assert(output.front()==0xdead&&output.back()==0xdead);
 assert(output[1+previewWidth*90+160]==0xf800);
 renderDeveloper(image,tracker,400,false,output.data()+1);assert(output[1+previewWidth*90+160]!=0xf800);
 image.pixelCount=5;renderDeveloper(image,tracker,100,false,output.data()+1);assert(output[1+previewWidth*90+160]!=0xf800);
 image.pixelCount=source.size();
 Frame f;f.count=1;auto& d=f.faces[0];d.x=.2f;d.y=.1f;d.w=.4f;d.h=.8f;d.confidence=.9f;
 for(unsigned now:{10,50,100}){f.captured=now;tracker.observe(f,now);}
 assert(overlaySelection(image,tracker,100).tracked);
 image.captured=101;assert(!overlaySelection(image,tracker,101).tracked);
 image.captured=100;auto a=mapPreview(Point{.2f,.3f},image,false),b=mapPreview(Point{.2f,.3f},image,true);
 assert(std::fabs(a.x+b.x-previewWidth)<.01f && a.y==b.y);
 puts("developer view: raw pixels, freshness, buffer bounds, matching timestamps and mirror passed");
}
