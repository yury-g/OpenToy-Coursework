#include "../firmware/FeatherFace/CameraProtocol.h"
#include "../firmware/FeatherFace/CameraSnapshot.h"
#include "../firmware/FeatherFace/CameraView.h"
#include <cassert>
int main(){
 uint8_t pixels[facecamera::FRAME_BYTES]={},packet[facecamera::PACKET_BYTES];
 facecamera::FrameAssembler teal(0),tab(1);
 for(unsigned i=0;i<facecamera::CHUNK_COUNT;i++){
  auto n=facecamera::buildChunk(pixels,1,1000,i,packet,sizeof(packet),123,1);
  assert(!teal.receive(packet,n,100));tab.receive(packet,n,100);
 }
 assert(!teal.fresh(100)&&tab.fresh(100));
 assert(facecamera::sourceAllowed(facecamera::CAMERA_MAC,0));
 assert(!facecamera::sourceAllowed(facecamera::CAMERA_MAC,1));
 assert(facecamera::sourceAllowed(facecamera::RELAY_MAC,1));
 assert(!facecamera::sourceAllowed(facecamera::RELAY_MAC,0));
 CameraSnapshotMeta m{7,1000,123,0,40,1},out{};uint8_t header[32];encodeCameraSnapshot(header,m);
 assert(decodeCameraSnapshot(header,32,out)&&out.source==1);header[5]=2;assert(!decodeCameraSnapshot(header,32,out));
 CameraView autoView;autoView.followDefault(0);assert(!autoView.active);autoView.followDefault(3);assert(autoView.active&&autoView.source==0);assert(autoView.expire(2));autoView.followDefault(1);assert(autoView.active);unsigned ownPage=1;autoView.nextInfo(ownPage,3);assert(!autoView.active&&ownPage==0);autoView.followDefault(3);assert(!autoView.active);
 CameraView v;unsigned page=0;v.nextInfo(page,3);assert(page==1&&!v.active);
 v.nextInfo(page,3);assert(v.active&&v.source==0);v.nextInfo(page,3);assert(v.active&&v.source==1);
 assert(!v.expire(2));assert(v.expire(1)&&!v.active); // Losing selected source must not borrow another's freshness.
 v.select(2,1);assert(v.active&&v.source==1);v.nextInfo(page,3);assert(!v.active&&page==0);
 v.cycle(3,1);assert(v.active&&v.source==0);v.cycle(3,1);assert(v.source==1);v.cycle(3,1);assert(!v.active);
 v.cycle(2,-1);assert(v.active&&v.source==1);v.cycle(2,-1);assert(!v.active);
}
