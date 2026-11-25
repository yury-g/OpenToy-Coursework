# CrowPanel Eyes 1 verification — 2026-11-17

Target: DHE04209D **V1.0**, ESP32-P4 revision 1.3; firmware **0.2.2**,
version created **2026-11-17**. Raw records, device identities and firmware images
stay private. The source-rendered preview is a scripted simulation, not a camera
recording or a performance measurement.

## Software and review

- CrowPanel host checks and all reused Animatronic Eyes host suites pass.
  Coverage includes viewport/footer separation, multiple contacts, fresh packet
  expiry, failed input release, timer wrap, deferred layout, independent controls,
  all personalities, partial/bottom lids and smooth geometry at both native sizes.
- ESP-IDF 5.4.2 target build passes: **610,112-byte** application with P4-compatible
  revision header, valid checksum/hash, and ample space in the 11 MiB factory slot.
- Exact dependencies and generated configurations are checked. The live partition
  entries match; no partition table or bootloader installation is needed.
- Fresh review prompted fixes for stale cached touch packets, deferred layout,
  deadline wrap and zero-tick delay. Native renderer review found no blockers.
  Final battery review required diagnostic-only candidate values, a total polling
  deadline and refreshed input time; follow-up review confirmed both fixes.
- Final source digest is
  `ff493b2789e4b855eec18351298114a67a2c58bbd080b8e0dcd04ef4ad267706`.
  The preview renderer digest matches source. Its 120 scripted 100 ms steps encode
  as 98 GIF frames after identical frames merge, preserving the 12-second duration.

## Installation

Expected P4 ROM identity, security, flash size, prior whole-flash digest and live
partition table were checked before the write. Only sector-aligned application
bytes **0x10000–0xa4fff** changed. Independent full application readback matches;
hardware digests verify every byte before and after that range unchanged.
Original bootloader, table, NVS and storage remain intact. XIAO and other devices
were not modified. The two verified factory recovery copies remain private.

## Final live checks

Installed version/source/factory slot match. Display and touch report ready.
The finite final run advances from frame 83 to 1115 with zero touch read errors;
sampled free heap stays 32,129,696 bytes. Header/ECO toggles, all personalities,
synthetic left/right/both pokes and recovery, sampling pause/resume, and 1,500
queued diagnostic requests pass. Pokes in this bench are explicitly synthetic.

| Personality | Header shown, scale 7 | Header hidden, scale 8 |
| --- | --- | --- |
| Curious | 14.89 fps | 12.62 fps |
| Dreamy | 15.11 fps | 12.82 fps |
| Cheeky | 14.83 fps | 12.96 fps |

These short intervals use normal mode; the separate default ECO sample is 14.29 fps.
Normal mode remains rendering-limited; 30 fps is not established. Endpoint pixel
work is 41.8–52.1 ms with the header and 51.9–63.0 ms without; submission is 9.6–9.8 ms
and 11.7 ms, respectively. Blink phase affects these endpoint samples. The larger
hidden-header eyes cost more to draw; hiding the header is not a promised speedup.

After the live run, an independently requested whole-flash hardware digest
matches the verified installation. Original non-application data remains intact.
A hardware reset restores Curious, ECO ON and visible information; installed
version, display/touch readiness and no crash markers were verified afterward.
All serial handles were then closed. This reset is not a battery power-cycle test.

The early 0.1.0–0.1.2 bench builds exposed slow display memory. Timing separated
sub-millisecond behavior calculation from pixel movement. ESP-IDF 5.4.2 had
silently fallen back to 20 MHz PSRAM without its experimental-feature gate.
Version 0.1.3 restored Elecrow's documented 200 MHz setting and added generated
configuration checks. Version 0.2.0 replaced enlarged source pixels with native
analytic curves and one-pixel antialiasing in two bounded eye images.

## Battery and owner observation

Final 0.2.2 displays neutral **BAT --**. STC8 responds with coherent candidate
values, but exact-board protocol/calibration and charging remain unqualified.
No displayed percentage or charging icon is claimed. See [battery evidence](BATTERY.md).

When asked whether the moving eyes and individual touch reactions looked correct,
the owner replied **“everything looks great right now”** while smooth 0.2.0 was
running. Telemetry also recorded physical pokes. This is positive owner feedback,
not a detailed simultaneous multi-touch test or an observation of the later battery
label. Backup restoration, a full power cycle, battery runtime and camera capture
have not been tested. Audio and the common family menu remain future work.
