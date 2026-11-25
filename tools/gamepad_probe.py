#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Read real native gamepad inputs; no BLE writes or firmware changes."""
import argparse,json,time
from AppKit import NSApplication
from Foundation import NSRunLoop,NSDate
from GameController import GCController

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--seconds',type=int,default=30)
a=parser.parse_args()
if not 1<=a.seconds<=3600:parser.error('seconds must be 1..3600')
app=NSApplication.sharedApplication()
GCController.setShouldMonitorBackgroundEvents_(True)
end=time.monotonic()+a.seconds
previous=None
while time.monotonic()<end:
 NSRunLoop.currentRunLoop().runUntilDate_(NSDate.dateWithTimeIntervalSinceNow_(.1))
 samples=[]
 for c in GCController.controllers():
  p=c.extendedGamepad()
  sample={'vendor':str(c.vendorName()),'category':str(c.productCategory()),'extended':p is not None}
  if p:
   sample.update(x=p.leftThumbstick().xAxis().value(),y=p.leftThumbstick().yAxis().value(),left=p.leftTrigger().value(),right=p.rightTrigger().value(),rx=p.rightThumbstick().xAxis().value(),ry=p.rightThumbstick().yAxis().value(),dpad=[p.dpad().xAxis().value(),p.dpad().yAxis().value()],buttons={n:bool(getattr(p,'button'+n)().isPressed()) for n in 'ABXY'})
  samples.append(sample)
 if samples!=previous:
  print(json.dumps({'controllers':samples}),flush=True);previous=samples
print('PROBE COMPLETE',flush=True)
