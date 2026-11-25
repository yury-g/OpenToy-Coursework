"""Find bench helpers stay deterministic and never need a device."""
import contextlib
import io
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import verify_find_me as verifier


class FindBenchHelpers(unittest.TestCase):
    def test_centers_only_fresh_faces_in_registry_order(self):
        report = {'NODE': {name: 'NODE fresh=1' for name in verifier.FACE_ORDER}}
        self.assertEqual(verifier.card_center('blue', report), (218, 172))
        self.assertEqual(verifier.card_center('green', report), (639, 172))
        report['NODE']['pink'] = 'NODE fresh=0'
        self.assertNotEqual(verifier.card_center('silver', report), (850, 604))
        report['NODE']['blue'] = 'NODE fresh=0'
        with self.assertRaises(verifier.TestAbort):
            verifier.card_center('blue', report)

    def test_missing_run_flag_cannot_open_hardware(self):
        with patch.object(sys, 'argv', ['verify_find_me.py']), \
             patch.object(verifier, 'open_monitor', side_effect=AssertionError('opened hardware')) as opened:
            with self.assertRaises(SystemExit), contextlib.redirect_stderr(io.StringIO()):
                verifier.main()
            opened.assert_not_called()

    def test_one_shot_sends_start_once(self):
        with patch.object(verifier, 'command') as send, \
             patch.object(verifier, 'read_line', return_value=b'FIND_ONCE accepted=1 lease_ms=6000 session=4\n'):
            self.assertEqual(verifier.once(object(), 'green')['session'], '4')
            self.assertEqual(send.call_count, 1)
            self.assertEqual(send.call_args.args[1], 'find once green')


if __name__ == '__main__':
    unittest.main()
