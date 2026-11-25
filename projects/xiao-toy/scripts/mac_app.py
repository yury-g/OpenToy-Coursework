#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Native Mac app: persistent BLE connection, bounded live control, OTA upload."""
import asyncio,threading,queue,json,time,hashlib,contextlib
from GameController import GCController
import faulthandler,signal
faulthandler.register(signal.SIGUSR1)
from pathlib import Path
import objc
from Foundation import NSObject,NSTimer
from AppKit import (NSApplication,NSWindow,NSButton,NSTextField,NSScrollView,NSTextView,NSOpenPanel,NSPopUpButton,NSMakeRect,NSBackingStoreBuffered,NSWindowStyleMaskTitled,NSWindowStyleMaskClosable,NSWindowStyleMaskMiniaturizable,NSFont,NSApplicationActivationPolicyRegular)
from bleak import BleakClient
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from ble_client import Toy,find,load_image
from controller_input import LayeredGamepadMapper,LayeredEyePose,PoseMailbox,stop_after_failure,prepare_controller
ROOT=Path(__file__).resolve().parents[1]
PROJECTS=[p for p in [ROOT,ROOT.parent/'xiao-class'] if (p/'device.json').exists()]
if not PROJECTS:raise SystemExit('Prepare a toy profile before opening the app.')
class Controller(NSObject):
 def init(self):
  self=objc.super(Controller,self).init()
  if self is None:return None
  self.root=PROJECTS[0];self.profile=json.loads((self.root/"device.json").read_text());self.selected=0;self.events=queue.Queue();self.loop=asyncio.new_event_loop();self.client=None;self.toy=None;self.busy=False;self.ready=False;self.controllerEnabled=False;self.gamepadMapper=LayeredGamepadMapper();self.supportsLayers=False;self.lastPose=None;self.poseMailbox=PoseMailbox(neutral=LayeredEyePose());self.lastPoseAt=0;self.hadController=False
  threading.Thread(target=self.loop.run_forever,daemon=True).start();return self
 @objc.python_method
 def log(self,s):
  (self.root/"evidence").mkdir(exist_ok=True)
  self.events.put(str(s))
  with (self.root/"evidence/mac-app-events.log").open("a",encoding="utf-8") as f:f.write(time.strftime("%Y-%m-%d %H:%M:%S ")+str(s)+"\n")
 @objc.python_method
 def launch(self,coro):
  if self.busy:self.log('Wait for the current operation to finish.');coro.close();return
  self.busy=True
  async def task():
   try:await coro
   except Exception as e:self.log('Failed: '+str(e));self.events.put('STATE disconnected' if not self.client or not self.client.is_connected else 'STATE connected')
   finally:self.busy=False
  asyncio.run_coroutine_threadsafe(task(),self.loop)
 def applicationDidFinishLaunching_(self,n):
  if hasattr(self,'window'):return
  print('creating control window',flush=True)
  self.window=NSWindow.alloc().initWithContentRect_styleMask_backing_defer_(NSMakeRect(100,100,700,680),NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable,NSBackingStoreBuffered,False)
  self.window.setTitle_('OpenToy Control');self.window.center();self.window.setReleasedWhenClosed_(False)
  v=self.window.contentView();self.peripheralControls=[]
  def label(text,x,y,w,h=25):
   l=NSTextField.labelWithString_(text);l.setFrame_(NSMakeRect(x,y,w,h));v.addSubview_(l);return l
  def button(text,x,y,w,action):
   b=NSButton.alloc().initWithFrame_(NSMakeRect(x,y,w,32));b.setTitle_(text);b.setBezelStyle_(1);b.setTarget_(self);b.setAction_(action);v.addSubview_(b);
   if action in ["eyesToy:","closedToy:","testScreenToy:","screenOffToy:","contrastToy:","thresholdToy:","touchOnToy:","touchOffToy:"]:self.peripheralControls.append(b);b.setEnabled_(False)
   return b
  label('OpenToy · Bluetooth workshop',22,631,650,30).setFont_(NSFont.boldSystemFontOfSize_(22))
  self.status=label('Disconnected · this Mac keeps internet on Ethernet',24,601,650)
  label('Toy',24,561,70);self.board=NSPopUpButton.alloc().initWithFrame_pullsDown_(NSMakeRect(95,554,390,32),False);self.board.addItemsWithTitles_([json.loads((p/'device.json').read_text())['name'] for p in PROJECTS]);self.board.setTarget_(self);self.board.setAction_('chooseBoard:');v.addSubview_(self.board)
  self.connect=button('Connect',22,505,120,'connectToy:');button('Check connection',152,505,170,'checkToy:');button('Disconnect',332,505,120,'disconnectToy:')
  label('Blink period (ms)',24,471,180);self.period=NSTextField.alloc().initWithFrame_(NSMakeRect(210,466,110,28));self.period.setStringValue_('1000');v.addSubview_(self.period)
  label('Duty (%)',345,471,110);self.duty=NSTextField.alloc().initWithFrame_(NSMakeRect(450,466,90,28));self.duty.setStringValue_('50');v.addSubview_(self.duty)
  button('Apply',557,465,120,'applyToy:')
  button('Blink',22,419,120,'blinkToy:');button('On',152,419,120,'onToy:');button('Off',282,419,120,'offToy:');button('Reset',412,419,120,'resetToy:')
  button('Eyes',22,373,120,'eyesToy:');button('Closed eyes',152,373,120,'closedToy:');button('Test pattern',282,373,120,'testScreenToy:');button('Screen off',412,373,120,'screenOffToy:');label('Brightness',553,398,120,18);self.contrast=NSTextField.alloc().initWithFrame_(NSMakeRect(550,373,52,28));self.contrast.setStringValue_('127');v.addSubview_(self.contrast);button('Set',607,371,70,'contrastToy:')
  label('Touch threshold',24,332,180);self.threshold=NSTextField.alloc().initWithFrame_(NSMakeRect(190,327,110,28));self.threshold.setStringValue_('80000');v.addSubview_(self.threshold);button('Set threshold',305,326,135,'thresholdToy:');button('Touch on',450,326,110,'touchOnToy:');button('Touch off',565,326,110,'touchOffToy:')
  button('Build + update',470,271,207,'buildToy:')
  button('Upload firmware…',22,271,195,'uploadToy:');label('Use a built .bin + manifest',228,275,235)
  label('Acknowledgments from the toy',24,240,640)
  self.controllerButton=button('Enable controller',24,204,180,'controllerToy:');self.controllerLabel=label('Controller input off',214,207,455)
  scroll=NSScrollView.alloc().initWithFrame_(NSMakeRect(24,40,652,155));scroll.setHasVerticalScroller_(True)
  self.output=NSTextView.alloc().initWithFrame_(NSMakeRect(0,0,652,155));self.output.setEditable_(False);self.output.setFont_(NSFont.monospacedSystemFontOfSize_weight_(11,0));scroll.setDocumentView_(self.output);v.addSubview_(scroll)
  label('Live controls change behavior immediately. Updates verify, reboot and reconnect.',24,10,652,20)
  self.timer=NSTimer.scheduledTimerWithTimeInterval_target_selector_userInfo_repeats_(.1,self,'poll:',None,True)
  for control in self.peripheralControls:control.setEnabled_(bool(self.profile.get('peripherals')))
  self.window.makeKeyAndOrderFront_(None);NSApplication.sharedApplication().activateIgnoringOtherApps_(True)
 def poll_(self,t):
  self.controllerButton.setTitle_('Stop controller' if self.controllerEnabled else 'Enable controller')
  while not self.events.empty():
   s=self.events.get()
   if s.startswith('STATE '):self.status.setStringValue_(s[6:]);continue
   previous=self.output.string();self.output.setString_((previous+s+'\n')[-16000:]);self.output.scrollRangeToVisible_((len(self.output.string()),0))
  self.poll_controller()
 @objc.python_method
 async def connect_async(self):
  if self.client and self.client.is_connected:self.log('Already connected.');return
  self.events.put('STATE Connecting…')
  d=await find(self.profile);self.client=BleakClient(d,disconnected_callback=lambda c:self.disconnected_notice());await self.client.connect();self.toy=Toy(self.client,self.profile);await self.toy.start()
  r=await self.cmd('status');self.supportsLayers='eyes=1' in r;self.lastPose=None
  if self.controllerEnabled:
   if not self.supportsLayers:
    self.controllerEnabled=False;self.poseMailbox.clear();self.log('Controller paused: this build lacks pupil/eyelid controls.')
   else:
    try:await prepare_controller(self.cmd,self.gamepadMapper,self.poseMailbox)
    except Exception:
     self.controllerEnabled=False;self.poseMailbox.stop();raise
  self.events.put('STATE Connected · '+d.name)
 def connectToy_(self,s):self.launch(self.connect_async())
 @objc.python_method
 async def disconnect_async(self):
  if self.client:await self.client.disconnect()
  self.toy=None;self.client=None;self.lastPose=None;self.events.put('STATE Disconnected');self.log('Disconnected; toy continues its behavior.')
 def disconnectToy_(self,s):self.launch(self.disconnect_async())
 @objc.python_method
 async def cmd(self,text):
  if not self.client or not self.client.is_connected:raise RuntimeError('Connect first.')
  start=time.monotonic();r=await self.toy.command((text+';').encode(),'STATUS' if text=='status' else 'OK');self.log(f'{r} ({1000*(time.monotonic()-start):.0f} ms)')
  if self.profile.get('peripherals'):
   seq=r.split('seq=')[1].split()[0];deadline=time.monotonic()+3
   while time.monotonic()<deadline:
    line=await asyncio.wait_for(self.toy.lines.get(),max(.01,deadline-time.monotonic()))
    if line.startswith('PERIPH seq='+seq+' '):self.log(line);break
   if text.startswith(('face ','eyes ')):
    while time.monotonic()<deadline:
     line=await asyncio.wait_for(self.toy.lines.get(),max(.01,deadline-time.monotonic()))
     if line.startswith('FACE seq='+seq+' '):self.log(line);break
    else:raise RuntimeError('Face state was not acknowledged.')
  return r
 def checkToy_(self,s):self.launch(self.cmd('status'))
 def blinkToy_(self,s):self.launch(self.cmd('mode blink'))
 def onToy_(self,s):self.launch(self.cmd('mode on'))
 def offToy_(self,s):self.launch(self.cmd('mode off'))
 def resetToy_(self,s):self.launch(self.cmd('reset'))
 def applyToy_(self,s):
  try:
   period=int(self.period.stringValue());duty=int(self.duty.stringValue())
   if not 100<=period<=10000 or not 1<=duty<=99:raise ValueError('Period 100–10000 ms; duty 1–99%.')
  except ValueError as e:self.log(e);return
  async def apply():await self.cmd(f'period {period}');await self.cmd(f'duty {duty}')
  self.launch(apply())
 def uploadToy_(self,s):
  if self.busy:self.log('Wait for the current operation.');return
  panel=NSOpenPanel.openPanel();panel.setAllowedFileTypes_(['bin']);panel.setCanChooseDirectories_(False);panel.setAllowsMultipleSelection_(False)
  if panel.runModal()!=1:return
  self.launch(self.upload_async(Path(panel.URL().path())))
 @objc.python_method
 async def upload_async(self,path):
  if not self.client or not self.client.is_connected:raise RuntimeError('Connect first.')
  data,m=load_image(path,self.profile)
  key=bytes.fromhex((self.root/'backup/ota-key.hex').read_text().strip())
  self.log(f'Uploading build {m["build"]}: {len(data)} bytes. Keep toy powered.');self.events.put('STATE Updating…')
  start=time.monotonic()
  try:await self.toy.upload(data,key,progress=self.log,fast=True)
  except Exception:
   with contextlib.suppress(Exception):await self.toy.op(b'\4')
   with contextlib.suppress(Exception):await self.client.disconnect()
   self.client=None;self.toy=None;self.log('Update not verified. Reconnect to check the running build, then retry from the beginning.');raise
  await self.client.disconnect();self.client=None;self.toy=None
  await asyncio.sleep(2);await self.connect_async();r=await self.toy.command(b'status;','STATUS')
  if f'build={m["build"]} ' not in r:raise RuntimeError('Reconnected firmware does not match expected build.')
  self.lastPose=None
  self.log(f'Update verified: {r}; total {time.monotonic()-start:.1f} s');self.events.put('STATE Connected · update verified')
 def chooseBoard_(self,s):
  chosen=self.board.indexOfSelectedItem()
  if self.busy or (self.client and self.client.is_connected):self.board.selectItemAtIndex_(self.selected);self.log('Disconnect before changing toys.');return
  self.controllerEnabled=False;self.controllerButton.setTitle_('Enable controller');self.lastPose=None;self.poseMailbox.clear()
  self.selected=chosen;self.root=PROJECTS[chosen];self.profile=json.loads((self.root/'device.json').read_text());self.events.put('STATE Disconnected · selected '+self.profile['name']);self.log('Selected '+self.profile['name'])
  for control in self.peripheralControls:control.setEnabled_(bool(self.profile.get('peripherals')))
 def eyesToy_(self,s):self.launch(self.cmd('screen eyes'))
 def closedToy_(self,s):self.launch(self.cmd('screen closed'))
 def testScreenToy_(self,s):self.launch(self.cmd('screen test'))
 def screenOffToy_(self,s):self.launch(self.cmd('screen off'))
 def touchOnToy_(self,s):self.launch(self.cmd('touch on'))
 def touchOffToy_(self,s):self.launch(self.cmd('touch off'))
 def contrastToy_(self,s):
  try:
   value=int(self.contrast.stringValue())
   if not 0<=value<=255:raise ValueError("Brightness 0–255.")
  except ValueError as e:self.log(e);return
  self.launch(self.cmd(f"contrast {value}"))
 def thresholdToy_(self,s):
  try:
   value=int(self.threshold.stringValue())
   if not 1000<=value<=2000000:raise ValueError('Threshold 1000–2000000.')
  except ValueError as e:self.log(e);return
  self.launch(self.cmd(f'touch threshold {value}'))
 def buildToy_(self,s):self.launch(self.build_async())
 @objc.python_method
 async def build_async(self):
  if not self.client or not self.client.is_connected:raise RuntimeError('Connect first.')
  label='Workshop-'+time.strftime('%Y%m%d-%H%M%S');self.log('Building current toy source…');self.events.put('STATE Building…')
  process=await asyncio.create_subprocess_exec(sys.executable,str(self.root/'scripts/project.py'),'build','--label',label,stdout=asyncio.subprocess.PIPE,stderr=asyncio.subprocess.STDOUT)
  output,_=await process.communicate();(self.root/'evidence'/('build-'+label+'.log')).write_bytes(output)
  if process.returncode:raise RuntimeError('Build failed; see the saved build log. Nothing uploaded.')
  await self.upload_async(self.root/'firmware/ToyBLE/build'/label/'ToyBLE.ino.bin')
 @objc.python_method
 def poll_controller(self):
  if self.controllerEnabled and self.supportsLayers:
   controllers=[c for c in GCController.controllers() if c.extendedGamepad() is not None]
   pose=None
   if len(controllers)>1:
    self.controllerLabel.setStringValue_('More than one controller: disconnect extras.');pose=self.gamepadMapper.disconnected();self.hadController=False
   elif controllers:
    controller=controllers[0];pad=controller.extendedGamepad();GCController.setShouldMonitorBackgroundEvents_(True)
    buttons={name:bool(getattr(pad,'button'+name)().isPressed()) for name in ['A','B','X','Y']}
    pose=self.gamepadMapper.sample(pad.leftThumbstick().yAxis().value(),pad.rightThumbstick().xAxis().value(),pad.rightThumbstick().yAxis().value(),buttons,dpad=(pad.dpad().xAxis().value(),pad.dpad().yAxis().value()))
    self.hadController=True;self.controllerLabel.setStringValue_((controller.vendorName() or 'Controller')+' · '+pose.command())
   else:
    self.controllerLabel.setStringValue_('Waiting for a controller recognized by macOS…')
    if self.hadController:pose=self.gamepadMapper.disconnected();self.hadController=False
   if pose is not None:self.poseMailbox.offer(pose)
  request=self.poseMailbox.peek()
  if request is None:return
  if self.busy or not self.client or not self.client.is_connected or not self.profile.get('peripherals') or not self.supportsLayers:return
  if request.pose==self.lastPose:self.poseMailbox.ack(request);return
  if time.monotonic()-self.lastPoseAt<.1:return
  self.lastPoseAt=time.monotonic()
  async def send():
   try:await self.cmd(request.pose.command());self.lastPose=request.pose;self.poseMailbox.ack(request)
   except Exception:
    self.controllerEnabled=False;self.events.put('Controller input paused after a communication failure.')
    with contextlib.suppress(Exception):await stop_after_failure(self.poseMailbox,self.client.disconnect)
    raise
  self.launch(send())
 @objc.python_method
 def disconnected_notice(self):
  self.lastPose=None;self.supportsLayers=False;self.events.put('STATE Disconnected · toy continues')
 def controllerToy_(self,s):
  if not self.profile.get('peripherals'):self.log('Select the OLED + touch toy for face control.');return
  if not self.controllerEnabled and self.busy:self.log('Wait for the current operation before enabling controller input.');return
  if not self.controllerEnabled and (not self.client or not self.client.is_connected):self.log('Connect the toy first.');return
  if not self.controllerEnabled and not self.supportsLayers:self.log('Update the toy to the pupil/eyelid firmware first.');return
  self.lastPose=None
  if not self.controllerEnabled:
   async def enable():
    await prepare_controller(self.cmd,self.gamepadMapper,self.poseMailbox)
    self.controllerEnabled=True
    self.log('Controller enabled: left stick moves whites vertically; D-pad moves pupils; right stick left/right winks, up closes from bottom, down closes from top. Release opens. A/B/X/Y expressions; touch off acknowledged.')
   self.launch(enable())
  else:
   self.controllerEnabled=False;self.controllerLabel.setStringValue_('Controller input off');self.gamepadMapper.disconnected();self.poseMailbox.stop()
 def applicationShouldTerminateAfterLastWindowClosed_(self,app):return True
 def applicationWillTerminate_(self,n):
  if self.client:
   with contextlib.suppress(Exception):asyncio.run_coroutine_threadsafe(self.client.disconnect(),self.loop).result(timeout=3)
if __name__=='__main__':
 app=NSApplication.sharedApplication();app.setActivationPolicy_(NSApplicationActivationPolicyRegular);controller=Controller.alloc().init();app.setDelegate_(controller);controller.applicationDidFinishLaunching_(None);app.run()
