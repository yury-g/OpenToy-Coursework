# SPDX-License-Identifier: MIT
import unittest,asyncio,sys,tempfile,json,hashlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from ble_client import Toy,require,load_image
class FakeClient:
 def __init__(self):self.toy=None
 async def write_gatt_char(self,*args,**kwargs):
  payload=args[1]
  if payload[0]==1:
   self.toy.q.put_nowait('HELLO 1 stale 0');self.toy.q.put_nowait('START 0')
  else:
   self.toy.q.put_nowait('DATA 100');self.toy.q.put_nowait('DATA 10')
class Tests(unittest.IsolatedAsyncioTestCase):
 async def test_late_status_does_not_satisfy_start(self):
  c=FakeClient();t=Toy(c);c.toy=t
  self.assertEqual(await t.op(b'\1'), 'START 0')
 async def test_exact_offset_not_prefix(self):
  c=FakeClient();t=Toy(c);c.toy=t
  self.assertEqual(await t.op(b'\2\x00\x00\x00\x00'+bytes(10)), 'DATA 10')
 async def test_cumulative_ack_skips_missing_intermediate(self):
  t=Toy(FakeClient());t.q.put_nowait('DATA 1440')
  self.assertFalse(await t.wait_data(0,1440,.02))
 async def test_missing_final_ack_checks_device_offset(self):
  t=Toy(FakeClient())
  async def hello(p):return 'HELLO 1 0123456789abcdef 1440'
  t.op=hello
  self.assertTrue(await t.wait_data(0,1440,.02))
 async def test_stalled_offset_cannot_continue(self):
  t=Toy(FakeClient())
  async def hello(p):return 'HELLO 1 0123456789abcdef 720'
  t.op=hello
  with self.assertRaises(RuntimeError):await t.wait_data(0,1440,.02)
 def test_manifest_identity_and_hash(self):
  with tempfile.TemporaryDirectory() as tmp:
   path=Path(tmp)/'app.bin';path.write_bytes(b'example')
   m={'device_mac':'board-A','target':'esp32:esp32:XIAO_ESP32S3','bytes':7,'sha256':hashlib.sha256(b'example').hexdigest(),'build':'test'}
   manifest=path.parent/'manifest.json';manifest.write_text(json.dumps(m))
   self.assertEqual(load_image(path,{'base_mac':'board-A'})[1]['build'],'test')
   with self.assertRaises(RuntimeError):load_image(path,{'base_mac':'board-B'})
   path.write_bytes(b'corrupt')
   with self.assertRaises(RuntimeError):load_image(path,{'base_mac':'board-A'})
   path.write_bytes(b'example');m['rollback_test']=True;manifest.write_text(json.dumps(m))
   with self.assertRaises(RuntimeError):load_image(path,{'base_mac':'board-A'})
   self.assertEqual(load_image(path,{'base_mac':'board-A'},allow_test=True)[0],b'example')
 def test_validation_is_not_assert(self):
  with self.assertRaises(RuntimeError):require(False,'bad image')
if __name__=='__main__':unittest.main()
