"""Explicit USB-powered Find Me bench check; no hardware access on import.

Run with --run only when deliberately authorizing Blue/Green LOW find pulses.
Each target gets one 20-second UI session and one unrenewed six-second lease.
Never disconnects radios/USB, restarts devices, starts races, or retries START.
"""
from contextlib import ExitStack
from pathlib import Path
import argparse
import datetime
import json
import math
import time
from device_io import MACS, command, fields, open_device, read_line, status
from monitor_io import monitor_status, open_monitor
from test_qwiic_accessories import (TestAbort, exception_record, quiet, require,
                                    rpc, safe, sample)

ROOT = Path(__file__).resolve().parents[1]
FACE_ORDER = ('blue', 'green', 'pink', 'yellow', 'rainbow', 'purple', 'orange', 'silver')


def card_center(color, report):
    """Match the current live-card layout; touch is sent once, never retried."""
    active = [c for c in FACE_ORDER if fields(report['NODE'][c])['fresh'] == '1']
    require(color in active, {'error': 'target battery card unavailable', 'color': color})
    count, best, best_scale = len(active), 1, 0
    for columns in range(1, count + 1):
        rows = math.ceil(count / columns)
        scale = min((1248 - (columns - 1) * 16) / columns / 616,
                    (632 - (rows - 1) * 16) / rows / 304)
        if scale > best_scale:
            best, best_scale = columns, scale
    rows = math.ceil(count / best)
    w, h = (1248 - (best - 1) * 16) // best, (632 - (rows - 1) * 16) // rows
    row, column = divmod(active.index(color), best)
    in_row = count - row * best if row == rows - 1 else best
    offset = (1248 - (in_row * w + (in_row - 1) * 16)) // 2
    return 16 + offset + column * (w + 16) + w // 2, 72 + row * (h + 16) + h // 2


def once(tab, color):
    command(tab, 'find once ' + color)  # Exactly one application START/datagram.
    end = time.monotonic() + 2
    while time.monotonic() < end:
        line = read_line(tab).decode(errors='replace').strip()
        if line.startswith('FIND_ONCE '):
            result = fields(line)
            require(result['accepted'] == '1' and result['lease_ms'] == '6000', result)
            return result
    raise TestAbort('Missing FIND_ONCE acknowledgement; START is never retried')


class Bench:
    def __init__(self, tab, board, color, proof, checkpoint):
        self.tab, self.board, self.color = tab, board, color
        self.proof, self.checkpoint = proof, checkpoint
        self.last_tab_uptime = None
        initial = status(board)
        proof['initial'] = initial
        require('v22-qwiic-find-breathing' in initial['FACE'], initial['FACE'])
        require(fields(initial['FACE'])['mac'].lower() == MACS[color], initial['FACE'])
        self.errors = int(fields(initial['HAPTIC'])['errors'])
        self.last_uptime = int(fields(initial['HAPTIC'])['at_ms'])
        self.last_pulses = int(fields(initial['HAPTIC'])['pulses'])
        q = fields(initial['QWIIC_BUTTON'])
        self.button_presses, self.button_errors = int(q['presses']), int(q['errors'])
        baseline = self.reading()
        quiet(baseline['motor'])
        self.baseline_volts = float(baseline['cell']['volts'])
        require(self.baseline_volts >= 3.45, baseline)
        self.baseline_runtime = fields(self.meta()['RUNTIME'])
        checkpoint()

    def reading(self):
        x = sample(self.board)
        h, q = x['motor'], x['button']
        safe(h)
        require(int(h['errors']) == self.errors, {'error': 'driver I2C error', 'sample': x})
        uptime, pulses = int(h['at_ms']), int(h['pulses'])
        require(uptime >= self.last_uptime and pulses >= self.last_pulses,
                {'error': 'Feather reset/counter regression', 'sample': x})
        self.last_uptime, self.last_pulses = uptime, pulses
        require(q['present'] == q['ready'] == q['io'] == '1' and q['pressed'] == '0'
                and int(q['presses']) == self.button_presses and int(q['errors']) == self.button_errors,
                {'error': 'Qwiic button fault or interference', 'sample': x})
        require(x['breath']['active'] == '0', {'error': 'unexpected breathing', 'sample': x})
        volts = float(x['cell']['volts'])
        require(volts >= 3.45 and getattr(self, 'baseline_volts', volts) - volts <= .15,
                {'error': 'cell voltage bound', 'sample': x})
        if h['active'] == '1':
            require(h['drive'] == '20' and h['duration_ms'] == '180' and h['mode'] == '1',
                    {'error': 'find is not LOW/180ms', 'sample': x})
        else:
            quiet(h)
        return x

    def meta(self):
        report = monitor_status(self.tab)
        require('BatteryMonitor-v22-find-me-runtime-observer' in report['MONITOR'], report['MONITOR'])
        uptime = int(fields(report['MONITOR'])['uptime_ms'])
        require(self.last_tab_uptime is None or uptime >= self.last_tab_uptime,
                {'error': 'Tab5 reset', 'report': report})
        self.last_tab_uptime = uptime
        a = fields(report['ACCESSORY'][self.color])
        require(a['fresh'] == a['ready'] == '1' and int(a['age_ms']) < 5000,
                {'error': 'accessory status stale/not ready', 'report': report})
        require(fields(report['NODE'][self.color])['fresh'] == '1', report)
        if hasattr(self, 'baseline_runtime'):
            r = fields(report['RUNTIME'])
            require(r['phase'] == self.baseline_runtime['phase'] and r['session'] == self.baseline_runtime['session'],
                    {'error': 'real runtime race state changed', 'report': report})
        return report

    def observe(self, seconds, trial, expect_active=True):
        start, next_meta = time.monotonic(), 0
        trial['samples'], trial['monitor'] = [], []
        while time.monotonic() - start < seconds:
            elapsed = time.monotonic() - start
            trial['samples'].append(self.reading())
            if elapsed >= next_meta:
                report = self.meta()
                trial['monitor'].append({'elapsed_s': elapsed, 'report': report})
                if expect_active and elapsed > 2:
                    require(fields(report['ACCESSORY'][self.color])['finding'] == '1', report)
                next_meta = elapsed + 1
                self.checkpoint()
            if expect_active:
                # Header navigation uses the exact touch hit tests. No race controls are touched.
                pages = [(2, 'race', 'ui tap 700 30'), (8, 'log', 'ui tap 920 30'),
                         (14, 'demo', 'ui tap 1120 30')]
                for at, name, cmd in pages:
                    if elapsed >= at and name not in trial.setdefault('pages', []):
                        command(self.tab, cmd)
                        r = fields(self.meta()['RUNTIME'])
                        require(r['demo'] == ('1' if name == 'demo' else '0') and
                                (name == 'demo' or r['page'] == name), r)
                        trial['pages'].append(name)
            time.sleep(.06)
        trial['observed_seconds'] = time.monotonic() - start

    def wait_stopped(self, seconds=3):
        end, samples, reports = time.monotonic() + seconds, [], []
        while time.monotonic() < end:
            x, report = self.reading(), self.meta()
            samples.append(x); reports.append(report)
            a = fields(report['ACCESSORY'][self.color])
            if a['finding'] == '0' and a['pending'] == '0' and x['motor']['active'] == '0':
                quiet(x['motor'])
                return {'samples': samples, 'monitor': reports}
            time.sleep(.08)
        raise TestAbort({'error': 'stop not confirmed', 'samples': samples, 'monitor': reports})

    def run(self):
        command(self.tab, 'view faces')
        report = self.meta()
        require(fields(report['ACCESSORY'][self.color])['finding'] == '0', report)
        ui = self.proof['ui'] = {'passed': False, 'pulses_before': self.last_pulses}
        x, y = card_center(self.color, report)
        ui['tap'] = [x, y]
        command(self.tab, f'ui tap {x} {y}')  # Whole-card hit test; one START action only.
        self.observe(20, ui)
        require(10 <= self.last_pulses - ui['pulses_before'] <= 16, ui)
        require(any(x['motor']['active'] == '1' for x in ui['samples']),
                {'error': 'no active LOW pulse sampled', 'trial': ui})
        command(self.tab, 'ui tap 120 30')  # Global STOP on Demo; no target-side stop yet.
        ui['stop'] = self.wait_stopped()
        ui['passed'] = True
        time.sleep(.6)
        lease = self.proof['lease_without_renewal'] = {'passed': False, 'pulses_before': self.last_pulses}
        lease['command'] = once(self.tab, self.color)
        self.observe(8.5, lease, expect_active=False)
        a = fields(lease['monitor'][-1]['report']['ACCESSORY'][self.color])
        require(a['finding'] == '0' and a['pending'] == '0' and a['reason'] == '5'
                and a['session'] == lease['command']['session'] and a['boot'] == lease['command']['target_boot'],
                {'error': 'unrenewed session did not expire by target lease', 'status': a, 'command': lease['command']})
        require(3 <= self.last_pulses - lease['pulses_before'] <= 4, lease)
        quiet(lease['samples'][-1]['motor'])
        lease['passed'] = True
        self.proof['final'] = self.meta()
        self.proof['passed'] = True
        self.checkpoint()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--run', action='store_true', help='Explicitly authorize the bounded actuator bench checks')
    parser.add_argument('--target', choices=('blue', 'green', 'both'), default='both')
    parser.add_argument('--output', type=Path, default=ROOT / 'outputs/find-me-v22-live.json')
    args = parser.parse_args()
    if not args.run:
        parser.error('--run is required; no device has been opened')
    proof = {'recorded_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
             'scope': 'Deliberate USB-powered LOW/180ms Find Me: 20s UI plus one unrenewed6s lease per target',
             'limits': 'No physical temperature/current/coil-rail measurement; no physical-button, reboot or120s bench test',
             'targets': {}, 'complete': False}
    def checkpoint():
        args.output.parent.mkdir(parents=True, exist_ok=True)
        temporary = args.output.with_suffix('.json.tmp')
        temporary.write_text(json.dumps(proof, indent=2) + '\n')
        temporary.replace(args.output)
    with ExitStack() as stack:
        tab = None
        opened = {}
        try:
            tab = stack.enter_context(open_monitor())
            for color in (('blue', 'green') if args.target == 'both' else (args.target,)):
                board = opened[color] = stack.enter_context(open_device(color))
                item = proof['targets'][color] = {'passed': False}
                Bench(tab, board, color, item, checkpoint).run()
                print(color, 'Find UI/STOP and unrenewed lease PASS', flush=True)
            proof['complete'] = True
        except BaseException as exc:
            proof['abort'] = exception_record(exc)
            raise
        finally:
            cleanup = proof['cleanup'] = {}
            if tab:
                try:
                    command(tab, 'find stop'); command(tab, 'view faces')
                    cleanup['tab_stop_requested'] = True
                except BaseException as exc:
                    cleanup['tab_error'] = exception_record(exc)
            for color, board in opened.items():
                try:
                    h, lines = rpc(board, 'haptic stop'); safe(h); quiet(h)
                    cleanup[color] = {'confirmed_inactive': True, 'motor': h, 'lines': lines}
                except BaseException as exc:
                    cleanup[color] = {'error': exception_record(exc)}
            if any('error' in x for x in cleanup.values() if isinstance(x, dict)) or 'tab_error' in cleanup:
                proof['complete'] = False
            checkpoint()
    require(proof['complete'], proof)
    print('PASS; both stop paths confirmed; evidence:', args.output, flush=True)


if __name__ == '__main__':
    main()
