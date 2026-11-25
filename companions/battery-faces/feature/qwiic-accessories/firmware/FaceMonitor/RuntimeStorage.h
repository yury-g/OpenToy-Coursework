// SPDX-License-Identifier: MIT
#pragma once
#include <Preferences.h>
#include "RuntimeObserver.h"
struct RuntimeStorage {
 struct Frame {uint32_t hash=0;RuntimeSnapshot data;};
 Preferences prefs;Frame buffer,check;
 bool available=false,saved=false,restored=false;uint32_t lastSave=0,writes=0;
 static uint32_t hash(const RuntimeSnapshot &s){uint32_t h=2166136261U;auto p=(const uint8_t*)&s;for(size_t i=0;i<sizeof(s);i++)h=(h^p[i])*16777619U;return h;}
 void begin(RuntimeObserver &r){
  available=prefs.begin("bf-runtime-v1",false);if(!available)return;
  if(load("snapshot"))restored=r.restore(buffer.data);
  if(load("background"))r.restoreBackground(buffer.data);
 }
 bool load(const char *key){return prefs.getBytesLength(key)==sizeof(Frame)&&prefs.getBytes(key,&buffer,sizeof(buffer))==sizeof(buffer)&&buffer.hash==hash(buffer.data);}
 bool put(const char *key,const RuntimeSnapshot &s){
  buffer.data=s;buffer.hash=hash(buffer.data);
  return prefs.putBytes(key,&buffer,sizeof(buffer))==sizeof(buffer)&&prefs.getBytes(key,&check,sizeof(check))==sizeof(check)&&check.hash==hash(check.data)&&!memcmp(&buffer,&check,sizeof(buffer));
 }
 bool save(const RuntimeObserver &r,uint32_t now){
  if(!available){saved=false;return false;}
  bool comparison=put("snapshot",r.state),background=put("background",r.background);saved=comparison&&background;
  lastSave=now;if(saved)writes++;return saved;
 }
};
