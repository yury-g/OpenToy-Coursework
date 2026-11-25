// SPDX-License-Identifier: MIT
#pragma once
struct CameraView {
 bool active=false,defaultWanted=true;unsigned source=0;
 void resetDefault(unsigned mask){defaultWanted=true;active=false;followDefault(mask);}
 void followDefault(unsigned mask){if(defaultWanted&&!active&&(mask&1)){source=0;active=true;}}
 void cycle(unsigned freshMask,int direction=1){
  // Carousel consists of faces, then each live source. Skip missing cameras.
  defaultWanted=false;int slot=active?int(source)+1:0;
  for(unsigned i=0;i<3;i++){slot=(slot+(direction<0?2:1))%3;if(!slot){leave();return;}if(freshMask&(1u<<(slot-1))){source=unsigned(slot-1);active=true;return;}}
  leave();
 }
 void nextInfo(unsigned &page,unsigned freshMask){if(active&&defaultWanted){leave();page=0;}else if(active){cycle(freshMask);if(!active)page=0;}else if(page==0)page=1;else if(page==1){cycle(freshMask);if(!active)page=0;}else page=0;}
 bool expire(unsigned freshMask){if(active&&!(freshMask&(1u<<source))){active=false;return true;}return false;}
 void select(unsigned freshMask,unsigned requested=0){defaultWanted=false;source=requested<2?requested:0;active=requested<2&&(freshMask&(1u<<source));}
 void leave(){defaultWanted=false;active=false;}
};
