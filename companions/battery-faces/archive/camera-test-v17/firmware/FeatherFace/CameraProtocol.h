// SPDX-License-Identifier: MIT
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// A deliberately small ESP-NOW v1 payload. No Arduino types, packed structs,
// dynamic allocation, or dependence on the receiving CPU's byte order.
namespace facecamera {
constexpr size_t WIDTH=80,HEIGHT=60,FRAME_BYTES=WIDTH*HEIGHT;
constexpr size_t CHUNK_BYTES=220,HEADER_BYTES=28,PACKET_BYTES=HEADER_BYTES+CHUNK_BYTES;
constexpr size_t CHUNK_COUNT=(FRAME_BYTES+CHUNK_BYTES-1)/CHUNK_BYTES;
constexpr uint32_t FRAME_TTL_MS=5000;
constexpr unsigned SOURCE_COUNT=2;
inline const char* sourceName(unsigned n){return n==1?"TAB5 CAMERA":"TEAL CAMERA";}
// Separate from the battery registry. Verify the board's ROM/station MAC before installation.
constexpr uint8_t CAMERA_MAC[6]={0x10,0xb4,0x1d,0xd2,0xa2,0xc0};
constexpr uint8_t RELAY_MAC[6]={0xb8,0xf8,0x62,0xd5,0x8b,0x8c};
inline bool sourceAllowed(const uint8_t *mac,unsigned source){return mac&&source<SOURCE_COUNT&&!memcmp(mac,source==0?CAMERA_MAC:RELAY_MAC,6);}
static_assert(PACKET_BYTES<=250,"Classic ESP32 ESP-NOW payload limit");

inline unsigned frameWidth(unsigned format){return format==2?160:80;}
inline unsigned frameHeight(unsigned format){return format==2?120:60;}
inline uint32_t read32(const uint8_t *p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
inline void write32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=uint8_t(v>>(i*8));}
inline uint32_t crc32(const uint8_t *p,size_t n){
 uint32_t crc=0xffffffffu;
 for(size_t i=0;i<n;i++){
  crc^=p[i];for(unsigned bit=0;bit<8;bit++)crc=(crc>>1)^((crc&1)?0xedb88320u:0u);
 }
 return crc^0xffffffffu;
}
inline size_t payloadBytes(unsigned index){
 if(index>=CHUNK_COUNT)return 0;
 const size_t left=FRAME_BYTES-size_t(index)*CHUNK_BYTES;return left<CHUNK_BYTES?left:CHUNK_BYTES;
}
// Camera RGB565 is high byte first, as in esp32-camera's RGB565 conversion path.
inline uint8_t rgb565beTo332(uint8_t high,uint8_t low){return (high&0xe0)|((high&7)<<2)|((low>>3)&3);}
inline uint16_t rgb332To565(uint8_t v){
 const uint16_t r=v>>5,g=(v>>2)&7,b=v&3;
 return uint16_t((((r<<2)|(r>>1))<<11)|(((g<<3)|g)<<5)|((b<<3)|(b<<1)|(b>>1)));
}
// Four-level 2-bit grayscale: 160x120 pixels in the same 4800 bytes.
inline uint16_t pixel565(const uint8_t *p,unsigned format,unsigned x,unsigned y){
 if(format!=2)return rgb332To565(p[y*80+x]);
 unsigned index=y*160+x,level=(p[index/4]>>(6-2*(index%4)))&3;unsigned v=level*85;
 return uint16_t(((v>>3)<<11)|((v>>2)<<5)|(v>>3));
}
inline bool downsampleGray2(const uint8_t *source,size_t length,size_t width,size_t height,uint8_t *out,size_t capacity){
 if(!source||!out||width!=320||height!=240||length<320u*240u*2u||capacity<FRAME_BYTES)return false;
 memset(out,0,FRAME_BYTES);
 for(unsigned y=0;y<120;y++)for(unsigned x=0;x<160;x++){
  // Average a 2x2 source block before luminance quantization.
  unsigned sum=0;for(unsigned dy=0;dy<2;dy++)for(unsigned dx=0;dx<2;dx++){
   size_t at=((y*2+dy)*320+x*2+dx)*2;unsigned c=(unsigned(source[at])<<8)|source[at+1];
   unsigned r=((c>>11)&31)*255/31,g=((c>>5)&63)*255/63,b=(c&31)*255/31;
   sum+=77*r+150*g+29*b;
  }
  unsigned gray=(sum+512)/1024,level=(gray*3+127)/255,index=y*160+x;
  out[index/4]|=uint8_t(level<<(6-2*(index%4)));
 }
 return true;
}
// Restrict input to the configured QVGA format, preventing accidental JPEG or
// differently-sized buffers from being interpreted as RGB565.
inline bool downsampleRgb565be(const uint8_t *source,size_t length,size_t width,size_t height,
                               uint8_t *out,size_t capacity){
 if(!source||!out||width!=320||height!=240||length<320u*240u*2u||capacity<FRAME_BYTES)return false;
 for(size_t y=0;y<HEIGHT;y++)for(size_t x=0;x<WIDTH;x++){
  const size_t offset=((y*4+2)*320+(x*4+2))*2;
  out[y*WIDTH+x]=rgb565beTo332(source[offset],source[offset+1]);
 }
 return true;
}
// The precomputed-CRC form avoids hashing 4,800 bytes for each of 22 packets.
inline size_t buildChunkWithCrc(const uint8_t *pixels,uint32_t seq,uint32_t uptime,
                               uint32_t checksum,unsigned index,uint8_t *out,size_t capacity,uint32_t session=0,unsigned source=0,unsigned format=1){
 const size_t n=payloadBytes(index);
 if(!pixels||!out||!n||capacity<HEADER_BYTES+n||source>=SOURCE_COUNT||(format!=1&&format!=2))return 0;
 out[0]='F';out[1]='C';out[2]='A';out[3]='M';out[4]=1;out[5]=uint8_t(index);
 out[6]=uint8_t(CHUNK_COUNT);out[7]=uint8_t(format|(source<<4)); // low nibble RGB332, high nibble camera source
 write32(out+8,seq);write32(out+12,uptime);write32(out+16,checksum);
 out[20]=uint8_t(frameWidth(format));out[21]=uint8_t(frameHeight(format));out[22]=uint8_t(n);out[23]=uint8_t(n>>8);
 write32(out+24,session);
 memcpy(out+HEADER_BYTES,pixels+size_t(index)*CHUNK_BYTES,n);return HEADER_BYTES+n;
}
inline size_t buildChunk(const uint8_t *pixels,uint32_t seq,uint32_t uptime,
                         unsigned index,uint8_t *out,size_t capacity,uint32_t session=0,unsigned source=0,unsigned format=1){
 if(!pixels)return 0;
 return buildChunkWithCrc(pixels,seq,uptime,crc32(pixels,FRAME_BYTES),index,out,capacity,session,source,format);
}

// Keep external synchronization around receive/copyPixels/accessors when called
// from different tasks (ESP-NOW callback versus loop). A successful receive
// publishes all pixels together; an incomplete next frame never erases the last.
class FrameAssembler {
 const unsigned source_;
 uint8_t staging_[FRAME_BYTES]={},complete_[FRAME_BYTES]={};
 unsigned pendingFormat_=1,format_=1;
 uint32_t pendingSession_=0,session_=0;
 uint32_t pendingSeq_=0,pendingUptime_=0,pendingCrc_=0,startedAt_=0;
 uint32_t sequence_=0,uptime_=0,completedAt_=0,completedFrames_=0,rejectedPackets_=0;
 uint32_t receivedMask_=0;bool active_=false,seen_=false;
 bool reject(){active_=false;++rejectedPackets_;return false;}
 public:
 explicit FrameAssembler(unsigned source=0):source_(source){}
 // true means a new CRC-verified complete frame was published. false includes
 // both accepted partial packets and rejected packets; use counters for diagnostics.
 bool receive(const void *data,size_t size,uint32_t now){
  if(active_&&uint32_t(now-startedAt_)>=FRAME_TTL_MS)active_=false;
  if(!data||size<HEADER_BYTES||size>PACKET_BYTES)return reject();
  const uint8_t *p=static_cast<const uint8_t*>(data);
  const unsigned index=p[5],format=p[7]&15;const size_t n=payloadBytes(index);
  if(p[0]!='F'||p[1]!='C'||p[2]!='A'||p[3]!='M'||p[4]!=1||p[6]!=CHUNK_COUNT||(p[7]>>4)!=source_||(format!=1&&format!=2)||
     p[20]!=frameWidth(format)||p[21]!=frameHeight(format)||!n||size!=HEADER_BYTES+n||
     (size_t(p[22])|(size_t(p[23])<<8))!=n)return reject();
  const uint32_t seq=read32(p+8),uptime=read32(p+12),checksum=read32(p+16),session=read32(p+24);
  // The normal repeated copy is harmless and is not an error counter event.
  if(seen_&&session==session_&&seq==sequence_&&uptime==uptime_)return false;
  const bool matchesPending=active_&&session==pendingSession_&&seq==pendingSeq_&&uptime==pendingUptime_&&checksum==pendingCrc_&&format==pendingFormat_;
  const bool newer=session==session_&&int32_t(seq-sequence_)>0&&int32_t(uptime-uptime_)>=0;
  const bool reboot=!fresh(now)&&session!=session_;
  if(seen_&&!matchesPending&&!newer&&!reboot){++rejectedPackets_;return false;}
  if(active_&&!matchesPending&&session==pendingSession_&&int32_t(seq-pendingSeq_)<=0){++rejectedPackets_;return false;}
  if(!matchesPending){
   pendingFormat_=format;pendingSession_=session;pendingSeq_=seq;pendingUptime_=uptime;pendingCrc_=checksum;startedAt_=now;receivedMask_=0;active_=true;
  }
  // A compact bitmap merges partial repeated copies and accepts reordering.
  // Only the complete image CRC permits publication; duplicates never refresh age.
  const uint32_t bit=uint32_t(1)<<index;uint8_t *target=staging_+size_t(index)*CHUNK_BYTES;
  if(receivedMask_&bit){if(memcmp(target,p+HEADER_BYTES,n))return reject();return false;}
  memcpy(target,p+HEADER_BYTES,n);receivedMask_|=bit;
  if(receivedMask_!=((uint32_t(1)<<CHUNK_COUNT)-1))return false;
  active_=false;
  if(crc32(staging_,FRAME_BYTES)!=pendingCrc_)return reject();
  memcpy(complete_,staging_,FRAME_BYTES);session_=pendingSession_;sequence_=pendingSeq_;uptime_=pendingUptime_;
  format_=pendingFormat_;completedAt_=now;seen_=true;++completedFrames_;return true;
 }
 bool fresh(uint32_t now)const{return seen_&&uint32_t(now-completedAt_)<FRAME_TTL_MS;}
 // out must have FRAME_BYTES bytes. A failed copy leaves it untouched.
 bool copyPixels(uint8_t *out,uint32_t now)const{
  if(!out||!fresh(now))return false;memcpy(out,complete_,FRAME_BYTES);return true;
 }
 // Call only for an externally confirmed source reset. This invalidates the old
 // image and permits a new sequence epoch immediately.
 void resetSource(){active_=false;seen_=false;}
 unsigned format()const{return format_;}
 unsigned width()const{return frameWidth(format_);}
 unsigned height()const{return frameHeight(format_);}
 uint32_t sequence()const{return sequence_;}
 uint32_t senderSession()const{return session_;}
 uint32_t senderUptime()const{return uptime_;}
 uint32_t completedAt()const{return completedAt_;}
 uint32_t completedFrames()const{return completedFrames_;}
 uint32_t rejectedPackets()const{return rejectedPackets_;}
};
static_assert(sizeof(FrameAssembler)<12*1024,"Camera assembler RAM budget");
} // namespace facecamera
