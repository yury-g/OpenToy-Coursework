// SPDX-License-Identifier: MIT
#pragma once
#include <Arduino.h>
#include <esp_ota_ops.h>
#include <esp_random.h>
#include <mbedtls/sha256.h>
#include <mbedtls/md.h>
#include "Secrets.h"
#include "ImageBounds.h"
// Custom protocol, intentionally distinct from Nordic DFU / Espressif BLE OTA.
class ToyOta {
 esp_ota_handle_t handle=0;
 const esp_partition_t *target=nullptr;
 uint32_t total=0,received=0,last=0;
 uint8_t expected[32]={},nonce[16]={};
 mbedtls_sha256_context hash;
 bool active=false,ready=false,committed=false;
public:
 char response[100]={};
 ToyOta(){mbedtls_sha256_init(&hash);}
 void init(){esp_fill_random(nonce,sizeof(nonce));status();}
 bool busy()const{return active;}
 bool staged()const{return ready;}
 uint32_t offset()const{return received;}
 void status(){char n[33];for(int i=0;i<16;i++)snprintf(n+i*2,3,"%02x",nonce[i]);snprintf(response,sizeof(response),"HELLO 1 %s %lu",n,(unsigned long)received);}
 void abort(){if(active)esp_ota_abort(handle);active=false;ready=false;received=0;}
 bool authenticate(const uint8_t *metadata,size_t len,const uint8_t *mac){
  uint8_t data[52],result[32];if(len>36)return false;memcpy(data,nonce,16);memcpy(data+16,metadata,len);
  if(mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),OTA_KEY,sizeof(OTA_KEY),data,16+len,result)!=0)return false;
  uint8_t diff=0;for(int i=0;i<32;i++)diff|=result[i]^mac[i];return diff==0;
 }
 bool tick(){if(active && millis()-last>15000){abort();strcpy(response,"ERR timeout");return true;}return false;}
 // Returns true only when a staged, validated image should reboot.
 bool packet(const uint8_t *p,size_t n){
  if(!n){strcpy(response,"ERR empty");return false;}
  if(p[0]==0 && n==1){status();return false;}
  if(p[0]==1){
   if(n!=69){strcpy(response,"ERR start-length");return false;}
   if(active||ready){strcpy(response,"ERR busy");return false;}
   if(!authenticate(p+1,36,p+37)){strcpy(response,"ERR auth");return false;}
   memcpy(&total,p+1,4);target=esp_ota_get_next_update_partition(nullptr);
   if(!target||!total||total>target->size){strcpy(response,"ERR size");return false;}
   esp_err_t e=esp_ota_begin(target,total,&handle);
   if(e!=ESP_OK){snprintf(response,sizeof(response),"ERR begin %d",e);return false;}
   memcpy(expected,p+5,32);esp_fill_random(nonce,sizeof(nonce));received=0;active=true;last=millis();
   mbedtls_sha256_starts(&hash,0);strcpy(response,"START 0");return false;
  }
  if(p[0]==2){
   if(!active){strcpy(response,"ERR inactive");return false;}
   if(n<6){strcpy(response,"ERR data-length");return false;}
   uint32_t o;memcpy(&o,p+1,4);
   if(o!=received || n-5>total-received){strcpy(response,"ERR offset");return false;}
   esp_err_t e=esp_ota_write(handle,p+5,n-5);
   if(e!=ESP_OK){abort();snprintf(response,sizeof(response),"ERR write %d",e);return false;}
   mbedtls_sha256_update(&hash,p+5,n-5);received+=n-5;last=millis();
   snprintf(response,sizeof(response),"DATA %lu",(unsigned long)received);return false;
  }
  if(p[0]==3 && n==1){
   if(!active||received!=total){strcpy(response,"ERR incomplete");return false;}
   uint8_t actual[32];mbedtls_sha256_finish(&hash,actual);
   if(memcmp(actual,expected,32)){abort();strcpy(response,"ERR sha256");return false;}
   if(!imageBounds(total,[&](uint32_t o,uint8_t *b,uint32_t count){return esp_partition_read(target,o,b,count)==ESP_OK;})){abort();strcpy(response,"ERR image bounds");return false;}
   esp_err_t e=esp_ota_end(handle);active=false;
   if(e!=ESP_OK){snprintf(response,sizeof(response),"ERR image %d",e);return false;}
   ready=true;strcpy(response,"READY");return false;
  }
  if(p[0]==4 && n==1){abort();strcpy(response,"ABORTED");return false;}
  if(p[0]==5 && n==1){if(!ready){strcpy(response,"ERR not-ready");return false;}esp_err_t e=esp_ota_set_boot_partition(target);if(e!=ESP_OK){snprintf(response,sizeof(response),"ERR activate %d",e);return false;}committed=true;strcpy(response,"REBOOT");return true;}
  strcpy(response,"ERR opcode");return false;
 }
};
