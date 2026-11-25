# Live control and authenticated Bluetooth updates

## Live commands

BLE UART uses Nordic UART Service UUID `6e400001-b5a3-f393-e0a9-e50e24dcca9e`; RX ends0002 (write/write-without-response), TX ends0003 (read/notify). Commands end with `;`, CR or LF. Text frames are bounded to63 characters. Queue depth and per-loop frame processing are bounded. Device Information serial-number characteristic reports the factory MAC, which the client verifies before controlling/uploading.

Bare controls: `period 100..10000`, `duty 1..99`, `mode blink|on|off`, `status`, `reset`. Checksummed Adafruit Bluefruit button packets1–8 are supported for the bare behavior mappings.

OLED controls: `screen eyes|closed|test|off`, `contrast 0..255`, `touch threshold 1000..2000000`, `touch on|off`.

Atomic face command: `face X Y LEFT RIGHT EXPRESSION`. X/Y are integers-100..100 (positive Y is down). Eyelids are0..100 (0 open,100 closed). Expression is `neutral`, `happy`, `angry` or `surprised`. Invalid commands change no face field. Accepted face commands select the eye screen. `FACE seq=...` reports actual face state; `PERIPH` reports display hash, raw touch measurements, touch mask and I2C state/errors.

Independent eye-layer command: `eyes X Y PX PY LEFT RIGHT EDGE EXPRESSION`. White X/Y and pupil PX/PY are integers-100..100; positive Y/PY is down. Lids are0..100. EDGE is `bottom` or `top`; it chooses the edge from which black covers the white. Each command atomically supplies all fields and selects the eye screen. Firmware advertises `eyes=1` in status/heartbeat. `FACE` adds `px`, `py`, `pupils` and `edge`. Default/disconnected eye state includes centered pupils; legacy `face` selects the original no-pupil drawing and top lids, preserving earlier clients. Partial lids are proportional; full closure draws a thin line.

The new Xbox mapping moves only white Y with the left stick; D-pad changes PX/PY; the right stick combines independent winks and two lid directions. Pupils cut only existing white pixels. Touch is disabled and acknowledged before controller streaming. With touch off, the28×32 white shapes stay within the128×64 display across the full14pixel vertical travel.

Face v2 command: `doodle EYE BROW MOUTH SKEW JAW LEFT RIGHT EXPRESSION`. First four fields are integers -100..100; jaw and independent lid amounts are 0..100. Expression is `neutral|happy|surprised|angry|sorry|nervous`. All fields are validated before assignment; invalid input changes none. It selects the eye screen, now drawn as a complete line face. `FACE` adds `doodle`, `eye`, `brow`, `mouth`, `skew`, `jaw`; `PERIPH` adds `settled`. Status advertises `doodle=1`. Desired fields change immediately; framebuffer hash evolves until settled, then equals the pure renderer target. Default/disconnected Face v2 state is neutral procedural lines; explicit legacy `face`/`eyes` commands retain their earlier renderer. The update protocol and partition layout are unchanged.

UART notifications are chunked for negotiated MTU, paced and retried on known GATT submission failures. `txerr` exposes such failures. Missing acknowledgments fail the operation instead of claiming success.

## Update protocol1

Custom service `8e7c0001-54ad-4da3-97f4-754ac67c0a00`; packet characteristic ends0002, status ends0003. This is not Nordic DFU or a generic Espressif OTA app protocol.

| Opcode | Payload/action |
| --- | --- |
| 0 HELLO | Fresh16-byte nonce and current received offset |
| 1 START | Little-endian size4 bytes, SHA25632 bytes, HMAC-SHA25632 bytes over nonce+size+hash |
| 2 DATA | Little-endian offset4 bytes followed by image bytes |
| 3 FINISH | Require exact byte count/hash, bounded ESP image segments/trailer and SDK validation; stage image |
| 4 ABORT | Discard active/staged session |
| 5 REBOOT | Select validated staged slot, acknowledge, then reboot after a short delay |

The uploader verifies target, device MAC, peripheral configuration, image size/hash and recovery-test flag from a local manifest. The firmware writes only the inactive application slot. FINISH alone does not select it. Four-packet groups use a GATT response on the final packet and cumulative offset checks. A missing final notification can be checked against the device's current offset; a stall or disconnect fails the transfer. No resume across disconnect is implemented: reconnect and restart at zero.

Active transfers abort after15 seconds idle or disconnect. Boot validation is deferred until application self-tests, BLE operation and a settling period succeed. OLED startup also requires successful display initialization/frame transfer. An unconfirmed failing boot rolls back to the previous valid application. USB ROM recovery remains available even when a new application damages Bluetooth.

## Prototype security boundary

Each toy has a unique32-byte secret, shared only with its local uploader. Firmware updates require a valid HMAC. Live controls are unpaired and have no authentication; nearby clients can change live behavior. The updater does not add payload encryption, secure boot or signed releases. Physical USB/flash access can expose the key. Publicly distributing compiled images from this prototype would publish that device's key.

Future classroom/fleet work should add an appropriate ownership/pairing mechanism before controls affect motors or other physical outputs. Keep keys out of Git, logs, AI prompts and shared archives.
