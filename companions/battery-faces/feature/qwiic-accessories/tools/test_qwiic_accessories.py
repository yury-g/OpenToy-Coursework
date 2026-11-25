"""Deliberate USB-powered burst checks for Blue/Green; never a discharge test.

Nine isolated bursts per board: 20/40/64 command out of 127 x 100/350/700ms.
No import-time hardware access. Cell sampling cannot certify fast 3.3V/coil-rail
transients, current, physical temperature, or battery-only behavior.
"""
from pathlib import Path
import datetime
import json
import statistics
import time
from device_io import command, fields, open_device, read_line, status
from monitor_io import open_monitor, monitor_status

ROOT = Path(__file__).resolve().parents[1]


class TestAbort(RuntimeError):
    """A safety bound, reset, driver fault, or human interference ended the run."""


def require(condition, detail):
    if not condition:
        raise TestAbort(detail)


def rpc(s, c, kind='HAPTIC'):
    command(s, c)
    lines = []
    end = time.monotonic() + 2
    while time.monotonic() < end:
        line = read_line(s).decode(errors='replace').strip()
        if line:
            lines.append(line)
        if line.startswith(kind + ' '):
            return fields(line), lines
    raise TestAbort({'command': c, 'lines': lines, 'error': 'response timeout'})


def sample(s):
    h, lines = rpc(s, 'haptic sample')
    cells = [fields(x) for x in lines if x.startswith('HAPTIC_SAMPLE ')]
    require(len(cells) == 1, {'error': 'missing fresh cell sample', 'lines': lines})
    result = {'motor': h, 'cell': cells[0]}
    for prefix, key in [('BREATH ', 'breath'), ('CREATURE ', 'creature'), ('QWIIC_BUTTON ', 'button')]:
        values = [fields(x) for x in lines if x.startswith(prefix)]
        if values:
            require(len(values) == 1, {'error': 'ambiguous sample', 'lines': lines})
            result[key] = values[0]
    return result


def safe(h):
    require(h['present'] == '1' and h['ready'] == '1' and h['io'] == '1'
            and h['chip'] == 'ba', {'error': 'haptic absent/not ready/I2C fault', 'motor': h})
    events, warning = int(h['events'], 16), int(h.get('warning_diag', '0'), 16)
    require(not (events & 0xda) and (not (events & 0x20) or (warning != 0 and not (warning & 0x3f))),
            {'error': 'driver fault or unclassified warning', 'motor': h})


def quiet(h):
    require(h['active'] == '0' and h['drive'] == '0' and h['mode'] == '0',
            {'error': 'motor did not stop', 'motor': h})


class SafetyGuard:
    """Carry monotonic uptime/pulse counters through every observed report."""
    def __init__(self, h, button=None):
        self.last_uptime = None
        self.pulses = int(h['pulses'])
        self.errors = int(h['errors'])
        self.button_presses = int(button['presses']) if button else None
        self.button_errors = int(button['errors']) if button else None
        self.motor(h)
        if button:
            self.button(button)

    def uptime(self, value):
        value = int(value)
        require(self.last_uptime is None or value >= self.last_uptime,
                {'error': 'device reset/uptime regression',
                 'previous_ms': self.last_uptime, 'observed_ms': value})
        self.last_uptime = value

    def motor(self, h, accepted=False):
        safe(h)
        self.uptime(h['at_ms'])
        require(int(h['errors']) == self.errors, {'error': 'haptic I2C error count changed', 'motor': h})
        expected = self.pulses + (1 if accepted else 0)
        require(int(h['pulses']) == expected,
                {'error': 'unexpected pulse count; physical-button interference or reset',
                 'expected_pulses': expected, 'motor': h})
        self.pulses = expected

    def button(self, q):
        require(q['present'] == '1' and q['ready'] == '1' and q['io'] == '1'
                and q['id'] == '5d', {'error': 'Qwiic button absent/not ready/I2C fault', 'button': q})
        require(q['pressed'] == '0' and
                (self.button_presses is None or int(q['presses']) == self.button_presses),
                {'error': 'physical-button interference; release button and rerun', 'button': q})
        require(self.button_errors is None or int(q['errors']) == self.button_errors,
                {'error': 'Qwiic button error count changed', 'button': q})

    def reading(self, x, baseline):
        # Sample timestamp precedes the final HAPTIC report on the wire.
        self.uptime(x['cell']['at_ms'])
        self.motor(x['motor'])
        if 'button' in x:
            self.button(x['button'])
        volts = float(x['cell']['volts'])
        require(volts >= 3.45 and baseline - volts <= .15,
                {'error': 'cell voltage safety bound', 'baseline_volts': baseline, 'volts': volts})
        return volts

    def report(self, report, initial):
        self.uptime(fields(report['FACE'])['uptime_ms'])
        self.motor(fields(report['HAPTIC']))
        self.button(fields(report['QWIIC_BUTTON']))
        require(fields(report['BATT'])['valid'] == '1' and fields(report['RADIO'])['ready'] == '1',
                {'error': 'battery gauge or radio not ready', 'report': report})
        require(fields(report['DIAG'])['test'] == fields(initial['DIAG'])['test'],
                {'error': 'network diagnostic state changed', 'report': report})


def exception_record(exc):
    return {'type': type(exc).__name__, 'message': str(exc)}


def cleanup(s, proof, checkpoint, explicit_stop=False):
    """Attempt both stop paths independently; record failures without hiding abort."""
    result = proof.setdefault('cleanup', {})
    commands = [('stop', 'haptic stop')] if explicit_stop else []
    commands.append(('low', 'haptic level low'))  # v21 exposes LOW as its stop command.
    for key, cmd in commands:
        try:
            h, lines = rpc(s, cmd)
            result[key] = {'motor': h, 'lines': lines}
            safe(h)
            quiet(h)
            if key == 'low':
                require(h['level'].lower() == 'low', h)
        except BaseException as exc:
            result[key] = {'error': exception_record(exc), **result.get(key, {})}
        checkpoint()
    try:
        proof['final'] = status(s)  # Only after both immediate-stop attempts.
        safe(fields(proof['final']['HAPTIC']))
        quiet(fields(proof['final']['HAPTIC']))
        require(fields(proof['final']['HAPTIC'])['level'].lower() == 'low', proof['final']['HAPTIC'])
        result['confirmed_inactive_low'] = True
    except BaseException as exc:
        result['final_error'] = exception_record(exc)
        result['confirmed_inactive_low'] = False
    checkpoint()
    return result['confirmed_inactive_low'] and all('error' not in result[k] for k, _ in commands)


def test_board(color, proof=None, checkpoint=lambda: None):
    if proof is None:
        proof = {'color': color, 'trials': []}
    try:
        with open_device(color) as s:
            try:
                initial = status(s)
                proof['initial'] = initial
                checkpoint()
                require(any(v in initial['FACE'] for v in ('v21-qwiic-haptic-buttons','v22-qwiic-find-breathing','v23-creature-breathing')), initial['FACE'])
                h, q = fields(initial['HAPTIC']), fields(initial['QWIIC_BUTTON'])
                guard = SafetyGuard(h, q)
                quiet(h)
                for state, expected in [('off', '0'), ('on', '16')]:
                    led, _ = rpc(s, 'button led ' + state, 'QWIIC_BUTTON')
                    proof.setdefault('led_write_readback', {})[state] = led
                    guard.button(led)
                    require(led['led_readback'] == expected, led)
                    checkpoint()
                for level, strength in [('low', 20), ('med', 40), ('high', 64)]:
                    for duration in [100, 350, 700]:
                        trial = {'level': level, 'command': strength, 'requested_ms': duration,
                                 'baselines': [], 'samples': [], 'passed': False}
                        proof['trials'].append(trial)
                        checkpoint()
                        time.sleep(1.35)
                        for _ in range(3):
                            x = sample(s)
                            trial['baselines'].append(x)
                            guard.reading(x, float(x['cell']['volts']))
                            quiet(x['motor'])
                        baseline = statistics.median(float(x['cell']['volts']) for x in trial['baselines'])
                        trial['baseline_cell_volts'] = baseline
                        pulse, lines = rpc(s, f'haptic test {level} {duration}')
                        trial['accepted'], trial['accept_lines'] = pulse, lines
                        require('HAPTIC_TEST accepted=1' in lines, lines)
                        guard.motor(pulse, accepted=True)
                        require(pulse['active'] == '1' and int(pulse['drive']) == strength
                                and int(pulse['duration_ms']) == duration, pulse)
                        began = int(pulse['start_ms'])
                        end = time.monotonic() + 2
                        while time.monotonic() < end:
                            x = sample(s)
                            trial['samples'].append(x)
                            guard.reading(x, baseline)
                            m = x['motor']
                            elapsed = (int(m['at_ms']) - began) & 0xffffffff
                            if m['active'] == '0':
                                quiet(m)
                                require(duration <= elapsed <= duration + 200,
                                        {'duration': duration, 'observed_off_ms': elapsed})
                                break
                            require(elapsed <= duration + 200, m)
                            time.sleep(.025)
                        else:
                            raise TestAbort('Motor did not confirm stop in time')
                        trial['observed_first_off_ms'] = elapsed
                        trial['minimum_sampled_cell_volts'] = min(float(x['cell']['volts']) for x in trial['samples'])
                        trial['after'] = status(s)
                        guard.report(trial['after'], initial)
                        trial['passed'] = True
                        checkpoint()
                        print(color, level, duration, 'ms PASS; stop', elapsed, 'ms; min cell',
                              trial['minimum_sampled_cell_volts'], flush=True)
            except BaseException as exc:
                proof['abort'] = exception_record(exc)
                checkpoint()
                raise
            finally:
                cleanup_ok = cleanup(s, proof, checkpoint)
            require(cleanup_ok, {'error': 'cleanup not fully confirmed', 'cleanup': proof['cleanup']})
    except BaseException as exc:
        proof.setdefault('abort', exception_record(exc))
        checkpoint()
        raise
    proof['passed'] = True
    checkpoint()
    return proof


def main():
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'outputs/qwiic-burst-tests-v23.json')
    args = parser.parse_args()
    evidence = {'recorded_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                'scope': 'Deliberate USB-powered bounded bursts; no battery load/discharge test or unattended repeats',
                'limits': {'command_max': 64, 'duration_max_ms': 700, 'rest_seconds': 1.35,
                           'abort_cell_below_volts': 3.45, 'abort_cell_drop_volts': .15},
                'measurement_limit': 'MAX17048 cell voltage and DA7280 fault/register checks; fast 3.3V/coil rail/current/physical temperature and battery-only behavior unmeasured',
                'boards': {}, 'complete': False}
    target = args.output
    def checkpoint():
        target.parent.mkdir(parents=True, exist_ok=True)
        temporary = target.with_suffix('.json.tmp')
        temporary.write_text(json.dumps(evidence, indent=2) + '\n')
        temporary.replace(target)
    try:
        for color in ('blue', 'green'):
            # Register before opening the device: exceptions retain partial proof.
            proof = {'color': color, 'trials': [], 'passed': False}
            evidence['boards'][color] = proof
            checkpoint()
            test_board(color, proof, checkpoint)
        with open_monitor() as s:
            evidence['tab5'] = monitor_status(s)
            for color in ('blue', 'green'):
                require(fields(evidence['tab5']['NODE'][color])['fresh'] == '1', evidence['tab5'])
        evidence['complete'] = True
    except BaseException as exc:
        evidence['abort'] = exception_record(exc)
        raise
    finally:
        checkpoint()
    print('PASS 18 isolated bursts; motors inactive/LOW; Tab5 receives both; no capacity or circuit certification', flush=True)


if __name__ == '__main__':
    main()
