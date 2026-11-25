import sys,json,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from test_device_io import Fake
from unittest.mock import patch
from camera_io import camera_status
class CameraFake(Fake):
 def write(self,data):
  token=data.decode().strip().split(' ',1)[-1]
  self.chunks.append(('SYNC '+token+'\n'+json.dumps({'mac':'02:00:00:00:00:0b','role':'camera','frame_seq':99})+'\n').encode());return len(data)
 def flush(self):pass
class CameraBarrier(unittest.TestCase):
 def test_queued_status_is_not_a_fresh_command_response(self):
  s=CameraFake([b'{"mac":"02:00:00:00:00:0b","role":"camera","frame_seq":1}\n'])
  self.assertEqual(camera_status(s)['frame_seq'],99)
