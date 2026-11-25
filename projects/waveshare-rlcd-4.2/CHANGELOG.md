# RLCD Eyes changes

## 0.2.1 — 2026-11-17 (source candidate; parked hardware remains 0.2.0)

Replace the hollow empty-battery rectangle icon in `drawHeader()` with explicit neutral `BAT --` when unavailable/stale and `BAT X.XXV` when a fresh calibrated voltage sample is valid. Never invent SOC percentage or firmware charge state.

## 0.2.0 — 2026-11-17

Native-resolution analytic eye contours replace four-times bitmap enlargement on this target. Black background, light eye shapes/dark enlarged pupils, and light status ink match the owner’s requested polarity. Existing pose/personality/reaction behavior and controller initialization remain unchanged; the historical pixel renderer stays preserved. No grayscale or dithering claim. Add calibrated battery sample/failure/freshness/timing diagnostics while retaining genuine volts or unknown in the header. Source comparisons and animation are recorded in docs/NATIVE-RENDERING.md. Replace normal-play BOOT mappings with the GPIO18 KEY short/hold menu profile: short reaction, 0.8-second hold Menu, short Next/hold Select, explicit Back, per-eye actions and reversible performance mode. Header/telemetry share voltage formatting and treat readings older than five seconds as unavailable.

## 0.1.1 — 2026-11-17

Move application entry points from the sketch into an ordinary C++ translation unit. On this Mac, Arduino's prototype preprocessing inserted calls to setup/loop inside setup, causing recursive startup and a live boot panic in0.1.0. The generated sketch and decoded backtrace establish the cause. Shared animation, button behavior and display drivers are unchanged. Rebuild and real boot verification required; host geometry tests alone did not detect this toolchain failure.

## 0.1.0 — 2026-11-17 — failed live boot, superseded

First RLCD adapter, shared personalities/large pupils/reactions, native mono packing, button mapping, voltage header and session performance toggle. Host checks and build passed; original backups and bounded write verified. Failed on live boot due to generated startup recursion. Do not install this checkpoint.
