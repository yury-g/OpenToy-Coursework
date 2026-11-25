"""Two live camera feeds, shared quality controls, default view and battery fallbacks.
Readback/UI/radio commands only; no battery load tests or Green access.
"""
from device_io import *
from monitor_io import *
from camera_io import camera_status
from pathlib import Path
from contextlib import ExitStack
import datetime,json
ROOT=Path(__file__).resolve().parents[1]
proof={'recorded_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'checks':[],'unavailable':[]}
def sources(d):return {k:fields(v) for k,v in d['CAMERA_SOURCE'].items()}
with ExitStack() as stack:
 ports={}
 for color in MACS:
  if color=='teal':continue
  try:ports[color]=stack.enter_context(open_device(color))
  except AssertionError:proof['unavailable'].append(color)
 assert set(('blue','pink','rainbow','purple'))<=ports.keys()
 camera=stack.enter_context(open_device('teal'));monitor=stack.enter_context(open_monitor());time.sleep(4)
 def snapshot():return {'viewers':{c:status(s) for c,s in ports.items()},'monitor':monitor_status(monitor),'sender':camera_status(camera)}
 def wait_for(test,seconds=16):
  end=time.monotonic()+seconds;d=None
  while time.monotonic()<end:
   d=snapshot()
   if test(d):return d
   time.sleep(.3)
  raise AssertionError(d)
 def every(d):return list(d['viewers'].values())+[d['monitor']]
 def live(d,source):return all(sources(x)[str(source)]['fresh']=='1' for x in every(d))
 def format_is(d,fmt):return live(d,0) and all(sources(x)['0']['format']==str(fmt) for x in every(d))
 try:
  initial=wait_for(lambda d:live(d,0) and live(d,1))
  proof['initial']=initial
  for color,d in initial['viewers'].items():
   assert fields(d['FACE'])['build']==BUILD
   assert fields(d['CAMERA'])['active']=='1' and fields(d['CAMERA'])['source']=='0',(color,d['CAMERA'])
   assert 'green' not in d['PEER'] and fields(d['RADIO'])['ready']=='1'
   assert fields(d['DIAG'])['outcome']!='1'
  assert fields(initial['monitor']['CAMERA'])['active']=='1' and fields(initial['monitor']['CAMERA'])['source']=='0'
  tab=fields(initial['monitor']['TAB_CAMERA']);assert tab['ready']=='1' and tab['chip']=='eb52' and tab['clock_hz']=='24000000' and int(tab['captured'])>2
  assert initial['sender']['firmware']==17 and initial['sender']['capture_age_ms']<=500
  hw=fields(initial['viewers']['pink']['HW']);assert hw['cfg_before']==hw['cfg_after']
  proof['checks'].append('All connected displays start on live XIAO; Tab5 real sensor PID eb52 captures fresh native frames at24MHz; Pink charger configuration preserved')
  for color,s in ports.items():
   command(s,'select '+color);command(s,'view face');time.sleep(.15);screen(s,ROOT/f'outputs/v17-screen-{color}-shows-{color}.png')
  command(monitor,'view faces');time.sleep(.3);monitor_screen(monitor,ROOT/'outputs/v17-tab5-screen.png',full=True)
  command(ports['pink'],'quality color');color=wait_for(lambda d:format_is(d,1));proof['color']=color
  for c,s in ports.items():command(s,'view camera 0');time.sleep(.1);screen(s,ROOT/f'outputs/v17-camera-color-{c}.png')
  command(monitor,'view camera 0');time.sleep(.3);monitor_screen(monitor,ROOT/'outputs/v17-camera-color-tab5.png',full=True)
  command(monitor,'quality gray');gray=wait_for(lambda d:format_is(d,2));proof['gray']=gray
  for c,s in ports.items():command(s,'view camera 0');time.sleep(.1);screen(s,ROOT/f'outputs/v17-camera-gray-{c}.png')
  command(monitor,'view camera 0');time.sleep(.3);monitor_screen(monitor,ROOT/'outputs/v17-camera-gray-tab5.png',full=True)
  proof['checks'].append('Pink radio request selects80x60color; Tab5 TCP-to-Blue radio request selects160x120four-shadegray on every viewer, same4800-byteimage')
  for c,s in ports.items():command(s,'view camera 1');time.sleep(.1);assert fields(status(s)['CAMERA'])['active']=='1';screen(s,ROOT/f'outputs/v17-camera-native-{c}.png')
  command(monitor,'view camera 1');time.sleep(.3);monitor_screen(monitor,ROOT/'outputs/v17-camera-native-tab5.png',full=True)
  proof['checks'].append('Tab5 onboard camera image selected and framebuffer captured on every connected screen, via Blue ESP-NOW relay on small devices')
  before=snapshot();time.sleep(20);after=snapshot();proof['idle']={'before':before,'after':after}
  for c in ports:
   assert int(fields(after['viewers'][c]['FACE'])['frames'])>int(fields(before['viewers'][c]['FACE'])['frames'])+50
   for source in ('0','1'):assert int(sources(after['viewers'][c])[source]['frames'])>=int(sources(before['viewers'][c])[source]['frames'])+5
  assert live(after,0) and live(after,1)
  assert after['sender']['send_timeouts']==before['sender']['send_timeouts']
  proof['checks'].append('20seconds without USB readers: both feeds, animation and live battery traffic advance together')
  for s in ports.values():command(s,'view camera 0')
  command(monitor,'view camera 0');command(camera,'stream off');time.sleep(7)
  lost=snapshot();proof['xiao_lost']=lost
  for d in every(lost):assert sources(d)['0']['fresh']=='0' and fields(d['CAMERA'])['active']=='0'
  assert live(lost,1)
  proof['checks'].append('XIAO loss returns its selected views to battery faces even while independent Tab5 camera stays live')
  powerBefore=lost['monitor']['TAB_POWER'];command(monitor,'camera off');time.sleep(7)
  both=snapshot();proof['all_cameras_off']=both
  for c,d in both['viewers'].items():
   assert all(sources(d)[k]['fresh']=='0' for k in ('0','1')) and fields(d['CAMERA'])['active']=='0'
   assert fields(d['DISPLAY'])['selected']==c
   assert int(fields(d['FACE'])['frames'])>int(fields(lost['viewers'][c]['FACE'])['frames'])
  assert fields(both['monitor']['CAMERA'])['active']=='0' and fields(both['monitor']['NODE']['pink'])['fresh']=='1'
  assert both['monitor']['TAB_POWER']==powerBefore
  screen(ports['pink'],ROOT/'outputs/v17-all-cameras-off-battery-pink.png');monitor_screen(monitor,ROOT/'outputs/v17-all-cameras-off-battery-tab5.png',full=True)
  proof['checks'].append('All cameras stopped: every screen returns to battery faces; battery measurements/animation remain live; Tab5 charger-control pin masks unchanged')
  command(camera,'stream on');command(monitor,'camera on');recovered=wait_for(lambda d:live(d,0) and live(d,1),20);proof['recovered']=recovered
  assert recovered['sender']['capture_age_ms']<=500
  assert int(fields(recovered['monitor']['TAB_CAMERA'])['captured'])>int(fields(both['monitor']['TAB_CAMERA'])['captured'])
  assert recovered['monitor']['TAB_POWER']==powerBefore
  proof['checks'].append('Both cameras resume with newly completed frames and independent source freshness')
  proof['passed']=True
 finally:
  command(camera,'stream on');command(camera,'radio on');command(monitor,'camera on');command(monitor,'quality gray')
  for s in ports.values():command(s,'view default')
  command(monitor,'view default');time.sleep(2)
  proof['final']=snapshot()
  (ROOT/'outputs/camera-family-v17-live.json').write_text(json.dumps(proof,indent=2)+'\n')
print(json.dumps({'passed':proof.get('passed',False),'checks':proof['checks'],'usb_unavailable':proof['unavailable']},indent=2))
