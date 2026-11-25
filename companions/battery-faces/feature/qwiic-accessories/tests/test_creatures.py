import contextlib
import io
import sys
import threading
import time
import unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import test_qwiic_accessories as shared
import verify_creatures as creatures


def envelope_samples():
    result = []
    for cycle in range(3):
        for elapsed in range(0, 9500 if cycle < 2 else 1000, 200):
            phase = 'inhale' if elapsed < 3500 else 'exhale' if elapsed < 8000 else 'rest'
            position = elapsed / 3500 if phase == 'inhale' else (8000 - elapsed) / 4500 if phase == 'exhale' else 0
            drive = int(20 * position * position * (3 - 2 * position) + .5)
            c = dict(enabled='1', active='1', seed='100', peak='20', drive=str(drive), phase=phase,
                     inhale_ms='3500', exhale_ms='4500', rest_ms='1500', cycle_elapsed_ms=str(elapsed), held='0')
            result.append({'creature': c, 'motor': {'at_ms': str(10000 + cycle * 9500 + elapsed), 'drive': str(drive)}})
    return result


class CreatureVerifier(unittest.TestCase):
    def test_fresh_classified_supply_limit_is_allowed_by_bench_guard(self):
        h = dict(present='1', ready='1', io='1', chip='ba', events='20', warning_diag='40')
        shared.safe(h)
        h['warning_diag'] = '80';shared.safe(h)
        h['warning_diag'] = 'c0';shared.safe(h)

    def test_temperature_unknown_or_critical_fault_is_still_rejected(self):
        h = dict(present='1', ready='1', io='1', chip='ba', events='20', warning_diag='40')
        for events, diagnostic in [('20','48'),('20','50'),('20','00'),('a0','40'),('22','40')]:
            h.update(events=events, warning_diag=diagnostic)
            with self.assertRaises(shared.TestAbort):shared.safe(h)

    def test_shared_sample_keeps_creature_and_final_motor(self):
        lines = ['HAPTIC_SAMPLE at_ms=1 volts=3.9 pct=80', 'BREATH active=0',
                 'CREATURE enabled=1 active=1 seed=55', 'QWIIC_BUTTON pressed=0']
        with patch.object(shared, 'rpc', return_value=({'active': '1'}, lines)):
            self.assertEqual(shared.sample(object())['creature']['seed'], '55')

    def test_two_complete_cycles_and_partial_third(self):
        result = creatures.validate_continuous(envelope_samples())
        self.assertEqual(result['complete_cycles'], 2)
        self.assertEqual(result['seed'], 100)

    def test_strong_profile_keeps_smooth_shape_with_higher_peaks(self):
        rows = envelope_samples()
        for row in rows:
            c = row['creature'];c['peak'] = '80'
            phase = c['phase'];elapsed = int(c['cycle_elapsed_ms'])
            fraction = elapsed / 3500 if phase == 'inhale' else (8000 - elapsed) / 4500 if phase == 'exhale' else 0
            c['drive'] = row['motor']['drive'] = str(int(80 * fraction * fraction * (3 - 2 * fraction) + .5))
        result = creatures.validate_continuous(rows, strong=True)
        self.assertEqual(result['complete_cycles'], 2)
        with self.assertRaises(shared.TestAbort):
            creatures.validate_continuous(rows)

    def test_jump_or_missing_rest_fails(self):
        rows = envelope_samples();rows[6]['creature']['drive'] = '28'
        with self.assertRaises(shared.TestAbort):
            creatures.validate_continuous(rows)
        rows = [x for x in envelope_samples() if x['creature']['phase'] != 'rest']
        with self.assertRaises(shared.TestAbort):
            creatures.validate_continuous(rows)

    def test_creature_led_readback_tracks_drive(self):
        row = {'creature': {'active': '1', 'drive': '19'}, 'button': {'pressed': '0', 'led': '114', 'led_readback': '114'}}
        creatures.validate_creature_led(row)
        row['button']['led_readback'] = '16'
        with self.assertRaises(shared.TestAbort):
            creatures.validate_creature_led(row)
        row['creature']['drive'] = '0';row['button']['led'] = row['button']['led_readback'] = '0'
        creatures.validate_creature_led(row)

    def test_tab_io_is_bounded_without_requiring_same_instant_as_board(self):
        creatures.validate_io_status({'io_known': '1', 'mode': 'breathing', 'drive': '28'})
        with self.assertRaises(shared.TestAbort):
            creatures.validate_io_status({'io_known': '1', 'mode': 'breathing', 'drive': '128'})
        with self.assertRaises(shared.TestAbort):
            creatures.validate_io_status({'io_known': '0', 'mode': 'unknown', 'drive': '0'})

    def test_find_capture_requires_low_pulse_and_finding_status(self):
        trial = {'pulses_before': 10, 'samples': [
            {'creature': {'enabled': '1', 'active': '0'},
             'motor': {'active': '1', 'drive': '20', 'duration_ms': '180'}}],
            'monitor': [{'ACCESSORY': {'blue': 'ACCESSORY finding=1'}}]}
        creatures.validate_find_capture(trial, 'blue', 11)
        trial['samples'][0]['motor']['drive'] = '40'
        with self.assertRaises(shared.TestAbort):
            creatures.validate_find_capture(trial, 'blue', 11)
        trial['samples'] = []
        with self.assertRaises(shared.TestAbort):
            creatures.validate_find_capture(trial, 'blue', 11)

    def test_resume_capture_allows_transition_then_requires_final_local_and_tab_state(self):
        trial = {'samples': [
            {'creature': {'enabled': '1', 'active': '0'}},
            {'creature': {'enabled': '1', 'active': '1'}}],
            'monitor': [
                {'ACCESSORY': {'blue': 'ACCESSORY finding=1 pending=2'}},
                {'ACCESSORY': {'blue': 'ACCESSORY finding=0 pending=0'}}]}
        creatures.validate_resume_capture(trial, 'blue')
        trial['samples'][-1]['creature']['active'] = '0'
        with self.assertRaises(shared.TestAbort):
            creatures.validate_resume_capture(trial, 'blue')
        trial['samples'][-1]['creature']['active'] = '1'
        trial['monitor'][-1]['ACCESSORY']['blue'] = 'ACCESSORY finding=1 pending=0'
        with self.assertRaises(shared.TestAbort):
            creatures.validate_resume_capture(trial, 'blue')
        trial['monitor'][-1]['ACCESSORY']['blue'] = 'ACCESSORY finding=0 pending=0'
        trial['samples'][0]['creature']['enabled'] = '0'
        with self.assertRaises(shared.TestAbort):
            creatures.validate_resume_capture(trial, 'blue')

    def test_baseline_stops_owner_mode_before_reading_initial_status(self):
        calls, proof = [], {}
        h = dict(present='1', ready='1', io='1', chip='ba', events='00', active='0', drive='0', mode='0', errors='0', at_ms='10', pulses='0')
        report = {'FACE': 'FACE build=BatteryFaces-v25-accessory-outline mac=' + creatures.MACS['blue'],
                  'HAPTIC': 'HAPTIC ' + ' '.join(k + '=' + v for k, v in h.items()),
                  'QWIIC_BUTTON': 'QWIIC_BUTTON presses=0 errors=0'}
        reading = {'motor': h, 'cell': {'volts': '4.0'}, 'creature': {'enabled': '0', 'active': '0'}}
        def stop(board, cmd):
            calls.append(cmd)
            return h, ['HAPTIC_STOP accepted=1']
        def initial(board):
            calls.append('status')
            return report
        with patch.object(creatures, 'rpc', side_effect=stop),              patch.object(creatures, 'status', side_effect=initial),              patch.object(creatures.CreatureBench, 'reading', return_value=reading),              patch.object(creatures.CreatureBench, 'meta', return_value={}):
            creatures.CreatureBench(object(), object(), 'blue', proof, lambda: None)
        self.assertEqual(calls, ['haptic stop', 'status'])
        self.assertEqual(proof['baseline_stop']['motor']['active'], '0')

    def test_delayed_metadata_does_not_starve_board_sampling(self):
        started, sampled = threading.Event(), threading.Event()
        counts = {'active': 0, 'max_active': 0, 'calls': 0, 'reads_during_meta': 0}
        def reading():
            if started.is_set():
                counts['reads_during_meta'] += 1
                sampled.set()
            return {'motor': {'at_ms': str(time.monotonic_ns() // 1000000)}}
        def delayed_meta():
            counts['active'] += 1
            counts['max_active'] = max(counts['max_active'], counts['active'])
            counts['calls'] += 1
            sampled.clear();started.set()
            try:
                # A synchronous metadata collector cannot satisfy this boundary.
                if not sampled.wait(.5):
                    raise AssertionError('board sampling stopped while metadata was pending')
                time.sleep(.015)  # Represents slow Tab5 serial I/O, with no hardware.
                return {'metadata': counts['calls']}
            finally:
                started.clear();counts['active'] -= 1
        result = {'samples': [], 'monitor': []}
        creatures.collect_continuous(reading, delayed_meta, lambda: None, result,
                                     seconds=.08, meta_interval=.01, sample_interval=.001)
        self.assertGreater(counts['reads_during_meta'], 1)
        self.assertEqual(counts['max_active'], 1)
        self.assertEqual(counts['active'], 0)  # Worker joined before the Tab5 socket can be reused.
        self.assertEqual(len(result['monitor']), counts['calls'])
        self.assertGreater(len(result['samples']), len(result['monitor']))

    def test_async_metadata_error_propagates_after_worker_is_drained(self):
        def bad_meta():
            raise shared.TestAbort('source became stale')
        result = {'samples': [], 'monitor': []}
        with self.assertRaisesRegex(shared.TestAbort, 'source became stale'):
            creatures.collect_continuous(lambda: {}, bad_meta, lambda: None, result,
                                         seconds=.05, meta_interval=.001, sample_interval=.001)
        self.assertTrue(result['samples'])

    def test_requires_explicit_run_before_opening_usb(self):
        with patch.object(sys, 'argv', ['verify_creatures.py']), \
             patch.object(creatures, 'open_monitor', side_effect=AssertionError('USB opened')) as opened:
            with self.assertRaises(SystemExit), contextlib.redirect_stderr(io.StringIO()):
                creatures.main()
            opened.assert_not_called()


if __name__ == '__main__':
    unittest.main()
