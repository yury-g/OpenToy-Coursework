"""Explicit USB-powered creature envelope and Tab5 Find Me bench check.

No device is opened on import or without --run. Each target runs35seconds of
continuous creature breathing, then a short Tab5 find preemption/resume check.
Always attempts Tab5 STOP and direct motor stop. Does not simulate physical
buttons, reset devices, unplug USB, flash firmware, or start a battery race.
"""
from contextlib import ExitStack
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import argparse
import datetime
import json
import time
from device_io import MACS, command, fields, open_device, status
from monitor_io import monitor_status, open_monitor
from test_qwiic_accessories import TestAbort, exception_record, quiet, require, rpc, safe, sample

ROOT = Path(__file__).resolve().parents[1]


def signature(c):
    return tuple(int(c[k]) for k in ('peak', 'inhale_ms', 'exhale_ms', 'rest_ms'))


def validate_creature_led(row):
    c, q = row['creature'], row['button']
    if c['active'] == '1' and q['pressed'] == '0':
        expected = min(int(c['drive']) * 6, 192)
        require(int(q['led']) == int(q['led_readback']) == expected,
                {'error': 'Q button LED does not follow creature intensity', 'expected_led': expected, 'sample': row})


def validate_io_status(a):
    require(a.get('io_known') == '1' and a.get('mode') in ('idle', 'held', 'vibrating', 'breathing') and
            0 <= int(a.get('drive', '-1')) <= 127, {'error': 'Tab5 live IO telemetry invalid', 'status': a})


def validate_continuous(rows, strong=False):
    """Verify two observed complete cycles, phase coverage and bounded smooth readback."""
    require(bool(rows), 'No continuous creature samples')
    seed = int(rows[0]['creature']['seed'])
    cycles, current, previous = [], [], None
    for row in rows:
        c, h = row['creature'], row['motor']
        require(c['enabled'] == c['active'] == '1' and c['held'] == '0', row)
        require(int(c['seed']) == seed and seed != 0, {'error': 'creature identity changed', 'sample': row})
        peak, inhale, exhale, rest = signature(c)
        require((64 <= peak <= 96 if strong else 12 <= peak <= 28) and 3500 <= inhale <= 6000 and
                4500 <= exhale <= 7000 and 1500 <= rest <= 3000, row)
        elapsed, drive = int(c['cycle_elapsed_ms']), int(c['drive'])
        require(0 <= elapsed <= inhale + exhale + rest + 100 and 0 <= drive <= peak and
                int(h['drive']) == drive, row)
        phase = 'inhale' if elapsed < inhale else 'exhale' if elapsed < inhale + exhale else 'rest'
        require(c['phase'] == phase, row)
        fraction = elapsed / inhale if phase == 'inhale' else (inhale + exhale - elapsed) / exhale if phase == 'exhale' else 0
        expected = peak * fraction * fraction * (3 - 2 * fraction)
        # The service updates every40ms; tolerate register quantization and that update lag.
        require(abs(drive - expected) <= (3 if strong else 2), {'error': 'envelope/readback mismatch', 'expected_drive': expected, 'sample': row})
        if phase == 'rest':
            require(drive == 0, {'error': 'rest is not quiet', 'sample': row})
        if previous:
            pc, ph = previous['creature'], previous['motor']
            dt = int(h['at_ms']) - int(ph['at_ms'])
            require(0 <= dt <= 650, {'error': 'sampling gap cannot support smoothness claim', 'gap_ms': dt})
            require(abs(drive - int(pc['drive'])) <= (.045 if strong else .012) * dt + 2,
                    {'error': 'abrupt motor command jump', 'before': previous, 'after': row})
            if elapsed < int(pc['cycle_elapsed_ms']):
                require({x['creature']['phase'] for x in current} == {'inhale', 'exhale', 'rest'},
                        {'error': 'completed cycle missing a phase', 'samples': current})
                cycles.append(current);current = []
            else:
                require(signature(pc) == signature(c), {'error': 'parameters changed within cycle', 'before': previous, 'after': row})
        current.append(row);previous = row
    require(len(cycles) >= 2, {'error': 'fewer than two complete cycles observed', 'complete_cycles': len(cycles)})
    return {'seed': seed, 'complete_cycles': len(cycles),
            'cycles': [{'parameters': signature(group[0]['creature']), 'samples': len(group),
                        'min_drive': min(int(x['creature']['drive']) for x in group),
                        'max_drive': max(int(x['creature']['drive']) for x in group)} for group in cycles]}


def collect_continuous(reading, meta, checkpoint, result, seconds=35, meta_interval=2, sample_interval=.07):
    """Sample the Feather continuously; only Tab5 metadata runs in a worker.

    At most one metadata read is outstanding. Drain/join it before returning so
    later Find Me commands and cleanup cannot race the Tab5 serial connection.
    Sampling continues while a final metadata read finishes after the deadline.
    """
    start, next_meta, pending = time.monotonic(), meta_interval, None
    try:
        with ThreadPoolExecutor(max_workers=1) as executor:
            while time.monotonic() - start < seconds or pending is not None:
                result['samples'].append(reading())
                elapsed = time.monotonic() - start
                if pending is not None and pending.done():
                    report = pending.result()  # Propagate stale-source or serial errors immediately.
                    pending = None
                    result['monitor'].append(report)
                    checkpoint()
                if elapsed < seconds and pending is None and elapsed >= next_meta:
                    pending = executor.submit(meta)
                    next_meta = elapsed + meta_interval
                time.sleep(sample_interval)
    finally:
        result['observed_seconds'] = time.monotonic() - start


def validate_find_capture(trial, color, pulses_after, expected_drive=20):
    low_seen = False
    for x in trial['samples']:
        c, h = x['creature'], x['motor']
        require(c['enabled'] == '1', {'error': 'find cleared creature preference', 'sample': x})
        if c['active'] == '0' and h['active'] == '1':
            require(int(h['drive']) == expected_drive and h['duration_ms'] == '180',
                    {'error': 'find pulse amplitude/duration mismatch', 'sample': x})
            low_seen = True
    finding_seen = any(fields(m['ACCESSORY'][color])['finding'] == '1' for m in trial['monitor'])
    require(low_seen and finding_seen and pulses_after > trial['pulses_before'],
            {'error': 'find preemption/pulse not observed', 'trial': trial})


def validate_resume_capture(trial, color):
    require(bool(trial['samples']) and bool(trial['monitor']), {'error': 'missing resume observations', 'trial': trial})
    for x in trial['samples']:
        require(x['creature']['enabled'] == '1', {'error': 'find stop cleared creature preference', 'sample': x})
    local = trial['samples'][-1]['creature']
    remote = fields(trial['monitor'][-1]['ACCESSORY'][color])
    require(local['active'] == '1' and remote['finding'] == remote['pending'] == '0',
            {'error': 'creature did not resume after find stop', 'trial': trial})


class CreatureBench:
    def __init__(self, tab, board, color, proof, checkpoint):
        self.tab, self.board, self.color = tab, board, color
        self.proof, self.checkpoint = proof, checkpoint
        stopped, stop_lines = rpc(board, 'haptic stop')
        proof['baseline_stop'] = {'motor': stopped, 'lines': stop_lines}
        safe(stopped);quiet(stopped)
        initial = proof['initial'] = status(board)
        face, h, q = fields(initial['FACE']), fields(initial['HAPTIC']), fields(initial['QWIIC_BUTTON'])
        require(face['build'] in ('BatteryFaces-v23-creature-breathing', 'BatteryFaces-v24-accessory-status', 'BatteryFaces-v25-accessory-outline', 'BatteryFaces-v26-strong-haptics', 'BatteryFaces-v27-seesaw-gamepad') and face['mac'].lower() == MACS[color], initial)
        self.strong = face['build'] in ('BatteryFaces-v26-strong-haptics', 'BatteryFaces-v27-seesaw-gamepad')
        self.live_io = self.strong or face['build'] in ('BatteryFaces-v24-accessory-status', 'BatteryFaces-v25-accessory-outline')
        proof['live_io_led_verification'] = self.live_io
        safe(h);quiet(h)
        self.errors, self.uptime, self.pulses = int(h['errors']), int(h['at_ms']), int(h['pulses'])
        self.q_presses, self.q_errors = int(q['presses']), int(q['errors'])
        self.last_tab_uptime, self.target_boot, self.runtime = None, None, None
        self.baseline_volts = None
        first = self.reading()
        require(first['creature']['enabled'] == first['creature']['active'] == '0', first)
        quiet(first['motor'])
        self.baseline_volts = float(first['cell']['volts'])
        self.meta()
        checkpoint()

    def reading(self):
        x = sample(self.board)
        require(all(k in x for k in ('creature', 'button', 'breath')), {'error': 'missing v23 sample field', 'sample': x})
        h, c, q = x['motor'], x['creature'], x['button']
        safe(h)
        require(int(h['errors']) == self.errors and int(h['at_ms']) >= self.uptime and
                int(h['pulses']) >= self.pulses, {'error': 'driver fault, reset or counter regression', 'sample': x})
        self.uptime, self.pulses = int(h['at_ms']), int(h['pulses'])
        require(q['present'] == q['ready'] == q['io'] == '1' and q['pressed'] == '0' and
                int(q['presses']) == self.q_presses and int(q['errors']) == self.q_errors,
                {'error': 'physical input interference or Q button fault', 'sample': x})
        require(c['held'] == '0' and x['breath']['active'] == '0', {'error': 'unexpected held/finite breathing mode', 'sample': x})
        volts = float(x['cell']['volts'])
        require(volts >= 3.45 and (self.baseline_volts is None or self.baseline_volts - volts <= .15),
                {'error': 'cell voltage safety bound', 'baseline_volts': self.baseline_volts, 'sample': x})
        require(int(c['drive']) == int(h['drive']), {'error': 'command/register readback mismatch', 'sample': x})
        require(h['mode'] == ('1' if int(h['drive']) else '0'), {'error': 'operation-mode mismatch', 'sample': x})
        if self.live_io:validate_creature_led(x)
        return x

    def meta(self):
        m = monitor_status(self.tab)
        require(any(label in m['MONITOR'] for label in ('BatteryMonitor-v22-find-me-runtime-observer', 'BatteryMonitor-v24-accessory-status', 'BatteryMonitor-v25-accessory-outline')), m['MONITOR'])
        uptime = int(fields(m['MONITOR'])['uptime_ms'])
        require(self.last_tab_uptime is None or uptime >= self.last_tab_uptime, {'error': 'Tab5 reset', 'report': m})
        self.last_tab_uptime = uptime
        a, r = fields(m['ACCESSORY'][self.color]), fields(m['RUNTIME'])
        require(a['fresh'] == a['ready'] == '1' and int(a['age_ms']) < 5000 and
                fields(m['NODE'][self.color])['fresh'] == '1', {'error': 'fresh source/readiness lost', 'report': m})
        require(self.target_boot is None or a['boot'] == self.target_boot, {'error': 'target rebooted', 'report': m})
        self.target_boot = a['boot']
        require(self.runtime is None or all(r[k] == self.runtime[k] for k in ('phase', 'session')),
                {'error': 'real battery race state changed', 'report': m})
        self.runtime = r
        if self.live_io:validate_io_status(a)
        return m

    def wait_creature(self, rows, seconds=2.5):
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            x = self.reading();rows.append(x)
            if x['creature']['enabled'] == x['creature']['active'] == '1':
                return x
            time.sleep(.06)
        raise TestAbort({'error': 'creature did not become active', 'samples': rows})

    def run(self):
        enabled, lines = rpc(self.board, 'haptic creature on')
        self.proof['enable'] = {'motor': enabled, 'lines': lines}
        require('CREATURE_SET accepted=1 enabled=1' in lines, lines)
        safe(enabled)
        startup = self.proof['startup'] = []
        first = self.wait_creature(startup)
        continuous = self.proof['continuous'] = {'samples': [first], 'monitor': []}
        collect_continuous(self.reading, self.meta, self.checkpoint, continuous)
        continuous['validation'] = validate_continuous(continuous['samples'], self.strong)
        if self.live_io:
            require(any(fields(m['ACCESSORY'][self.color])['mode'] == 'breathing' and
                        int(fields(m['ACCESSORY'][self.color])['drive']) > 0 for m in continuous['monitor']),
                    {'error': 'Tab5 did not display active creature intensity', 'monitor': continuous['monitor']})
            continuous['led_and_tab_telemetry_passed'] = True
        require(int(continuous['samples'][-1]['motor']['pulses']) == int(first['motor']['pulses']),
                {'error': 'unexpected actuator restart during continuous cycles', 'trial': continuous})
        self.checkpoint()
        find = self.proof['find_preemption_resume'] = {'samples': [], 'monitor': [], 'pulses_before': self.pulses}
        command(self.tab, 'find ' + self.color)  # Exactly one explicit action; never retry START.
        collect_continuous(self.reading, self.meta, self.checkpoint, find,
                           seconds=6, meta_interval=.75, sample_interval=.06)
        validate_find_capture(find, self.color, self.pulses, 64 if self.strong else 20)
        command(self.tab, 'find stop')
        find['resume_samples'], find['stop_monitor'] = [], []
        resume = {'samples': find['resume_samples'], 'monitor': find['stop_monitor']}
        # Allow the six-second remote lease,500ms actuator quiet time and UI relay latency.
        collect_continuous(self.reading, self.meta, self.checkpoint, resume,
                           seconds=8, meta_interval=.75, sample_interval=.06)
        find['resume_observed_seconds'] = resume['observed_seconds']
        validate_resume_capture(resume, self.color)
        find['passed'] = True
        h, lines = rpc(self.board, 'haptic stop');safe(h);quiet(h)
        off = self.proof['direct_stop'] = {'motor': h, 'lines': lines, 'samples': []}
        end = time.monotonic() + 1.2
        while time.monotonic() < end:
            x = self.reading();off['samples'].append(x);quiet(x['motor'])
            require(x['creature']['enabled'] == x['creature']['active'] == '0',
                    {'error': 'direct stop allowed creature restart', 'sample': x})
            time.sleep(.08)
        if self.live_io:
            off['monitor'] = [];end = time.monotonic() + 2;idle_seen = False
            while time.monotonic() < end:
                m = self.meta();off['monitor'].append(m);a = fields(m['ACCESSORY'][self.color])
                if a['mode'] == 'idle' and a['drive'] == '0' and a['button_down'] == '0':
                    idle_seen = True;break
                time.sleep(.08)
            require(idle_seen, {'error': 'Tab5 did not clear stopped motor activity', 'monitor': off['monitor']})
        off['passed'] = True
        self.proof['final'] = self.meta();self.proof['passed'] = True;self.checkpoint()
        return continuous['validation']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--run', action='store_true', help='Explicitly authorize bounded USB-powered hardware checks')
    parser.add_argument('--output', type=Path, default=ROOT / 'outputs/creatures-v26-live.json')
    args = parser.parse_args()
    if not args.run:parser.error('--run is required; no USB device has been opened')
    proof = {'recorded_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'boards': {}, 'complete': False,
             'scope': '35s continuous creature envelope per Blue/Green, short Tab5 find preemption/resume, direct stop',
             'measurement_limit': 'Cell voltage and driver register/fault checks only; no physical input simulation, temperature/current/coil-rail measurement or battery-only certification'}
    def checkpoint():
        args.output.parent.mkdir(parents=True, exist_ok=True)
        temporary = args.output.with_suffix('.json.tmp')
        temporary.write_text(json.dumps(proof, indent=2) + '\n');temporary.replace(args.output)
    with ExitStack() as stack:
        tab, opened = None, {}
        try:
            tab = stack.enter_context(open_monitor())
            identities = []
            for color in ('blue', 'green'):
                board = opened[color] = stack.enter_context(open_device(color))
                item = proof['boards'][color] = {'passed': False}
                identities.append(CreatureBench(tab, board, color, item, checkpoint).run())
                print(color, 'continuous creature/find resume/direct stop PASS', flush=True)
            require(identities[0]['seed'] != identities[1]['seed'] and
                    identities[0]['cycles'][0]['parameters'] != identities[1]['cycles'][0]['parameters'],
                    {'error': 'board creature seeds/parameters are not distinct', 'identities': identities})
            proof['distinct_board_identities'] = True;proof['complete'] = True
        except BaseException as exc:
            proof['abort'] = exception_record(exc)
            raise
        finally:
            cleanup = proof['cleanup'] = {}
            if tab:
                try:command(tab, 'find stop');cleanup['tab_stop_requested'] = True
                except BaseException as exc:cleanup['tab_error'] = exception_record(exc)
            for color, board in opened.items():
                try:
                    h, lines = rpc(board, 'haptic stop');safe(h);quiet(h)
                    x = sample(board);quiet(x['motor'])
                    require(x['creature']['enabled'] == x['creature']['active'] == '0', x)
                    cleanup[color] = {'confirmed_off': True, 'motor': h, 'sample': x, 'lines': lines}
                except BaseException as exc:cleanup[color] = {'error': exception_record(exc)}
            if 'tab_error' in cleanup or any('error' in x for x in cleanup.values() if isinstance(x, dict)):
                proof['complete'] = False
            checkpoint()
    require(proof['complete'], proof)
    print('PASS: two distinct creatures; both motors off; evidence:', args.output, flush=True)


if __name__ == '__main__':
    main()
