// SPDX-License-Identifier: MIT
#include "../firmware/FeatherFace/CameraProtocol.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
using namespace facecamera;

// Hand-built wire fixtures, independent of the encoder. CRCs cross-checked with zlib.
static size_t fixture(uint8_t *p, unsigned index=0, uint32_t seq=42,
                      uint32_t uptime=2000, uint8_t fill=0xa5,uint32_t session=0) {
 memset(p,0,PACKET_BYTES);
 p[0]='F';p[1]='C';p[2]='A';p[3]='M';p[4]=1;p[5]=index;p[6]=22;p[7]=1;
 for(unsigned i=0;i<4;i++){p[8+i]=seq>>(8*i);p[12+i]=uptime>>(8*i);}
 const uint32_t crc=fill==0xa5?0x36e48945u:0xe2cd75e3u;
 for(unsigned i=0;i<4;i++)p[16+i]=crc>>(8*i);
 p[20]=80;p[21]=60;const size_t n=index==21?180:220;p[22]=n;p[23]=0;
 write32(p+24,session);memset(p+28,fill,n);return 28+n;
}
static void complete(FrameAssembler &a,uint32_t now=100,uint32_t seq=42,
                     uint32_t uptime=2000,uint8_t fill=0xa5,uint32_t session=0) {
 uint8_t p[PACKET_BYTES];
 for(unsigned i=0;i<22;i++)assert(a.receive(p,fixture(p,i,seq,uptime,fill,session),now+i)==(i==21));
}
static void encoding_and_color_contract() {
 assert(crc32(reinterpret_cast<const uint8_t*>("123456789"),9)==0xcbf43926u);
 uint8_t pixels[FRAME_BYTES],p[PACKET_BYTES],gold[PACKET_BYTES];memset(pixels,0xa5,sizeof pixels);
 assert(buildChunk(pixels,42,2000,0,p,sizeof p)==fixture(gold));assert(!memcmp(p,gold,248));
 assert(buildChunk(pixels,42,2000,21,p,sizeof p)==fixture(gold,21));assert(!memcmp(p,gold,208));
 assert(buildChunk(pixels,1,1,22,p,sizeof p)==0);
 assert(buildChunk(pixels,1,1,0,p,247)==0);assert(buildChunk(nullptr,1,1,0,p,sizeof p)==0);
 assert(buildChunk(pixels,1,1,0,nullptr,sizeof p)==0);
 assert(rgb565beTo332(0xf8,0)==0xe0);assert(rgb565beTo332(7,0xe0)==0x1c);
 assert(rgb565beTo332(0,0x1f)==3);assert(rgb565beTo332(0xff,0xff)==255);
 assert(rgb332To565(0xe0)==0xf800);assert(rgb332To565(0x1c)==0x07e0);
 assert(rgb332To565(3)==0x001f);assert(rgb332To565(255)==0xffff);
}
static void publication_is_atomic_and_expires() {
 FrameAssembler a;uint8_t out[FRAME_BYTES];memset(out,0x66,sizeof out);
 assert(!a.fresh(0));assert(!a.copyPixels(out,0));assert(out[0]==0x66);
 complete(a);assert(a.sequence()==42);assert(a.completedAt()==121);assert(a.completedFrames()==1);
 assert(a.copyPixels(out,122));for(auto v:out)assert(v==0xa5);
 uint8_t p[PACKET_BYTES];assert(!a.receive(p,fixture(p,0,43,4000,0x5a),200));
 assert(a.copyPixels(out,201));for(auto v:out)assert(v==0xa5);
 assert(a.fresh(5120));assert(!a.fresh(5121));memset(out,0x77,sizeof out);
 assert(!a.copyPixels(out,5121));assert(out[0]==0x77);assert(!a.copyPixels(nullptr,300));
}
static void rejects_bad_headers_and_exact_lengths() {
 uint8_t p[PACKET_BYTES+1],out[FRAME_BYTES];
 const unsigned fields[]={0,4,6,7,20,21,22,23};
 for(unsigned field:fields){FrameAssembler a;size_t n=fixture(p);p[field]^=1;assert(!a.receive(p,n,100));assert(!a.copyPixels(out,100));assert(a.rejectedPackets()==1);}
 for(size_t n=0;n<24;n++){FrameAssembler a;fixture(p);assert(!a.receive(p,n,100));}
 FrameAssembler a;size_t n=fixture(p);assert(!a.receive(nullptr,n,100));
 assert(!a.receive(p,n-1,100));assert(!a.receive(p,n+1,100));
 fixture(p);p[5]=22;assert(!a.receive(p,248,100));
 n=fixture(p,21);assert(!a.receive(p,n,100));
}
static void order_duplicates_crossframe_and_crc() {
 uint8_t p[PACKET_BYTES],out[FRAME_BYTES];FrameAssembler a;
 assert(!a.receive(p,fixture(p,0),100));assert(!a.receive(p,fixture(p,2),101));
 assert(!a.receive(p,fixture(p,2),102));assert(!a.receive(p,fixture(p,1),103));
 for(unsigned i=3;i<22;i++)assert(a.receive(p,fixture(p,i),104+i)==(i==21));
 assert(a.fresh(130));assert(a.completedFrames()==1);
 // Gaps in one repeated copy fill from the next, with reordered duplicates harmless.
 for(unsigned i=0;i<22;i+=2)assert(!a.receive(p,fixture(p,i,43,4000,0x5a),200+i));
 assert(a.copyPixels(out,230)&&out[0]==0xa5);
 for(unsigned i=1;i<22;i+=2)assert(a.receive(p,fixture(p,i,43,4000,0x5a),240+i)==(i==21));
 assert(a.completedFrames()==2&&a.copyPixels(out,270)&&out[0]==0x5a);
 // A wrong whole-frame checksum never publishes and leaves the last good image.
 for(unsigned i=0;i<22;i++){size_t n=fixture(p,i,44,6000,0xa5);if(i==10)p[HEADER_BYTES]^=1;assert(!a.receive(p,n,300+i));}
 assert(a.completedFrames()==2&&a.copyPixels(out,330)&&out[0]==0x5a);
 complete(a,400,45,8000);assert(a.completedFrames()==3);
}
static void old_replay_does_not_abort_newer_partial_or_refresh_age() {
 FrameAssembler a;uint8_t p[PACKET_BYTES],out[FRAME_BYTES];complete(a);
 assert(!a.receive(p,fixture(p,0,43,4000,0x5a),200));
 const uint32_t rejectedBeforeRepeat=a.rejectedPackets();
 for(unsigned i=0;i<22;i++)assert(!a.receive(p,fixture(p,i),201+i));
 assert(a.rejectedPackets()==rejectedBeforeRepeat);
 for(unsigned i=1;i<22;i++)assert(a.receive(p,fixture(p,i,43,4000,0x5a),300+i)==(i==21));
 assert(a.sequence()==43);assert(a.completedFrames()==2);
 for(unsigned i=0;i<22;i++)assert(!a.receive(p,fixture(p,i,41,1000),400+i));
 assert(a.completedAt()==321);assert(!a.fresh(5321));
 // Even after expiry an old frame is not permission to resurrect the source.
 for(unsigned i=0;i<22;i++)assert(!a.receive(p,fixture(p,i,42,2000),5400+i));
 assert(!a.copyPixels(out,5500));
 // A source restart cannot displace a live epoch, but can recover after expiry.
 for(unsigned i=0;i<22;i++)assert(!a.receive(p,fixture(p,i,1,1000),500+i));
 assert(a.sequence()==43);
 complete(a,5600,1,1000,0xa5,1);assert(a.fresh(5700));assert(a.sequence()==1);
 a.resetSource();assert(!a.fresh(5700));complete(a,5800,1,500);assert(a.fresh(5900));
 FrameAssembler wrap;complete(wrap,100,0xffffffffu,3000);complete(wrap,200,0,5000);assert(wrap.sequence()==0);
}
static void pending_timeout_restart_and_millis_wrap() {
 uint8_t p[PACKET_BYTES];FrameAssembler a;
 assert(!a.receive(p,fixture(p,0),100));
 for(unsigned i=1;i<22;i++)assert(!a.receive(p,fixture(p,i),5100+i));assert(!a.fresh(5200));
 // A new chunk zero supersedes an abandoned partial frame.
 assert(!a.receive(p,fixture(p,0,44),5300));complete(a,5400,45);assert(a.sequence()==45);
 FrameAssembler b;complete(b,0xfffffff0u);assert(b.completedAt()==5);assert(b.fresh(5004));assert(!b.fresh(5005));
 FrameAssembler c;assert(!c.receive(p,fixture(p,0),0xfffffff0u));
 for(unsigned i=1;i<22;i++)assert(!c.receive(p,fixture(p,i),5000+i));assert(!c.fresh(5100));
}
static void downsample_validates_length_and_camera_byte_order() {
 uint8_t source[320*240*2]={},out[FRAME_BYTES];memset(out,0x77,sizeof out);
 assert(!downsampleRgb565be(source,sizeof source-1,320,240,out,sizeof out));assert(out[0]==0x77);
 assert(!downsampleRgb565be(source,sizeof source,319,240,out,sizeof out));
 assert(!downsampleRgb565be(source,sizeof source,320,240,out,sizeof out-1));
 assert(!downsampleRgb565be(nullptr,sizeof source,320,240,out,sizeof out));
 assert(!downsampleRgb565be(source,sizeof source,320,240,nullptr,sizeof out));
 // Each sample is the center of its 4x4 source cell, including the bottom right.
 const unsigned positions[]={2*320+2,2*320+6,2*320+10,238*320+318};
 const uint8_t colors[][2]={{0xf8,0},{7,0xe0},{0,0x1f},{0xff,0xff}};
 for(unsigned i=0;i<4;i++){source[positions[i]*2]=colors[i][0];source[positions[i]*2+1]=colors[i][1];}
 assert(downsampleRgb565be(source,sizeof source,320,240,out,sizeof out));
 assert(out[0]==0xe0);assert(out[1]==0x1c);assert(out[2]==3);assert(out[4799]==255);assert(out[3]==0);
}
static void encoder_roundtrip_and_bounded_state() {
 FrameAssembler a;uint8_t pixels[FRAME_BYTES],p[PACKET_BYTES],out[FRAME_BYTES];
 for(unsigned i=0;i<FRAME_BYTES;i++)pixels[i]=uint8_t(i*31+7);
 for(unsigned i=0;i<CHUNK_COUNT;i++){size_t n=buildChunk(pixels,0xffffffffu,99,i,p,sizeof p);assert(n<=250);assert(a.receive(p,n,100+i)==(i==21));}
 assert(a.copyPixels(out,200));assert(!memcmp(out,pixels,sizeof out));
 assert(sizeof(FrameAssembler)<12*1024);
}
static void later_restart_frame_recovers_after_missed_startup(){
 FrameAssembler a;uint8_t pixels[FRAME_BYTES],packet[PACKET_BYTES];memset(pixels,0xa5,sizeof pixels);
 for(unsigned i=0;i<CHUNK_COUNT;i++){auto n=buildChunk(pixels,1800,3600000,i,packet,sizeof packet);a.receive(packet,n,100+i);}
 assert(a.fresh(150));
 for(unsigned i=0;i<CHUNK_COUNT;i++){auto n=buildChunk(pixels,8,16000,i,packet,sizeof packet,0x12345678);a.receive(packet,n,6000+i);}
 assert(a.fresh(6050)&&a.sequence()==8&&a.senderSession()==0x12345678);
}
int main(){later_restart_frame_recovers_after_missed_startup();encoding_and_color_contract();publication_is_atomic_and_expires();rejects_bad_headers_and_exact_lengths();order_duplicates_crossframe_and_crc();old_replay_does_not_abort_newer_partial_or_refresh_age();pending_timeout_restart_and_millis_wrap();downsample_validates_length_and_camera_byte_order();encoder_roundtrip_and_bounded_state();puts("PASS camera protocol: encoding, atomic publication, malformed packets, order, CRC, expiry, wrap, memory bound");}
