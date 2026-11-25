"""Charging-only Tab5 smoke test. Never starts a real comparison or changes reporters.

Exercises the same hit tests as touch, exports the actual log, verifies a checked
checkpoint and returns to Faces. The demo is synthetic and stays isolated.
"""
from pathlib import Path
import sys,json,time,csv,io,datetime
from monitor_io import *
ROOT=Path(__file__).resolve().parents[1]

def export_csv(s,path):
 command(s,'race export');end=time.monotonic()+20;lines=[];started=False
 while time.monotonic()<end:
  line=read_line(s).decode(errors='replace').strip()
  if line=='RUNTIME_CSV_BEGIN':started=True
  elif line=='RUNTIME_CSV_END' and started:break
  elif started:lines.append(line)
 else:raise RuntimeError('Incomplete runtime CSV export')
 data='\n'.join(lines)+'\n';Path(path).write_text(data)
 rows=list(csv.DictReader(io.StringIO('\n'.join(x for x in lines if not x.startswith('#')))))
 assert rows and {r['color'] for r in rows}=={'blue','green','orange','silver'}
 assert {r['dataset'] for r in rows}=={'comparison','background'}
 return rows

def verify():
 proof={'recorded_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'scope':'USB charging smoke only; no real race start, unplugging, load or reporter reset','checks':{}}
 with open_monitor() as s:
  initial=monitor_status(s);proof['initial']=initial;phase=fields(initial['RUNTIME']);assert phase['phase']=='WAITING' and phase['session']=='0'
  for color in ('blue','green','orange','silver'):assert fields(initial['NODE'][color])['fresh']=='1'
  # Synthetic tap coordinates share physical touch's release-handler.
  command(s,'ui tap 700 30');race=monitor_status(s);assert fields(race['RUNTIME'])['page']=='race'
  # Current low-charge readiness must make Start inert, without entering a session.
  if int(fields(race['RUNTIME'])['ready'])<4:
   command(s,'ui tap 700 680');assert fields(monitor_status(s)['RUNTIME'])['session']=='0'
  monitor_screen(s,ROOT/'outputs/v19-tab5-race-waiting.png',full=True)
  command(s,'ui tap 1120 30');demo=monitor_status(s);assert fields(demo['RUNTIME'])['demo']=='1';assert fields(demo['RUNTIME'])['session']=='0'
  monitor_screen(s,ROOT/'outputs/v19-tab5-race-demo.png',full=True)
  command(s,'ui tap 700 680');back=monitor_status(s);assert fields(back['RUNTIME'])['demo']=='0'
  command(s,'ui tap 920 30');log=monitor_status(s);assert fields(log['RUNTIME'])['page']=='log'
  monitor_screen(s,ROOT/'outputs/v19-tab5-background-log.png',full=True)
  rows=export_csv(s,ROOT/'outputs/runtime-observer-v19.csv');proof['checks']['csv_rows']=len(rows)
  # Smoke readings are actual USB-era voltages, not synthetic fixture values.
  for color in ('blue','green','orange','silver'):
   bg=[r for r in rows if r['color']==color and r['dataset']=='background'];assert bg
   assert abs(float(bg[-1]['volts'])-float(fields(log['NODE'][color])['volts']))<.15
  command(s,'race save');saved=monitor_status(s);assert fields(saved['RUNTIME'])['saved']=='1'
  command(s,'ui tap 550 30');final=monitor_status(s);r=fields(final['RUNTIME']);assert r['page']=='faces' and r['demo']=='0' and r['phase']=='WAITING' and r['session']=='0'
  monitor_screen(s,ROOT/'outputs/v19-tab5-screen.png',full=True)
  proof['final']=monitor_status(s)
  for color in ('blue','green','orange','silver'):assert fields(proof['final']['NODE'][color])['fresh']=='1'
  proof['checks'].update({'four_live_sources':True,'shared_touch_hit_tests':True,'physical_touch_not_manually_confirmed':True,'demo_isolated':True,'start_blocked_while_charging_low_members':int(r['ready'])<4,'checkpoint_write_readback':True,'final_faces_waiting_no_real_race':True})
 (ROOT/'outputs/runtime-observer-v19-live.json').write_text(json.dumps(proof,indent=2)+'\n')
 print(json.dumps({'checks':proof['checks'],'runtime':proof['final']['RUNTIME']},indent=2))
if __name__=='__main__':verify()
