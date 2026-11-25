"""Read-only fresh diagnostics for connected enrolled members; Green is excluded."""
from device_io import *
from camera_io import camera_status
from pathlib import Path
import concurrent.futures,datetime,json
ROOT=Path(__file__).resolve().parents[1]
def read_one(color):
 try:
  try:s=open_device(color)
  except AssertionError:return {'color':color,'mac':MACS[color],'available':False,'note':'USB unavailable'}
  with s:
   if color=='teal':r={'CAMERA_JSON':camera_status(s)}
   else:
    r=status(s)
    assert f'build={BUILD}' in r['FACE'] and f'mac={MACS[color]}' in r['FACE'],r
    assert fields(r['BATT'])['valid']==('0' if color=='purple' else '1'),r
  return {'color':color,'mac':MACS[color],'available':True,'recorded_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'latest':r}
 except Exception as e:return {'color':color,'mac':MACS[color],'error':str(e)}
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as ex:devices=list(ex.map(read_one,MACS))
result={'recorded_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'devices':devices,'limits':'Orange measures signed cell current. Blue has gauge SOC/voltage; watch boards and Pink estimate SOC from voltage. Purple/Teal have no battery gauge. No health/capacity certification or cross-reset charge-gain claim.'}
(ROOT/'outputs/family-device-latest-v16.json').write_text(json.dumps(result,indent=2)+'\n')
for d in devices:
 r=d.get('latest',{});print(d['color'],d.get('error') or r.get('BATT') or r.get('CAMERA_JSON') or 'USB unavailable')
if any('error' in d for d in devices):raise SystemExit(1)
