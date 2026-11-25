#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Controller-neutral mappings. No Bluetooth or platform imports here."""
import math
import threading
from dataclasses import dataclass

@dataclass(frozen=True)
class EyePose:
 x: int = 0
 y: int = 0
 left: int = 0
 right: int = 0
 expression: str = 'neutral'
 def command(self):
  return f'face {self.x} {self.y} {self.left} {self.right} {self.expression}'

def bounded(value,low,high):
 if not math.isfinite(value):raise ValueError('Non-finite controller input')
 return max(low,min(high,value))

def axis(value):
 value=bounded(value,-1,1)
 if abs(value)<=.15:return 0
 return round(math.copysign((abs(value)-.15)/.85*100,value))

class GamepadMapper:
 def __init__(self):
  self.expression='neutral';self.buttons={}
 def sample(self,x,y,left,right,buttons,dpad=(0,0)):
  for button,expression in [('A','happy'),('B','angry'),('X','surprised'),('Y','neutral')]:
   if buttons.get(button) and not self.buttons.get(button):self.expression=expression
  self.buttons=dict(buttons)
  dx,dy=(bounded(v,-1,1) for v in dpad)
  if abs(dx)>.15 or abs(dy)>.15:x,y=dx,dy
  return EyePose(axis(x),-axis(y),round(bounded(left,0,1)*100),round(bounded(right,0,1)*100),self.expression)
 def disconnected(self):
  self.expression='neutral';self.buttons={};return EyePose()

@dataclass(frozen=True)
class LayeredEyePose:
 x: int = 0
 y: int = 0
 px: int = 0
 py: int = 0
 left: int = 0
 right: int = 0
 edge: str = 'bottom'
 expression: str = 'neutral'
 def command(self):
  return f'eyes {self.x} {self.y} {self.px} {self.py} {self.left} {self.right} {self.edge} {self.expression}'

class LayeredGamepadMapper(GamepadMapper):
 def sample(self,left_y,right_x,right_y,buttons,dpad=(0,0)):
  expression=super().sample(0,0,0,0,buttons).expression
  horizontal=axis(right_x);vertical=axis(right_y)
  both=abs(vertical)
  return LayeredEyePose(y=-axis(left_y),px=axis(dpad[0]),py=-axis(dpad[1]),left=max(both,-horizontal),right=max(both,horizontal),edge='top' if vertical<0 else 'bottom',expression=expression)
 def disconnected(self):
  super().disconnected();return LayeredEyePose()

@dataclass(frozen=True)
class PoseRequest:
 generation: int
 pose: EyePose

class PoseMailbox:
 """One pending pose; a completed old request cannot discard a newer stop."""
 def __init__(self,neutral=None):self.lock=threading.Lock();self.pending=None;self.generation=0;self.neutral=neutral if neutral is not None else EyePose()
 def offer(self,pose,force=False):
  with self.lock:
   if not force and self.pending and self.pending.pose==pose:return self.pending
   self.generation+=1;self.pending=PoseRequest(self.generation,pose);return self.pending
 def stop(self):return self.offer(self.neutral,force=True)
 def peek(self):
  with self.lock:return self.pending
 def ack(self,request):
  with self.lock:
   if self.pending==request:self.pending=None
 def clear(self):
  with self.lock:self.pending=None

async def stop_after_failure(mailbox,disconnect):
 """Retain neutral for reconnect and end this unverified control session."""
 mailbox.stop()
 await disconnect()

async def prepare_controller(command,mapper,mailbox):
 """Input may be enabled only after the toy acknowledges isolation from touch."""
 await command('touch off')
 mapper.disconnected()
 mailbox.clear()
