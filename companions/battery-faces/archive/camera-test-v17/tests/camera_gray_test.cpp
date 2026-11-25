#include "../firmware/FeatherFace/CameraProtocol.h"
#include <vector>
#include <cassert>
using namespace facecamera;
int main(){std::vector<uint8_t> src(320*240*2,255);uint8_t out[4800],p[PACKET_BYTES];assert(downsampleGray2(src.data(),src.size(),320,240,out,sizeof(out)));for(auto v:out)assert(v==255);src.assign(src.size(),0);assert(downsampleGray2(src.data(),src.size(),320,240,out,sizeof(out)));for(auto v:out)assert(v==0);out[0]=0x1b;assert(pixel565(out,2,0,0)==0);assert(pixel565(out,2,1,0)==0x52aa);assert(pixel565(out,2,2,0)==0xad55);assert(pixel565(out,2,3,0)==0xffff);
 FrameAssembler a;for(unsigned i=0;i<CHUNK_COUNT;i++){auto n=buildChunk(out,1,10,i,p,sizeof(p),123,0,2);a.receive(p,n,100);}assert(a.format()==2&&a.fresh(100)&&a.width()==160&&a.height()==120);assert(buildChunk(out,2,20,0,p,sizeof(p),123,0,3)==0);
}
