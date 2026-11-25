import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
import controller_input as c

class EyeLayerTests(unittest.TestCase):
 def mapper(self):
  self.assertTrue(hasattr(c,'LayeredGamepadMapper'),'Missing independent white/pupil/lid mapping')
  return c.LayeredGamepadMapper()
 def test_white_movement_does_not_move_pupils_or_lids(self):
  p=self.mapper().sample(1,0,0,{},dpad=(0,0))
  self.assertEqual(p.command(),'eyes 0 -100 0 0 0 0 bottom neutral')
 def test_dpad_moves_only_pupils(self):
  p=self.mapper().sample(0,0,0,{},dpad=(-1,1))
  self.assertEqual(p.command(),'eyes 0 0 -100 -100 0 0 bottom neutral')
 def test_right_stick_winks_corresponding_eye(self):
  m=self.mapper()
  self.assertEqual(m.sample(0,-1,0,{}).command(),'eyes 0 0 0 0 100 0 bottom neutral')
  self.assertEqual(m.sample(0,1,0,{}).command(),'eyes 0 0 0 0 0 100 bottom neutral')
 def test_up_and_down_close_from_opposite_edges(self):
  m=self.mapper();up=m.sample(0,0,1,{});down=m.sample(0,0,-1,{})
  self.assertEqual((up.left,up.right,up.edge),(100,100,'bottom'))
  self.assertEqual((down.left,down.right,down.edge),(100,100,'top'))
 def test_diagonal_retains_partial_wink_and_both_lids(self):
  p=self.mapper().sample(.575,-1,.575,{'A':True},dpad=(1,-1))
  self.assertEqual(p.command(),'eyes 0 -50 100 100 100 50 bottom happy')
 def test_release_and_stop_are_neutral_with_pupils(self):
  m=self.mapper();m.sample(1,1,-1,{'B':True});p=m.disconnected()
  self.assertEqual(p.command(),'eyes 0 0 0 0 0 0 bottom neutral')
  mailbox=c.PoseMailbox(neutral=p);old=mailbox.offer(m.sample(1,1,1,{}));mailbox.stop();mailbox.ack(old)
  self.assertEqual(mailbox.peek().pose,p)
 def test_deadzone_and_invalid_right_stick(self):
  self.assertEqual(self.mapper().sample(.1,-.1,.1,{}).command(),'eyes 0 0 0 0 0 0 bottom neutral')
  with self.assertRaises(ValueError):self.mapper().sample(0,float('nan'),0,{})

class EnableTests(unittest.IsolatedAsyncioTestCase):
 async def test_failed_touch_prerequisite_preserves_pending_stop(self):
  self.assertTrue(hasattr(c,'prepare_controller'),'Missing acknowledged controller prerequisite')
  mapper=c.LayeredGamepadMapper();mapper.sample(0,0,0,{'A':True});mailbox=c.PoseMailbox(neutral=mapper.disconnected());stop=mailbox.stop()
  async def command(text):raise RuntimeError('Touch command failed')
  with self.assertRaises(RuntimeError):await c.prepare_controller(command,mapper,mailbox)
  self.assertEqual(mailbox.peek(),stop)
 async def test_successful_prerequisite_clears_stale_input_and_expression(self):
  self.assertTrue(hasattr(c,'prepare_controller'),'Missing acknowledged controller prerequisite')
  mapper=c.LayeredGamepadMapper();mailbox=c.PoseMailbox();mailbox.offer(mapper.sample(1,1,1,{'A':True}));events=[]
  async def command(text):events.append(text)
  await c.prepare_controller(command,mapper,mailbox)
  self.assertEqual(events,['touch off']);self.assertIsNone(mailbox.peek());self.assertEqual(mapper.sample(0,0,0,{}).expression,'neutral')

if __name__=='__main__':unittest.main()
