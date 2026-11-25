#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Locally serve the documented Face v2 comparison. No toy/network APIs."""
import argparse
from http.server import BaseHTTPRequestHandler,HTTPServer
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--port',type=int,default=0);a=p.parse_args()
class Handler(BaseHTTPRequestHandler):
 def do_GET(self):
  if self.path!='/':self.send_error(404);return
  fragment=(ROOT/'docs/simulations/face-v2-fragment.html').read_text()
  page=('''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>OpenToy Face v2 comparison</title><style>
:root{color-scheme:light dark;--foreground:light-dark(#15202b,#eee);--background:light-dark(#fff,#191919);--muted-foreground:light-dark(#566,#bbb)}
body{font:14px system-ui,sans-serif;color:var(--foreground);background:var(--background);margin:1rem auto;padding:0 1rem;max-width:736px}button,input{font:inherit}button{padding:.5rem .75rem;border-radius:.6rem;border:1px solid light-dark(#ccd,#555);color:inherit;background:light-dark(#eee,#333)}button[aria-pressed=true]{background:light-dark(#222,#ddd);color:light-dark(#fff,#111)}.viz-row{display:flex;align-items:center;flex-wrap:wrap;gap:.5rem;margin:.5rem 0}.text-muted{color:var(--muted-foreground)}.text-small{font-size:12px}.form-label{display:block;margin:.5rem 0}.form-range{width:100%;margin:.25rem 0}.form-check{display:flex;align-items:center;gap:.3rem}.tabular-nums{font-variant-numeric:tabular-nums}
</style>'''+fragment+'</html>').encode()
  self.send_response(200);self.send_header('Content-Type','text/html; charset=utf-8');self.send_header('Content-Length',str(len(page)));self.end_headers();self.wfile.write(page)
 def log_message(self,*args):pass
server=HTTPServer(('127.0.0.1',a.port),Handler);print('http://127.0.0.1:%d/'%server.server_port,flush=True)
try:server.serve_forever()
except KeyboardInterrupt:server.server_close()
