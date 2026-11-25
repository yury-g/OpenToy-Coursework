"""Deliberate, bounded USB breathing check for one Blue/Green board at a time.

Run manually with the project's Python environment. No daemon, discharge cycle,
import-time hardware access, or full-power command. The fixed sequence is peak
20 for 30 seconds, then peak 32 for 20 seconds, with quiet rest between patterns.
"""
import datetime
import json
import math
import time
from test_qwiic_accessories import (
    ROOT, SafetyGuard, TestAbort, cleanup, exception_record, fields, open_device,
    quiet, require, rpc, sample, status,
)

SAMPLE_SECONDS = .1
REST_SECONDS = 3.0
PATTERNS = ((20, 30), (32, 20))


def breath_fields(lines):
    values = [fields(line) for line in lines if line.startswith('BREATH ')]
    require(len(values) == 1, {'error': 'missing breathing report', 'lines': lines})
    return values[0]


class BreathValidator:
    """Check bounded amplitude, smooth direction, off rests, and natural expiry."""
    def __init__(self, peak, seconds):
        require(4 <= peak <= 32 and 10 <= seconds <= 60 and seconds % 10 == 0,
                'Refusing unbounded breathing test')
        self.peak = peak
        self.duration = seconds * 1000
        self.last_elapsed = -1
        self.previous = None
        self.cycles = {}
        self.finished = False

    def observe(self, motor, breath):
        elapsed = int(breath['elapsed_ms'])
        remaining = int(breath['remaining_ms'])
        drive = int(breath['drive'])
        require(self.last_elapsed <= elapsed <= self.duration,
                {'error': 'breathing elapsed time regressed/exceeded cap', 'breath': breath})
        require(int(breath['peak']) == self.peak and 0 <= drive <= self.peak,
                {'error': 'breathing amplitude bound', 'breath': breath})
        require(int(motor['drive']) == drive,
                {'error': 'command/driver drive mismatch', 'motor': motor, 'breath': breath})
        self.last_elapsed = elapsed
        if breath['active'] == '0':
            require(elapsed == self.duration and remaining == 0,
                    {'error': 'breathing ended before its time cap', 'breath': breath})
            quiet(motor)
            require(drive == 0 and breath['phase'] in ('off', 'rest'), breath)
            self.finished = True
            return
        require(not self.finished and motor['active'] == '1' and remaining > 0
                and elapsed + remaining == self.duration,
                {'error': 'breathing state/time cap mismatch', 'breath': breath, 'motor': motor})
        position = elapsed % 10000
        expected_phase = 'inhale' if position < 4000 else 'exhale' if position < 8000 else 'rest'
        require(breath['phase'] == expected_phase,
                {'error': 'unexpected breathing phase', 'breath': breath})
        require(int(motor['mode']) == (1 if drive else 0),
                {'error': 'driver not inactive at zero, or not driving at nonzero', 'motor': motor})
        require(expected_phase != 'rest' or drive == 0,
                {'error': 'motor continued during breathing rest', 'breath': breath})
        cycle = elapsed // 10000
        phases = self.cycles.setdefault(cycle, {})
        phases.setdefault(expected_phase, []).append(drive)
        if self.previous:
            old_elapsed, old_cycle, old_phase, old_drive = self.previous
            delta = elapsed - old_elapsed
            require(delta <= 500, {'error': 'sampling gap too large to verify envelope', 'gap_ms': delta})
            if cycle == old_cycle and expected_phase == old_phase:
                if expected_phase == 'inhale':
                    require(drive >= old_drive, {'error': 'inhale amplitude fell', 'breath': breath})
                if expected_phase == 'exhale':
                    require(drive <= old_drive, {'error': 'exhale amplitude rose', 'breath': breath})
                # Smooth 4-second ramps have bounded slope, allowing integer rounding.
                require(abs(drive - old_drive) <= math.ceil(self.peak * 1.5 * delta / 4000) + 1,
                        {'error': 'abrupt amplitude step', 'breath': breath})
        self.previous = elapsed, cycle, expected_phase, drive

    def complete(self):
        require(self.finished, 'Pattern did not naturally expire')
        require(len(self.cycles) == self.duration // 10000,
                {'error': 'not every breath was sampled', 'cycles': list(self.cycles)})
        for cycle, phases in self.cycles.items():
            require(all(phase in phases for phase in ('inhale', 'exhale', 'rest')),
                    {'error': 'breath phase missing', 'cycle': cycle, 'phases': list(phases)})
            require(max(phases['inhale'] + phases['exhale']) >= self.peak - 1,
                    {'error': 'breath never approached requested peak', 'cycle': cycle})
            for phase in ('inhale', 'exhale'):
                values = phases[phase]
                require(max(values) - min(values) >= self.peak * .75,
                        {'error': 'ramp range too small', 'cycle': cycle, 'phase': phase})


def validate_reading(x, guard, baseline, validator=None):
    require('breath' in x and 'button' in x,
            {'error': 'v22 fresh sample lacks BREATH/button proof', 'sample': x})
    guard.reading(x, baseline)
    # Gauge timestamp must belong to this response, before the motor report.
    lag = int(x['motor']['at_ms']) - int(x['cell']['at_ms'])
    require(0 <= lag <= 250, {'error': 'stale/invalid cell sample timestamp', 'sample': x})
    if validator:
        validator.observe(x['motor'], x['breath'])
    else:
        quiet(x['motor'])
        require(x['breath']['active'] == '0' and x['breath']['drive'] == '0', x)


def test_board(color, proof, checkpoint):
    try:
        with open_device(color) as s:
            try:
                # Generic status is intentionally used only before/after a pattern:
                # it stops actuation before its potentially blocking USB report.
                initial = status(s)
                proof['initial'] = initial
                checkpoint()
                require('v22-qwiic-find-breathing' in initial['FACE'], initial['FACE'])
                guard = SafetyGuard(fields(initial['HAPTIC']), fields(initial['QWIIC_BUTTON']))
                quiet(fields(initial['HAPTIC']))
                require(fields(initial['BATT'])['valid'] == '1' and fields(initial['RADIO'])['ready'] == '1',
                        {'error': 'battery gauge or radio not ready before test', 'report': initial})
                for peak, seconds in PATTERNS:
                    trial = {'peak_command': peak, 'duration_seconds': seconds,
                             'baselines': [], 'samples': [], 'post_expiry': [], 'passed': False}
                    proof['trials'].append(trial)
                    checkpoint()
                    time.sleep(REST_SECONDS)
                    for _ in range(5):
                        x = sample(s)
                        trial['baselines'].append(x)
                        validate_reading(x, guard, float(x['cell']['volts']))
                        time.sleep(SAMPLE_SECONDS)
                    # Use the highest fresh quiet voltage for the conservative droop bound.
                    baseline = max(float(x['cell']['volts']) for x in trial['baselines'])
                    trial['baseline_cell_volts'] = baseline
                    accepted, lines = rpc(s, f'haptic breathe {peak} {seconds}')
                    trial['accepted'], trial['accept_lines'] = accepted, lines
                    require(any(line.startswith('HAPTIC_BREATHE accepted=1') for line in lines), lines)
                    guard.motor(accepted, accepted=True)
                    require(accepted['active'] == '1', accepted)
                    validator = BreathValidator(peak, seconds)
                    validator.observe(accepted, breath_fields(lines))
                    end = time.monotonic() + seconds + .5
                    while time.monotonic() < end:
                        x = sample(s)  # Fresh gauge read; does not stop the envelope.
                        trial['samples'].append(x)
                        validate_reading(x, guard, baseline, validator)
                        elapsed_motor = int(x['motor']['at_ms']) - int(accepted['start_ms'])
                        require(elapsed_motor <= seconds * 1000 + 500,
                                {'error': 'motor continued beyond pattern time cap', 'sample': x})
                        if validator.finished:
                            trial['observed_off_ms'] = elapsed_motor
                            break
                        time.sleep(SAMPLE_SECONDS)
                    else:
                        raise TestAbort('Breathing did not confirm natural stop by its time cap')
                    validator.complete()
                    # Keep observing after natural expiry before status can stop anything.
                    for _ in range(5):
                        time.sleep(SAMPLE_SECONDS)
                        x = sample(s)
                        trial['post_expiry'].append(x)
                        validate_reading(x, guard, baseline)
                    trial['after'] = status(s)
                    guard.report(trial['after'], initial)
                    trial['minimum_sampled_cell_volts'] = min(float(x['cell']['volts']) for x in trial['samples'])
                    trial['cycles_observed'] = len(validator.cycles)
                    trial['passed'] = True
                    checkpoint()
                    print(color, 'breathing peak', peak, 'for', seconds, 'seconds PASS;',
                          trial['cycles_observed'], 'breaths, natural stop', trial['observed_off_ms'], 'ms;',
                          'minimum sampled cell', trial['minimum_sampled_cell_volts'], flush=True)
            except BaseException as exc:
                proof['abort'] = exception_record(exc)
                checkpoint()
                raise
            finally:
                cleanup_ok = cleanup(s, proof, checkpoint, explicit_stop=True)
            require(cleanup_ok, {'error': 'cleanup not fully confirmed', 'cleanup': proof['cleanup']})
    except BaseException as exc:
        proof.setdefault('abort', exception_record(exc))
        checkpoint()
        raise
    proof['passed'] = True
    checkpoint()


def main():
    evidence = {'recorded_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                'scope': 'Deliberate USB-powered breathing; Blue then Green, one board at a time; no discharge cycle',
                'limits': {'patterns': [{'peak_command': p, 'seconds': s} for p, s in PATTERNS],
                           'firmware_peak_max': 32, 'firmware_duration_max_seconds': 60,
                           'rest_seconds': REST_SECONDS, 'sample_interval_seconds': SAMPLE_SECONDS,
                           'abort_cell_below_volts': 3.45, 'abort_cell_drop_volts': .15},
                'measurement_limit': 'Fresh MAX17048 cell voltage and DA7280 fault/register checks; fast 3.3V/coil rail/current/physical temperature and battery-only behavior unmeasured',
                'boards': {}, 'complete': False}
    target = ROOT / 'outputs/haptic-breathing-v22.json'
    def checkpoint():
        target.parent.mkdir(parents=True, exist_ok=True)
        temporary = target.with_suffix('.json.tmp')
        temporary.write_text(json.dumps(evidence, indent=2) + '\n')
        temporary.replace(target)
    try:
        for color in ('blue', 'green'):
            proof = {'color': color, 'trials': [], 'passed': False}
            evidence['boards'][color] = proof
            checkpoint()
            test_board(color, proof, checkpoint)
        evidence['complete'] = True
    except BaseException as exc:
        evidence['abort'] = exception_record(exc)
        raise
    finally:
        checkpoint()
    print('PASS breathing on Blue/Green; natural expiry and inactive LOW confirmed; no circuit certification', flush=True)


if __name__ == '__main__':
    main()
