// SPDX-License-Identifier: MIT
#pragma once
#include "Controls.h"
#include "Personality.h"
#if __has_include("Idle.h")
#include "Idle.h"
#include "Presentation.h"
#else
#include "../NessoEyes/Idle.h"
#include "../NessoEyes/Presentation.h"
#endif
namespace feather_face {
struct CompanionPose {
 FaceState eye[2];int mood[2]={100,100};nesso_eyes::IdlePose idle;
 unsigned personality=0,pokeMask=0;bool originalShape=false;
};
class Companion {
 unsigned personality_=0;bool hadController_=false;
 Controls controls{1};nesso_eyes::Reaction reaction;
 bool manualExpression=false,connected=false;uint32_t previous=0;
public:
 nesso_eyes::Idle idle;
 explicit Companion(unsigned id=0):personality_(personality(id).id),idle(settings(id)){}
 static nesso_eyes::IdleSettings settings(unsigned id){
  const auto& p=personality(id);nesso_eyes::IdleSettings s;s.seed=p.seed;s.pace=p.pace;s.gazeRange=p.gazeRange;s.firstBlink=p.firstBlink;s.winkPeriod=p.winkPeriod;s.blinkStyle=p.id;return s;
 }
 CompanionPose update(int x,int y,uint32_t buttons,bool valid,uint32_t now,unsigned syntheticMask=0){
  CompanionPose out;out.personality=personality_;
  if(!valid&&personality_){
   if(hadController_){controls.update(512,512,0,false);reaction={};manualExpression=false;previous=0;idle.update(now,true);}
   x=y=512;buttons=0;
  }
  hadController_=valid;
  if(!valid&&!personality_){controls.update(x,y,buttons,false);reaction={};idle={};manualExpression=false;connected=false;previous=0;out.mood[0]=out.mood[1]=0;return out;}
  if(!connected){connected=true;idle.update(now,true);}
  if(syntheticMask){x=y=512;buttons=SELECT|(syntheticMask&1?X:0)|(syntheticMask&2?Y:0);}
  unsigned poke=buttons&SELECT?((buttons&X?1:0)|(buttons&Y?2:0)):0;
  if(buttons&B)poke=3;
  uint32_t forwarded=buttons;if(poke)forwarded&=~(X|Y);
  uint32_t pressed=forwarded&~previous;previous=forwarded;
  auto manual=controls.update(x,y,forwarded,true);
  if(pressed&(A|X|Y)){reaction={};manualExpression=true;}
  if(poke)manualExpression=false;
  auto reactionPose=reaction.update(now,poke!=0,poke);
  bool active=buttons||Controls::axis(x)||Controls::axis(y);
  out.idle=idle.update(now,active);out.pokeMask=poke;
  out.originalShape=manualExpression&&manual.expression==SURPRISED;
  for(unsigned i=0;i<2;i++){
   out.mood[i]=manualExpression?(manual.expression==HAPPY?100:0):reactionPose.eyeMood[i];
   auto face=manual;face.expression=out.originalShape?SURPRISED:out.mood[i]<0?ANGRY:out.mood[i]>0?HAPPY:NEUTRAL;
   face.left=std::max(face.left,reactionPose.face.left);face.right=std::max(face.right,reactionPose.face.right);
   out.eye[i]=out.idle.apply(face,i);
  }
  return out;
 }
};
inline nesso_eyes::Transform companionTransform(const CompanionPose& pose){
 nesso_eyes::Transform tr;
 int shift=eyeWhiteY(pose.eye[0])-16;
 tr.scale=std::min(tr.scale,105.f/(36+2*std::abs(shift)+(pose.personality==3?2:0)));
 return tr;
}
// Twice the original radius (8 vs 4 logical pixels). Apply the pupil
// after the existing eye/lid masks so it stays clipped to the visible white.
inline bool companionPixel(FaceState face,int mood,bool originalShape,unsigned eye,int x,int y,unsigned id=0){
 if(x<0||x>=28||y<0||y>=32)return false;
 const auto& p=personality(id);int sx=14+(x-14)*28/p.eyeWidth[eye],sy=16+(y-16)*32/p.eyeHeight[eye];
 unsigned lid=eye?face.right:face.left;
 if(lid==100)return y==16&&sx>=0&&sx<28;
 FaceState shape=face;shape.pupils=false;shape.left=shape.right=0;shape.expression=originalShape?SURPRISED:NEUTRAL;
 if(!eyeLayerPixel(shape,eye,sx,sy))return false;
 unsigned height=32*lid/100;
 if(height&&(face.bottomLids?y>=32-int(height):y<int(height)))return false;
 if(!originalShape){
  if(mood<0&&y*28*100<(eye?28-x:x)*12*(-mood))return false;
  if(mood>0&&y*100<16*nesso_eyes::pupilClearMood(face,mood))return false;
 }
 if(!face.pupils)return true;
 int dx=x-(14+face.pupilX*6/100),dy=y-(16+face.pupilY*7/100);
 return dx*dx+dy*dy>64;
}
inline void drawCompanion(const CompanionPose& pose,uint32_t now,uint8_t* mono){
 std::fill(mono,mono+1024,0);
 for(unsigned eye=0;eye<2;eye++){
  auto shake=nesso_eyes::jitter(now,pose.mood[eye],eye);
  for(int y=0;y<32;y++)for(int x=0;x<28;x++){
   bool lit=companionPixel(pose.eye[eye],pose.mood[eye],pose.originalShape,eye,x,y,pose.personality);
   if(lit){int px=(eye?78:22)+x+shake.x,py=eyeWhiteY(pose.eye[eye])+y+shake.y+personality(pose.personality).eyeOffset[eye];if(px>=0&&px<128&&py>=0&&py<64)mono[(py/8)*128+px]|=1<<(py%8);}
  }
 }
}
}
