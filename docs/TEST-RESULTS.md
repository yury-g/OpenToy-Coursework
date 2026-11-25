# Bench evidence — October 1, 2026

Tests ran on the physical Mac (native macOS27.0.1), its USB serial ports and CoreBluetooth. Agent checks observe serial, state and display writes. The owner's later “Works great” confirmation is recorded separately below; no agent optical observation of the OLED is claimed.

| Check | Result |
| --- | --- |
| Two board identities | Separate ROM-confirmed ESP32-S3 revision0.2, embedded8MB PSRAM,8MB flash |
| Preservation | Original8MB flash images read before replacement; source preserved; separate private keys |
| USB seed | Both compiled/uploaded; stable serial heartbeats/status; no routine erase-flash |
| Live BLE | Bare period/duty/mode, bounds/fragments/checksummed button packets; OLED contrast/screens/touch state corroborated by USB |
| Full BLE update | Multiple complete images on both boards, SHA256 and running-build verification |
| Failed-boot rollback | Deliberately unconfirmed images rolled back on both physical boards |
| Mac app | Live control, file-picker upload and Build + update exercised on both profiles |
| OLED electronic evidence | Checked real I2C frame writes, zero errors in passing suites; distinct frame hashes |
| Touch electronic evidence | Four real changing raw readings; threshold/gating changed touch mask and frame; no physical touch claim |
| Face control | Synthetic controller mappings sent over real BLE; USB matched gaze, independent lids and expressions; default/changed frame hashes checked |
| Controller recognition | Physical USB045e:0b12; Apple XboxSeriesXGamepad driver; one native extended gamepad; actual full stick ranges and left/right D-pad captured |
| Actual controller → toy baseline | Native Mac app sent actual neutral pose over BLE; independent USB matched seq58 and neutral frame; txerr0/I2C errors0 |
| iPad | USB identity seen earlier; app/client tests explicitly deferred |

Normal measured application-transfer/reconnect time ranged roughly65–230seconds across images/modes. Faster flow-controlled groups completed multiple images on both boards. Compilation adds time. These are bench measurements on one Mac, not a campus reliability guarantee.

## Failures retained in the local evidence

An Arduino preprocessor problem was removed by keeping implementation in main.cpp. Debug serial output was made nonblocking so it could not stall BLE without a reader. Initial fast class-board transfers stopped at40/60%; the incomplete images were not activated. Cumulative acknowledgments, smaller flow-controlled groups and negotiated-MTU/paced UART reports were tested afterward. Extended peripheral suites passed three consecutive runs.

After face controls were added, the deliberately truncated-image test stalled twice in SDK validation. USB ROM reset recovered the application. Its image header declared segments larger than the authenticated1024-byte payload. A structural segment/trailer bounds check was added before SDK validation. The rebuilt FaceSafe updater rejected the same malformed payload as `ERR image bounds`, remained responsive, and passed the complete invalid-update/disconnect suite. Full SDK validation remains required for structurally complete images.

FaceControl was accepted over BLE in78.4seconds; the initial synthetic face stream returned acknowledgments in67–149ms and a second run76–150ms. FaceSafe was then accepted in69.4seconds and its face/USB suite passed. Default neutral eye hash remained a1bf2f0d. These synthetic tests validate mapping and radio/render state, not physical sticks/buttons.

Review added a pending-stop delivery rule, reconnect invalidation, selected-profile log-directory creation and bounded failed-frame retry. Host tests cover the mapping/coalescing/stop policy, image bounds, frame retry/backoff, command parsing and uploader validation. No physical I2C-fault injection or gamepad unplug test has been performed. The native UI showed zero-controller waiting and its Stop button received a neutral FACE acknowledgment. GUI checks then stopped when the Mac locked; local execution continued. A background LaunchServices startup of the new alias app stalled on a filesystem operation; launching through the local executor worked. This remains a first-launch/portability limitation.

## Practical limits

Remote USB/I2C evidence does not prove OLED glass appearance or physical touch feel. The OLED controller/wiring came from preserved class source; address0x3C alone is not an ID readback. The local alias app is not a portable installer. Fresh classroom provisioning on other Macs/boards, complete Xbox trigger/button/unplug qualification, iPad clients/uploader, an AI build-delivery service and direct controller-to-XIAO use remain separate tests.

Private raw logs, exact device profiles, key material and flash images stay in the owner's local working projects. The public source contains a synthetic identity fixture and reusable tests, not original firmware or secrets.

The generic bare repository source compiled, transferred581520 bytes in139.4seconds and reconnected as OpenToyBare. Its final live and invalid-update suites passed. Its unique advertised name is derived from the factory MAC, so the source is no longer hardcoded to a bench board.

Final generic OLED source was wirelessly accepted in70.8seconds and reconnected as OpenToyFaceFinal. Its full face/USB, peripheral-regression and invalid-update suites passed. The final face run measured69–118ms acknowledgments and verified that a real BLE disconnect from a non-neutral pose produced neutral FACE state over USB. This is a transport-disconnect test, not a physical gamepad unplug test.

## Physical Xbox connection added

The controller uses USB VID045e/PID0b12. Apple's own driver identifies the Xbox Series X family; its GameController API exposes vendor `Controller` and category `Xbox One`. Exact printed model/firmware remain unverified. With a Cocoa main run loop active, real stick motion spanned both axes -1..+1 and left/right D-pad events were recorded. These events occurred before the toy connection; they are not counted as end-to-end movement validation.

After selecting the OLED toy and disabling touch input, the native app sent the actual neutral controller state. BLE acknowledged seq58 in76ms with txerr0. Independent USB reported `FACE seq=58 x=0 y=0 left=0 right=0 expression=neutral` and `PERIPH seq=58 ... frame=a1bf2f0d i2c_ok=1 errors=0`. The bare toy had been unplugged when this controller was attached; only the OLED/touch board was targeted. Physical trigger/button and gamepad-unplug tests remain pending.

With the controller still connected and its adapter enabled, **Build + update** compiled642048bytes, transferred them over BLE, rebooted and verified `Workshop-20261001-163405` in73.0seconds (compilation added19seconds). The adapter then reapplied its neutral pose; independent USB corroborated the new build and FACE seq1. Controller streaming pauses while build/upload owns the connection. This verifies update/reconnect with stationary input; reapplying a held non-neutral control remains open.

The earlier physical stick/D-pad recording was replayed through the real BLE client and corroborated pose-by-pose over independent USB. All41 selected poses passed, with22 frame hashes and68–123ms acknowledgments; txerr0 and I2C errors0 throughout. This is a recorded-input replay, not simultaneous live movement. The raw recording and exact executed bench scripts are retained in the private local evidence folder. The reusable `tools/gamepad_probe.py` was run on this Mac and recognized the connected native gamepad.

## Independent whites, pupils and lids

The owner reported that the original live smoke test worked well, then approved right-stick left/right winks, up closure from the bottom, down closure from the top, and release-to-open. Implemented an independent `eyes` command and pixel renderer; the legacy `face` command still selects its original drawing. Left-stick Y now moves whites, D-pad moves pupils, and right-stick X/Y controls proportional lids. Default neutral/disconnected state has centered pupils; its frame hash is f2edb589. The pre-change source ZIP and private source/build recovery copies were preserved.

EyeLayers compiled into642880bytes, SHA25615425dac69e809320e53185528438237331c5221b200d1fa1ab582bd1d6bff97. The existing updater authenticated and accepted it in70.9seconds, then reconnected and verified buildEyeLayers/eyes1. No USB flashing or whole-flash erase was needed for this change.

The new hardware suite matched15 exact framebuffer references over independent USB: whites up/down, pupils in four directions, each wink, half lids from both edges, full closure, combined controls and expressions. Another20 combined synthetic poses passed. Invalid pupil/lid/edge/expression values preserved all eye fields and framebuffer state. A real BLE disconnect restored centered whites/pupils and open lids. New command acknowledgments ranged58–177ms; txerr0 and I2C errors0 were reported throughout. These are synthetic-input radio/render tests, not a claim that a person operated each physical control.

The legacy face suite, extended touch/display suite and invalid-update suite all passed again on EyeLayers. Host mapping/stop/prerequisite/parser/pixel/retry/uploader tests passed. Read-only review caught controller-enable and unsupported-reconnect ordering issues; they were fixed and checked. Input starts only after acknowledged Touch off and `eyes=1` negotiation.

The native app was restarted, selected the class toy, acknowledged Touch off, and enabled the attached Xbox. Its neutral `eyes` command (seq98,89ms) matched USB FACE/PERIPH state and f2edb589.

The owner's subsequent real controller movements produced82 live USB-correlated poses (seq98–179), captured alongside428 native input changes. All82 actual display hashes matched the pixel reference for their reported eye state; touch0/I2C errors0 held throughout. D-pad moved pupils in all four cardinal directions. Actual right-stick input produced independent partial left/right winks and closure from both edges, then neutral release. Actual left-stick movement moved the whites; the new-map capture did not include its full upward/downward range. Recorded right Y spanned-1..+1; right X spanned about-0.90..+0.99. This is simultaneous physical controller → Mac → BLE → independent USB evidence, separate from the synthetic tests.

Raw capture, exact executed capture/reference sources and physical-summary JSON are preserved privately. Physical expression-button/unplug and held-input-update qualification remain open. No remote OLED-glass observation is claimed for this version.

Owner confirmation, October 1, 2026: after the EyeLayers controls were installed and the live test recorded, the owner reported “Works great.” This is a human confirmation of the prototype's behavior on the toy, alongside the independent USB/frame evidence. It does not extend the recorded range or qualify the remaining unplug/update/classroom checks.
