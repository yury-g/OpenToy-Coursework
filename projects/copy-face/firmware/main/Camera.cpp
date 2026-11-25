// SPDX-License-Identifier: MIT
// V1.0 wiring and pinned source evidence: docs/CAMERA-RESEARCH.md.
// All frame memory is volatile. No file, network, identity or expression path.
#include "Camera.h"
#include "../../core/LipLandmarkAdapter.h"
#include <atomic>
#include <cerrno>
#include <cstring>
#include <new>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "driver/i2c_master.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_video_init.h"
#include "esp_video_device.h"
#include "esp_video_ioctl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "human_face_detect.hpp"
namespace {
constexpr size_t pixels=camera_width*camera_height,bytes=pixels*sizeof(uint16_t);
SemaphoreHandle_t pendingLock=nullptr,publishedLock=nullptr;
uint16_t *pending=nullptr,*working=nullptr,*published=nullptr;
CameraStatus status,publishedStatus;
copy_face::Frame publishedFrame;
uint32_t pendingAt=0,pendingSequence=0;
bool havePublished=false,started=false;
std::atomic<bool> captureStopped{false};
uint32_t nowMs(){return uint32_t(esp_timer_get_time()/1000);}
// These locks only protect short copies. The UI never waits for either worker.
void fail(int error){
 if(xSemaphoreTake(publishedLock,pdMS_TO_TICKS(20))==pdTRUE){
  status.error=error;status.ready=false;
  xSemaphoreGive(publishedLock);
 }
}
int ioError(){return errno?-errno:ESP_FAIL;}
void stage(unsigned value){if(xSemaphoreTake(publishedLock,pdMS_TO_TICKS(20))==pdTRUE){status.stage=value;xSemaphoreGive(publishedLock);}}
void inferRun(){
 // ESP-DL owns its model allocations; preflight headroom before constructors.
 // The upstream model API has no recoverable allocation-error contract.
 if(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)<4*1024*1024){captureStopped.store(true);fail(ESP_ERR_NO_MEM);return;}
 HumanFaceDetect detector(HumanFaceDetect::MSRMNP_S8_V1,false);
 if(xSemaphoreTake(publishedLock,pdMS_TO_TICKS(20))==pdTRUE){
  status.detector_ready=true;xSemaphoreGive(publishedLock);
 }
 uint32_t last=0;
 while(!captureStopped.load()){
  uint32_t sequence=0,at=0;
  if(xSemaphoreTake(pendingLock,pdMS_TO_TICKS(5))==pdTRUE){
   if(pendingSequence!=last){
    sequence=pendingSequence;at=pendingAt;std::memcpy(working,pending,bytes);
   }
   xSemaphoreGive(pendingLock);
  }
  if(!sequence){vTaskDelay(1);continue;}
  last=sequence;
  copy_face::Frame frame;frame.captured=at;
  dl::image::img_t input={working,camera_width,camera_height,dl::image::DL_IMAGE_PIX_TYPE_RGB565LE};
  const uint32_t start=nowMs();
  auto& results=detector.run(input);
  const uint32_t duration=nowMs()-start;
  for(const auto& result:results){
   if(frame.count==8)break;
   if(result.box.size()<4)continue;
   copy_face::Detection d;
   const float x0=copy_face::clamp(float(result.box[0])/camera_width,0,1);
   const float y0=copy_face::clamp(float(result.box[1])/camera_height,0,1);
   const float x1=copy_face::clamp(float(result.box[2])/camera_width,0,1);
   const float y1=copy_face::clamp(float(result.box[3])/camera_height,0,1);
   d.x=x0;d.y=y0;d.w=x1-x0;d.h=y1-y0;d.confidence=result.score;
   if(!copy_face::valid(d))continue;
   if(result.keypoint.size()==10){
    d.keypointsValid=true;
    for(unsigned i=0;i<5;i++){
     const float x=float(result.keypoint[i*2])/camera_width;
     const float y=float(result.keypoint[i*2+1])/camera_height;
     if(x<0||x>1||y<0||y>1)d.keypointsValid=false;
     d.keypoints[i]={x,y};
    }
   }
   // Five points do not establish lips, eyelids, aperture, bend or emotion.
   copy_face::applyFivePointOnly(d);
   frame.faces[frame.count++]=d;
  }
  if(xSemaphoreTake(publishedLock,pdMS_TO_TICKS(10))==pdTRUE){
   if(!captureStopped.load()){
    std::memcpy(published,working,bytes);publishedFrame=frame;
    status.captured_ms=at;status.inference_ms=duration;
    publishedStatus=status;publishedStatus.capture_count=sequence;
    havePublished=true;
   }
   xSemaphoreGive(publishedLock);
  }
  // Finite model graph/candidate count, but inference is not hard realtime.
  vTaskDelay(1);
 }
}
void inferTask(void*){
 inferRun(); // Destroy model allocations before deleting the FreeRTOS task.
 if(xSemaphoreTake(publishedLock,pdMS_TO_TICKS(20))==pdTRUE){
  status.detector_ready=false;xSemaphoreGive(publishedLock);
 }
 vTaskDelete(nullptr);
}
struct Capture {
 i2c_master_bus_handle_t bus=nullptr;
 int fd=-1;
 bool video=false,stream=false;
 void* buffers[2]={};size_t sizes[2]={};
 uint32_t width=0,height=0,stride=0;
 int setup(){
  i2c_master_bus_config_t cfg={};cfg.i2c_port=I2C_NUM_1;
  cfg.sda_io_num=GPIO_NUM_12;cfg.scl_io_num=GPIO_NUM_13;
  cfg.clk_source=I2C_CLK_SRC_DEFAULT;cfg.glitch_ignore_cnt=7;
  cfg.flags.enable_internal_pullup=true;
  stage(1);esp_err_t err=i2c_new_master_bus(&cfg,&bus);if(err!=ESP_OK)return err;
  i2c_device_config_t devcfg={};devcfg.dev_addr_length=I2C_ADDR_BIT_LEN_7;
  devcfg.device_address=0x30;devcfg.scl_speed_hz=100000;
  i2c_master_dev_handle_t dev=nullptr;
  stage(2);err=i2c_master_bus_add_device(bus,&devcfg,&dev);if(err!=ESP_OK)return err;
  const uint8_t hiReg[2]={0x31,0x07},loReg[2]={0x31,0x08};uint8_t hi=0,lo=0;
  stage(3);err=i2c_master_transmit_receive(dev,hiReg,2,&hi,1,20);
  if(err==ESP_OK){stage(4);err=i2c_master_transmit_receive(dev,loReg,2,&lo,1,20);}
  esp_err_t remove=i2c_master_bus_rm_device(dev);
  if(err!=ESP_OK)return err;
  if(remove!=ESP_OK)return remove;
  stage(5);const uint16_t identity=(uint16_t(hi)<<8)|lo;
  if(xSemaphoreTake(publishedLock,pdMS_TO_TICKS(20))==pdTRUE){status.sensor_id=identity;xSemaphoreGive(publishedLock);}
  if(identity!=0xcb3a)return ESP_ERR_NOT_SUPPORTED;
  esp_video_init_csi_config_t csi={};
  csi.sccb_config.init_sccb=false;csi.sccb_config.i2c_handle=bus;csi.sccb_config.freq=100000;
  csi.reset_pin=GPIO_NUM_NC;csi.pwdn_pin=GPIO_NUM_NC;
  // Board.c already owns LDO3 at 2.5V for the display MIPI PHY.
  csi.dont_init_ldo=true;
  esp_video_init_config_t init={};init.csi=&csi;
  stage(6);err=esp_video_init(&init);if(err!=ESP_OK)return err;video=true;
  stage(7);fd=open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME,O_RDONLY);if(fd<0)return ioError();
  v4l2_capability cap={};if(ioctl(fd,VIDIOC_QUERYCAP,&cap)<0)return ioError();
  stage(8);v4l2_format format={};format.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if(ioctl(fd,VIDIOC_G_FMT,&format)<0)return ioError();
  format.fmt.pix.pixelformat=V4L2_PIX_FMT_RGB565; // v2: native little endian.
  if(ioctl(fd,VIDIOC_S_FMT,&format)<0||ioctl(fd,VIDIOC_G_FMT,&format)<0)return ioError();
  width=format.fmt.pix.width;height=format.fmt.pix.height;
  if(format.fmt.pix.pixelformat!=V4L2_PIX_FMT_RGB565||width!=1024||height!=600)return ESP_ERR_NOT_SUPPORTED;
  // v2.0.0 reports zero bytesperline for packed raw buffers; its allocator
  // uses width*height*bpp/8. Respect a reported stride when nonzero.
  stride=format.fmt.pix.bytesperline?format.fmt.pix.bytesperline:width*2;
  if(stride<width*2||stride>8192)return ESP_ERR_INVALID_SIZE;
  stage(9);timeval timeout={0,250000},actual={};
  if(ioctl(fd,VIDIOC_S_DQBUF_TIMEOUT,&timeout)<0||ioctl(fd,VIDIOC_G_DQBUF_TIMEOUT,&actual)<0)return ioError();
  if(actual.tv_sec!=0||actual.tv_usec<=0||actual.tv_usec>250000)return ESP_ERR_INVALID_STATE;
  stage(10);v4l2_requestbuffers request={};request.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
  request.memory=V4L2_MEMORY_MMAP;request.count=2;
  if(ioctl(fd,VIDIOC_REQBUFS,&request)<0)return ioError();
  if(request.count!=2)return ESP_ERR_INVALID_SIZE;
  for(unsigned i=0;i<2;i++){
   v4l2_buffer b={};b.type=request.type;b.memory=request.memory;b.index=i;
   if(ioctl(fd,VIDIOC_QUERYBUF,&b)<0)return ioError();
   if(b.length<size_t(stride)*height)return ESP_ERR_INVALID_SIZE;
   sizes[i]=b.length;buffers[i]=mmap(nullptr,b.length,PROT_READ|PROT_WRITE,MAP_SHARED,fd,b.m.offset);
   if(!buffers[i]||buffers[i]==MAP_FAILED){buffers[i]=nullptr;return ioError();}
   if(ioctl(fd,VIDIOC_QBUF,&b)<0)return ioError();
  }
  stage(11);int type=V4L2_BUF_TYPE_VIDEO_CAPTURE;if(ioctl(fd,VIDIOC_STREAMON,&type)<0)return ioError();stream=true;
  if(xSemaphoreTake(publishedLock,pdMS_TO_TICKS(20))==pdTRUE){
   status.width=width;status.height=height;status.ready=true;status.stage=12;xSemaphoreGive(publishedLock);
  }
  return 0;
 }
 int capture(){
  v4l2_buffer b={};b.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;b.memory=V4L2_MEMORY_MMAP;
  // This release has no hardware capture timestamp. Continuous draining plus
  // a one-frame allowance gives an age estimate, not an exposure timestamp.
  const uint32_t captured=nowMs()-34;
  if(ioctl(fd,VIDIOC_DQBUF,&b)<0)return ioError();
  int error=0;
  if(b.index>=2)return ESP_ERR_INVALID_RESPONSE;
  if((b.flags&V4L2_BUF_FLAG_ERROR)||b.bytesused<size_t(stride)*height||b.bytesused>sizes[b.index])error=ESP_ERR_INVALID_SIZE;
  else if(xSemaphoreTake(pendingLock,pdMS_TO_TICKS(5))==pdTRUE){
   // Center crop 1024x600 to 1024x576, then downsample without aspect distortion.
   // Both inference and preview consume this exact crop/orientation.
   for(unsigned y=0;y<camera_height;y++){
    const unsigned sy=12+y*576/camera_height;
    const auto* row=reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(buffers[b.index])+sy*stride);
    for(unsigned x=0;x<camera_width;x++)pending[y*camera_width+x]=row[x*width/camera_width];
   }
   pendingAt=captured;++pendingSequence;
   xSemaphoreGive(pendingLock);
  }
  if(ioctl(fd,VIDIOC_QBUF,&b)<0)return ioError();
  return error;
 }
 void close(){
  if(stream){int type=V4L2_BUF_TYPE_VIDEO_CAPTURE;ioctl(fd,VIDIOC_STREAMOFF,&type);stream=false;}
  // MMAP is a driver-owned allocation. close/deinit frees the buffer lifecycle.
  for(unsigned i=0;i<2;i++)if(buffers[i]){munmap(buffers[i],sizes[i]);buffers[i]=nullptr;}
  if(fd>=0){::close(fd);fd=-1;}
  if(video){esp_video_deinit();video=false;}
  if(bus){i2c_del_master_bus(bus);bus=nullptr;}
 }
};
void captureTask(void*){
 Capture camera;int error=camera.setup();
 if(!error&&xTaskCreatePinnedToCore(inferTask,"face-infer",16384,nullptr,3,nullptr,1)!=pdPASS)error=ESP_ERR_NO_MEM;
 unsigned failures=0;
 while(!error&&!captureStopped.load()){
  const int result=camera.capture();
  if(result){if(++failures>=3)error=result;else vTaskDelay(pdMS_TO_TICKS(20));}
  else failures=0;
 }
 captureStopped.store(true);if(error)fail(error);camera.close();vTaskDelete(nullptr);
}
}
bool camera_begin(){
 if(started)return false;
 started=true;
 pendingLock=xSemaphoreCreateMutex();publishedLock=xSemaphoreCreateMutex();
 if(!pendingLock||!publishedLock)return false;
 pending=static_cast<uint16_t*>(heap_caps_malloc(bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
 working=static_cast<uint16_t*>(heap_caps_malloc(bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
 published=static_cast<uint16_t*>(heap_caps_malloc(bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
 if(!pending||!working||!published){fail(ESP_ERR_NO_MEM);return false;}
 if(xTaskCreatePinnedToCore(captureTask,"face-capture",8192,nullptr,4,nullptr,0)!=pdPASS){fail(ESP_ERR_NO_MEM);return false;}
 return true;
}
bool camera_copy_latest(uint16_t* dest,size_t capacity,copy_face::Frame* observations,CameraStatus* out){
 if(!publishedLock||xSemaphoreTake(publishedLock,0)!=pdTRUE)return false;
 bool ok=havePublished&&dest&&capacity>=pixels&&observations;
 if(out){*out=havePublished?publishedStatus:status;out->ready=status.ready;out->detector_ready=status.detector_ready;out->error=status.error;}
 if(ok){std::memcpy(dest,published,bytes);*observations=publishedFrame;}
 xSemaphoreGive(publishedLock);return ok;
}
