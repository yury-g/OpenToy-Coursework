"""Exercise the real FeatherHaptic wrapper without Arduino, USB, or hardware."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / 'firmware/FeatherFace/FeatherHaptic.h'
ZIG = Path('/Users/example/Documents/Codex/2026-10-01/handle-this-project-end-to-end/work/zig-aarch64-macos-0.16.0/zig')

WIRE = r'''
#pragma once
#include <cstdint>
struct FakeWire {
    bool present = false;
    uint8_t selected = 0;
    uint8_t registers[256] = {};
    unsigned transactions = 0;
    void beginTransmission(uint8_t) { ++transactions; }
    void write(uint8_t reg) { selected = reg; }
    unsigned endTransmission(bool) { return present ? 0 : 1; }
    unsigned requestFrom(uint8_t, uint8_t) { return present ? 1 : 0; }
    uint8_t read() { return registers[selected]; }
};
inline FakeWire Wire;
'''

FIXTURE = r'''
#include <cassert>
#include <cstring>
#include <Wire.h>
static uint32_t clockMs = 0;
uint32_t millis() { return clockMs; }
struct FakeSerial {
    template <typename... T> void printf(const char *, T...) {}
} Serial;

// Replace only the external driver's I2C boundary, preserving the real wrapper.
#define _SPARKFUN_HAPTIC_DRIVER_DA7280_
enum { INACTIVE = 0, DRO_MODE = 1 };
static unsigned driverCalls = 0;
struct Haptic_Driver {
    FakeWire *bus = nullptr;
    bool failZero = false;
    bool ignoreNonzeroDrive = false, ignoreActiveMode = false;
    uint8_t eventAfterActive=0, warningAfterActive=0;
    bool begin(FakeWire &wire) { ++driverCalls; bus = &wire; return true; }
    void checkBound() { assert(bus != nullptr); ++driverCalls; }
    bool setVibrate(uint8_t value) {
        checkBound();
        if (!value && failZero) return false;
        if (value && ignoreNonzeroDrive) return true;
        bus->registers[0x23] = value;
        return true;
    }
    bool setOperationMode(uint8_t mode) {
        checkBound();
        if (mode && ignoreActiveMode) return true;
        bus->registers[0x22] = mode;
        if(mode && eventAfterActive){bus->registers[3]=eventAfterActive;bus->registers[4]=warningAfterActive;}
        return true;
    }
    bool defaultMotor() { checkBound(); return true; }
    bool enableFreqTrack(bool) { checkBound(); return true; }
    bool clearIrq(uint8_t) { checkBound(); return true; }
};
#include "@HEADER@"

int main(int argc, char **argv) {
    assert(argc == 2);
    FeatherHaptic h;
    if (!std::strcmp(argv[1], "absent")) {
        assert(!h.begin());
        unsigned readsAfterProbe = Wire.transactions;
        h.stop();
        h.service(1000);
        assert(driverCalls == 0);
        assert(Wire.transactions == readsAfterProbe);
        assert(!h.busy() && !h.ready && !h.stopPending && h.commanded == 0);
        return 0;
    }
    Wire.present = true;
    Wire.registers[0] = 0xba;
    assert(h.begin());
    assert(h.strength() == 127); // Strong default after boot.
    if (!std::strcmp(argv[1], "trigger-held") || !std::strcmp(argv[1], "trigger-creature")) {
        clockMs = 1000;
        bool heldCase = !std::strcmp(argv[1], "trigger-held");
        assert(heldCase ? h.startHeld(clockMs) : h.startCreature(clockMs, 12345));
        unsigned startedPulses = h.pulses;
        uint8_t startedDrive = Wire.registers[0x23];
        unsigned callsBefore = driverCalls;
        clockMs = 1050;
        assert(!h.trigger(clockMs));
        assert(h.busy() && !h.pulse.active && h.pulses == startedPulses);
        assert(h.held == heldCase && h.creature.active == !heldCase);
        assert(Wire.registers[0x23] == startedDrive && driverCalls == callsBefore);
        return 0;
    }
    if (!std::strcmp(argv[1], "startup-held-fault") || !std::strcmp(argv[1], "startup-pulse-fault")) {
        h.driver.eventAfterActive=0x20;h.driver.warningAfterActive=0x08;
        bool accepted=!std::strcmp(argv[1],"startup-held-fault")?h.startHeld(1000):h.trigger(1000);
        assert(!accepted && !h.ready && !h.busy());
        assert(Wire.registers[0x23]==0 && Wire.registers[0x22]==INACTIVE);
        return 0;
    }
    if (!std::strcmp(argv[1], "warning-policy")) {
        const uint8_t limits[]={0x40,0x80,0xc0};
        for (uint8_t diagnostic : limits) {
            Wire.registers[3]=Wire.registers[4]=0;
            FeatherHaptic limited;assert(limited.begin());clockMs=1000;
            Wire.registers[3]=0x20;Wire.registers[4]=diagnostic;
            assert(limited.startHeld(clockMs));
            clockMs=1200;limited.confirmHeld(clockMs,true,true);limited.service(clockMs);
            assert(limited.ready && limited.held && limited.warning==diagnostic && limited.commanded==127);
            Wire.registers[4]=diagnostic|0x08; // Later temperature warning must still stop.
            clockMs=1400;limited.confirmHeld(clockMs,true,true);limited.service(clockMs);
            assert(!limited.ready && !limited.busy() && Wire.registers[0x23]==0);
        }
        const uint8_t blockedWarnings[]={0,0x08,0x10,0x01,0x50,0xc8};
        for (uint8_t diagnostic : blockedWarnings) {
            Wire.registers[3]=Wire.registers[4]=0;
            FeatherHaptic blocked;assert(blocked.begin());
            Wire.registers[3]=0x20;Wire.registers[4]=diagnostic;
            assert(!blocked.startHeld(1000) && !blocked.ready && !blocked.busy());
        }
        return 0;
    }
    if (!std::strcmp(argv[1], "held-levels")) {
        for (unsigned level = 0; level < 3; ++level) {
            FeatherHaptic continuous;
            assert(continuous.begin());
            continuous.level = level;
            clockMs = 1000;
            assert(continuous.startHeld(clockMs));
            for (clockMs = 1050; clockMs <= 2200; clockMs += 50) {
                continuous.confirmHeld(clockMs, true, true);
                continuous.service(clockMs);
                assert(continuous.held && continuous.busy() && continuous.ready);
                assert(continuous.heldConfirmedAt == clockMs);
                assert(continuous.commanded == 127);
                assert(Wire.registers[0x23] == 127);
                assert(Wire.registers[0x22] == DRO_MODE && continuous.pulses == 1);
            }
            continuous.stop();
            assert(!continuous.held && !continuous.busy());
        }
        return 0;
    }
    if (!std::strncmp(argv[1], "held-", 5)) {
        clockMs = 1000;
        if (!std::strcmp(argv[1], "held-preempt-pulse") || !std::strcmp(argv[1], "held-preempt-failed-stop")) {
            assert(h.trigger(clockMs));
            clockMs = 1100;
            if (!std::strcmp(argv[1], "held-preempt-failed-stop")) {
                h.driver.failZero = true;
                assert(!h.startHeld(clockMs));
                assert(!h.held && h.stopPending && h.busy() && !h.ready);
                h.driver.failZero = false;
                clockMs = 1200;
                h.service(clockMs);
                assert(!h.held && !h.busy() && Wire.registers[0x23] == 0);
                return 0;
            }
            h.level = 2;
            assert(h.startHeld(clockMs));
            assert(h.held && !h.pulse.active && !h.breathing.active && h.commanded == 127);
            assert(h.pulses == 2 && Wire.registers[0x23] == 127);
            return 0;
        }
        if (!std::strcmp(argv[1], "held-preempt-breath")) {
            assert(h.startBreathing(clockMs, 20, 10));
            clockMs = 2000;
            h.service(clockMs);
            assert(h.breathing.active && Wire.registers[0x23] > 0);
            clockMs = 2050;
            assert(h.startHeld(clockMs));
            assert(h.held && !h.pulse.active && !h.breathing.active && h.commanded == 127);
            assert(h.pulses == 2 && Wire.registers[0x23] == 127);
            return 0;
        }
        assert(h.startHeld(clockMs));
        assert(h.held && h.busy() && h.heldConfirmedAt == 1000);
        if (!std::strcmp(argv[1], "held-release") || !std::strcmp(argv[1], "held-invalid")) {
            clockMs = 1050;
            bool valid = !std::strcmp(argv[1], "held-release");
            h.confirmHeld(clockMs, !valid, valid);
            assert(!h.held && !h.busy()); // Release/error itself stops, without service().
            unsigned callsAfterRelease = driverCalls;
            h.confirmHeld(clockMs + 1, true, true);
            assert(!h.held && !h.busy() && driverCalls == callsAfterRelease);
        } else if (!std::strcmp(argv[1], "held-lease")) {
            clockMs = 1249;
            h.service(clockMs);
            assert(h.held && h.busy() && Wire.registers[0x23] == 127);
            clockMs = 1250;
            h.service(clockMs);
            assert(!h.held && !h.busy());
        } else {
            clockMs = 1200;
            h.confirmHeld(clockMs, true, true);
            if (!std::strcmp(argv[1], "held-drive-readback")) Wire.registers[0x23] = 19;
            else if (!std::strcmp(argv[1], "held-mode-readback")) Wire.registers[0x22] = INACTIVE;
            else { assert(!std::strcmp(argv[1], "held-fault")); Wire.registers[3] = 0x80; }
            h.service(clockMs);
            assert(!h.held && !h.busy() && !h.ready && h.errors > 0);
        }
        assert(h.commanded == 0 && Wire.registers[0x23] == 0 && Wire.registers[0x22] == INACTIVE);
        return 0;
    }
    if (!std::strcmp(argv[1], "drive-readback") || !std::strcmp(argv[1], "mode-readback")) {
        clockMs = 1000;
        assert(h.startBreathing(clockMs, 20, 10));
        assert(h.busy() && h.breathing.active && h.pulses == 1);
        h.driver.ignoreNonzeroDrive = !std::strcmp(argv[1], "drive-readback");
        h.driver.ignoreActiveMode = !std::strcmp(argv[1], "mode-readback");
        clockMs = 2000;
        h.service(clockMs); // A successful write ACK with wrong physical register value.
        assert(!h.ready && !h.busy() && !h.breathing.active);
        assert(h.errors == 1 && h.commanded == 0);
        assert(Wire.registers[0x23] == 0 && Wire.registers[0x22] == INACTIVE);
        return 0;
    }
    assert(!std::strcmp(argv[1], "retry"));
    clockMs = 100;
    assert(h.trigger(clockMs));
    assert(h.busy() && Wire.registers[0x23] == 127);
    h.driver.failZero = true;
    clockMs = 200;
    h.stop();
    assert(h.stopPending && h.busy() && !h.ready);
    assert(h.commanded == 127 && h.errors == 1);
    unsigned callsAfterFailure = driverCalls;
    h.driver.failZero = false;
    clockMs = 299;
    h.service(clockMs);
    assert(driverCalls == callsAfterFailure && h.busy());
    clockMs = 300;
    h.service(clockMs);
    assert(driverCalls > callsAfterFailure);
    assert(!h.stopPending && !h.busy() && h.commanded == 0);
    assert(Wire.registers[0x23] == 0 && Wire.registers[0x22] == INACTIVE);
    assert(h.hasStopped && h.lastStopped == 300 && h.errors == 1);
    assert(!h.ready); // A failed hardware stop stays faulted until explicit reprobe.
}
'''


def compile_fixture(directory, header=HEADER):
    directory = Path(directory)
    (directory / 'Wire.h').write_text(WIRE)
    source = directory / 'wrapper_test.cpp'
    source.write_text(FIXTURE.replace('@HEADER@', str(header)))
    executable = directory / 'wrapper_test'
    result = subprocess.run(
        [str(ZIG), 'c++', '-std=c++17', '-I' + str(directory),
         '-I' + str(HEADER.parent), str(source), '-o', str(executable)],
        capture_output=True, text=True, timeout=120,
    )
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return executable


class FeatherHapticWrapper(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix='feather-haptic-host-')
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.executable = compile_fixture(cls.temporary.name)

    def run_fixture(self, scenario):
        result = subprocess.run([str(self.executable), scenario], capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_absent_probe_then_stop_never_calls_unbound_driver(self):
        self.run_fixture('absent')

    def test_failed_zero_write_remains_busy_until_bounded_stop_retry(self):
        self.run_fixture('retry')

    def test_acknowledged_drive_write_with_wrong_readback_stops_breathing(self):
        self.run_fixture('drive-readback')

    def test_acknowledged_mode_write_with_wrong_readback_stops_breathing(self):
        self.run_fixture('mode-readback')

    def test_normal_pulse_is_rejected_while_button_is_held(self):
        self.run_fixture('trigger-held')

    def test_normal_pulse_is_rejected_while_creature_is_active(self):
        self.run_fixture('trigger-creature')

    def test_new_temperature_warning_stops_immediately_after_held_start(self):
        self.run_fixture('startup-held-fault')

    def test_new_temperature_warning_stops_immediately_after_pulse_start(self):
        self.run_fixture('startup-pulse-fault')

    def test_supply_limits_allow_full_command_but_temperature_and_unknown_warnings_stop(self):
        self.run_fixture('warning-policy')

    def test_held_full_command_ignores_diagnostic_level_with_confirmations(self):
        self.run_fixture('held-levels')

    def test_held_release_stops_immediately_without_service(self):
        self.run_fixture('held-release')

    def test_held_missing_confirmation_stops_at_250_milliseconds(self):
        self.run_fixture('held-lease')

    def test_held_invalid_button_read_stops_immediately(self):
        self.run_fixture('held-invalid')

    def test_held_wrong_drive_readback_stops_and_faults(self):
        self.run_fixture('held-drive-readback')

    def test_held_wrong_mode_readback_stops_and_faults(self):
        self.run_fixture('held-mode-readback')

    def test_held_driver_fault_stops_and_disables_ready(self):
        self.run_fixture('held-fault')

    def test_held_preempts_pulse_after_confirmed_stop(self):
        self.run_fixture('held-preempt-pulse')

    def test_held_preempts_breathing_after_confirmed_stop(self):
        self.run_fixture('held-preempt-breath')

    def test_held_refuses_preemption_when_stop_is_unconfirmed(self):
        self.run_fixture('held-preempt-failed-stop')



if __name__ == '__main__':
    unittest.main()
