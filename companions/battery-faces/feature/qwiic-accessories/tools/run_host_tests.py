"""Compile and run deterministic packet, freshness, geometry and battery tests."""
from pathlib import Path
import subprocess,json,datetime
root=Path(__file__).resolve().parents[1]
zig=Path('/Users/example/Documents/Codex/2026-10-01/handle-this-project-end-to-end/work/zig-aarch64-macos-0.16.0/zig')
results=[]
for p in sorted((root/'tests').glob('*test*.cpp')):
 modes=[False,True,"silver"] if p.stem=='display_test' else [False]
 for watch in modes:
  name=p.stem+('_silver' if watch=='silver' else '_watch' if watch else '')
  args=[str(zig),'c++','-std=c++17']+(['-DFACE_SILVER=1'] if watch=='silver' else ['-DFACE_WAVESHARE_169'] if watch else [])+[str(p),'-o',str(root/'outputs'/name)]
  a=subprocess.run(args,capture_output=True,text=True)
  if a.returncode:print(a.stderr);raise SystemExit(name)
  a=subprocess.run([str(root/'outputs'/name)],capture_output=True,text=True)
  if a.returncode:print(a.stderr);raise SystemExit(name)
  results.append({'test':name,'passed':True});print('PASS',name)
(root/'outputs/host-tests-v26.json').write_text(json.dumps({'recorded_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'tests':results},indent=2)+'\n')
