"""Export Tab5's real background/comparison log over native USB; no test start."""
from pathlib import Path
import argparse
from monitor_io import open_monitor,monitor_status
from verify_runtime_observer import export_csv
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path);args=p.parse_args();args.output.parent.mkdir(parents=True,exist_ok=True)
 with open_monitor() as s:
  monitor_status(s);rows=export_csv(s,args.output)
 print(f'Saved {len(rows)} observed points to {args.output.resolve()}')
