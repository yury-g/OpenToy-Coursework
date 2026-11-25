"""Screenshot transport: preserve partial lines and megabyte frame boundaries."""
import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from device_io import read_line
class Fake:
 def __init__(self,chunks):self.chunks=list(chunks)
 @property
 def in_waiting(self):return len(self.chunks[0]) if self.chunks else 0
 def read(self,n):
  if not self.chunks:return b''
  b=self.chunks.pop(0);out=b[:n]
  if b[n:]:self.chunks.insert(0,b[n:])
  return out
class CaptureTransport(unittest.TestCase):
 def test_partial_lines_and_queued_response(self):
  s=Fake([b'SCREEN_BE',b'GIN 2 1\n0000ffff\nSCREEN_END\nSYNC next\n'])
  self.assertEqual([read_line(s) for _ in range(4)],[b'SCREEN_BEGIN 2 1\n',b'0000ffff\n',b'SCREEN_END\n',b'SYNC next\n'])
 def test_full_resolution(self):
  row=b'0f1e'*1280+b'\n';raw=b'SCREEN_BEGIN 1280 720\n'+row*720+b'SCREEN_END\n'
  s=Fake([raw[n:n+997] for n in range(0,len(raw),997)])
  self.assertEqual(read_line(s),b'SCREEN_BEGIN 1280 720\n')
  for _ in range(720):self.assertEqual(read_line(s),row)
  self.assertEqual(read_line(s),b'SCREEN_END\n')
if __name__=='__main__':unittest.main()
