import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
import controller_input as c
class DoodleTests(unittest.TestCase):
 def mapper(self):
  self.assertTrue(hasattr(c,'DoodleGamepadMapper'),'Missing doodle face puppetry mapping')
  return c.DoodleGamepadMapper()
 def test_independent_four_stick_axes(self):
  p=self.mapper().sample(1,-1,-1,1,0,0,{})
  self.assertEqual(p.command(),'doodle -100 100 100 -100 0 0 0 neutral')
 def test_five_sticky_expression_presets(self):
  m=self.mapper()
  for button,expression,jaw in [('A','happy',25),('X','surprised',92),('B','angry',0),('Y','sorry',0),('RB','nervous',0)]:
   p=m.sample(0,0,0,0,0,0,{button:True});self.assertEqual((p.expression,p.jaw),(expression,jaw))
   self.assertEqual(m.sample(0,0,0,0,0,0,{}).expression,expression)
 def test_trigger_jaw_blink_and_dpad_winks(self):
  m=self.mapper();p=m.sample(0,0,0,0,.5,1,{})
  self.assertEqual((p.jaw,p.left,p.right),(100,50,50))
  self.assertEqual(m.sample(0,0,0,0,0,0,{},dpad=(-1,0)).command(),'doodle 0 0 0 0 0 100 0 neutral')
  self.assertEqual(m.sample(0,0,0,0,0,0,{},dpad=(1,0)).right,100)
 def test_neutral_stop_keeps_new_generation(self):
  m=self.mapper();m.sample(1,1,1,1,1,1,{'RB':True});neutral=m.disconnected()
  self.assertEqual(neutral.command(),'doodle 0 0 0 0 0 0 0 neutral')
  box=c.PoseMailbox(neutral=neutral);old=box.offer(m.sample(1,1,1,1,1,1,{}));box.stop();box.ack(old)
  self.assertEqual(box.peek().pose,neutral)
 def test_deadzone_clamping_and_nonfinite_input(self):
  m=self.mapper();self.assertEqual(m.sample(.1,-.1,.1,-.1,0,0,{}).command(),'doodle 0 0 0 0 0 0 0 neutral')
  self.assertEqual(m.sample(2,0,0,0,-2,2,{}).brow,100)
  with self.assertRaises(ValueError):m.sample(float('nan'),0,0,0,0,0,{})
 def test_neutral_shoulder_preset(self):
  m=self.mapper();m.sample(0,0,0,0,0,0,{'RB':True})
  self.assertEqual(m.sample(0,0,0,0,0,0,{'LB':True}).expression,'neutral')
if __name__=='__main__':unittest.main()
