import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from controller_input import GamepadMapper
class MappingTests(unittest.TestCase):
 def test_dead_zone_stops_stick_drift(self):
  m=GamepadMapper();self.assertEqual(m.sample(.1,-.1,0,0,{}).command(),'face 0 0 0 0 neutral')
 def test_axes_invert_vertical_and_clamp(self):
  self.assertEqual(GamepadMapper().sample(2,1,-1,2,{}).command(),'face 100 -100 0 100 neutral')
 def test_independent_eyelids(self):
  self.assertEqual(GamepadMapper().sample(0,0,.25,.75,{}).command(),'face 0 0 25 75 neutral')
 def test_button_expression_is_sticky_and_edge_triggered(self):
  m=GamepadMapper();self.assertEqual(m.sample(0,0,0,0,{'A':True}).expression,'happy');self.assertEqual(m.sample(0,0,0,0,{}).expression,'happy')
  self.assertEqual(m.sample(0,0,0,0,{'B':True}).expression,'angry');self.assertEqual(m.sample(0,0,0,0,{'X':True}).expression,'surprised');self.assertEqual(m.sample(0,0,0,0,{'Y':True}).expression,'neutral')
 def test_dpad_overrides_thumbstick(self):
  self.assertEqual(GamepadMapper().sample(.5,.5,0,0,{},dpad=(-1,-1)).command(),'face -100 100 0 0 neutral')
 def test_invalid_input_is_rejected(self):
  for value in [float('nan'),float('inf')]:
   with self.assertRaises(ValueError):GamepadMapper().sample(value,0,0,0,{})
 def test_disconnect_returns_neutral_pose(self):
  m=GamepadMapper();m.sample(1,1,1,1,{'A':True});self.assertEqual(m.disconnected().command(),'face 0 0 0 0 neutral')
class PendingInputTests(unittest.TestCase):
 def mailbox(self):
  import controller_input
  self.assertTrue(hasattr(controller_input,'PoseMailbox'),'Missing latest-input/stop delivery policy')
  return controller_input.PoseMailbox()
 def test_stop_survives_acknowledgment_of_inflight_pose(self):
  m=self.mailbox();old=m.offer(GamepadMapper().sample(1,0,0,0,{}));m.stop();m.ack(old);self.assertEqual(m.peek().pose.command(),'face 0 0 0 0 neutral')
 def test_samples_coalesce_to_latest_instead_of_backlog(self):
  m=self.mailbox();m.offer(GamepadMapper().sample(-1,0,0,0,{}));m.offer(GamepadMapper().sample(1,0,0,0,{}));self.assertEqual(m.peek().pose.x,100)
 def test_old_ack_cannot_consume_new_stop_with_identical_pose(self):
  m=self.mailbox();old=m.stop();m.offer(GamepadMapper().sample(1,0,0,0,{}));m.stop();m.ack(old);self.assertIsNotNone(m.peek())
class FailurePolicyTests(unittest.IsolatedAsyncioTestCase):
 async def test_failed_delivery_retains_neutral_and_disconnects(self):
  import controller_input as c
  self.assertTrue(hasattr(c,'stop_after_failure'),'Missing failed-delivery stop policy')
  m=c.PoseMailbox();m.offer(c.GamepadMapper().sample(1,0,0,0,{}));events=[]
  async def disconnect():events.append('disconnected')
  await c.stop_after_failure(m,disconnect)
  self.assertEqual(m.peek().pose.command(),'face 0 0 0 0 neutral');self.assertEqual(events,['disconnected'])
 async def test_disconnect_failure_does_not_discard_pending_stop(self):
  import controller_input as c
  self.assertTrue(hasattr(c,'stop_after_failure'),'Missing failed-delivery stop policy')
  m=c.PoseMailbox();m.offer(c.GamepadMapper().sample(1,0,0,0,{}))
  async def disconnect():raise RuntimeError('transport unavailable')
  with self.assertRaises(RuntimeError):await c.stop_after_failure(m,disconnect)
  self.assertEqual(m.peek().pose.command(),'face 0 0 0 0 neutral')
if __name__=='__main__':unittest.main()
