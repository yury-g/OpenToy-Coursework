# OpenToy

Expressive toy apps across small color screens, round touchscreens, reflective LCDs and larger touch panels. **Shared behavior, familiar controls, device-specific hardware support.**

[All projects and source locations](projects/README.md) · [Shared controls](docs/CONTROLS.md) · [App status bar](docs/APP-STATUS-BAR.md) · [Roadmap](docs/ROADMAP.md) · [Device-family strategy](docs/DEVICE-FAMILY-ARCHITECTURE.md)

## Devices and capabilities

Snapshot: **2026-11-17**. Hardware capability, installed firmware and owner-confirmed behavior are different things. Each device guide carries its detailed versions and checks; the tables below summarize the family without claiming every feature is ready.

| Device | Display and direct input | Motion / camera | Sound | Battery information |
| --- | --- | --- | --- | --- |
| [Reverse TFT Feather ESP32-S3](projects/feather/README.md) | 240 × 135 color; QT gamepad in current Eyes builds; onboard buttons unassigned | Motion needs an add-on; no built-in touchscreen | Current Eyes assembly has no speaker; mic/speaker add-ons to qualify | MAX17048 gauge estimate where fitted/working; charging is a separate check |
| [Arduino Nesso N1](projects/nesso/README.md) | Color touchscreen; poke either eye | Eyes uses motion sensing for smooth orientation | Sound exists in the Water project; microphone response needs qualification | Local reading in existing app; shared-header migration pending |
| [Waveshare 1.85C BOX, V2](projects/waveshare-1.85c/README.md) | 360 × 360 round color touch | Fixed orientation in current Eyes; no motion port | Quiet reaction playback implemented; mic disabled pending qualification | Measured volts or unknown; no verified percentage/charging flag |
| [Waveshare ESP32-S3-RLCD-4.2](https://github.com/yury-g/OpenToy-Coursework/tree/device/waveshare-rlcd-4.2/projects/waveshare-rlcd-4.2) | 400 × 300 landscape reflective monochrome; no backlight; KEY user button | Fixed orientation; no implemented touch/motion input | Audio hardware track; capture/playback not qualified in Eyes | Calibrated battery-node voltage; owner observed steady CHG with supplied battery reinstalled/USB connected (2026-11-17); firmware charging unknown |
| [Elecrow CrowPanel P4, 9-inch V1.0](projects/crowpanel/README.md) | 1024 × 600 color touch; XIAO not needed for local Eyes | Camera optional, connected-camera operation unverified; no qualified automatic orientation | Audio qualification planned | BAT --; controller candidate voltage is diagnostic only; percentage/charging unverified |
| XIAO ESP32-S3 companion | Optional separate controller/sensor/radio role; not a display target here | Depends on attached hardware | Depends on attached hardware | Do not substitute its reading for the panel battery |

**Latest device work:** CrowPanel [smooth Eyes 0.2.2](projects/crowpanel/EYES1.md) is installed with verified app-only recovery, live controls and reset defaults; battery remains unknown. RLCD Eyes 0.2.0 adds native contours, black background and KEY-only menus; see its [verification record](https://github.com/yury-g/OpenToy-Coursework/blob/device/waveshare-rlcd-4.2/projects/waveshare-rlcd-4.2/VERIFICATION.md) for installed/live and owner-observation status. Copy-Face is in development. CrowPanel’s [verification record](projects/crowpanel/EYES1-VERIFICATION.md) is current. The RLCD link explicitly points to its device branch. RLCD now has an owner-observed hardware charging indication (steady CHG); firmware cannot read CHG. CrowPanel charging remains unconfirmed.

## App support matrix

**Live** = device operation recorded, with remaining physical checks in its guide. **Source** = existing project/release; not a fresh deployment claim. **Developing** = implementation work, not a tested camera/app. **Assess** = requires a port and capability check. No universal app launcher is installed.

| App | Feather | Nesso | Round Waveshare | RLCD 4.2 | CrowPanel P4 |
| --- | --- | --- | --- | --- | --- |
| [Animatronic Eyes 1](projects/animatronic-eyes/README.md) | Live: Curious / Dreamy / Cheeky | Live: touch + orientation | Live: touch + quiet reactions | Live; smooth-render update underway | Live 0.2.2; smooth renderer, positive owner feedback; detailed multi-touch check open |
| [Animatronic Eyes 2](projects/animatronic-eyes/README.md) | Live/source: QT gamepad | Assess | Assess | Assess | Assess |
| [Gravity Water](https://github.com/yury-g/OpenToy-Coursework/tree/feature/gravity-water-feather/projects/gravity-water) | Live: add-on motion | Live: touch + motion | Assess: motion missing | Assess: motion missing | Assess: motion missing |
| [Orange Face](https://github.com/yury-g/OpenToy-Coursework/tree/feature/orange-face/projects/orange-face) | Source: motion + touch-pad add-ons | Assess | Assess | Assess | Assess |
| [Nearby Buddy](https://github.com/yury-g/OpenToy-Coursework/tree/main/companions/nearby-buddy/main) | Source: two peers; distance calibration unfinished | Assess | Assess | Assess | Assess: radio transport + peer |
| [Battery Faces](https://github.com/yury-g/OpenToy-Coursework/tree/main/companions/battery-faces/main) | Source: gauge/gamepad/haptic variants | Assess | Assess | Assess: voltage adaptation | Assess: battery interface |
| [Nesso Face](https://github.com/yury-g/OpenToy-Coursework/tree/feature/nesso-n1/projects/nesso-n1) | Assess | Source | Assess | Assess | Assess |
| **Copy-Face** | Not targeted yet | Not targeted yet | Not targeted yet | Not targeted yet | Developing: camera face-following eyes + copied mouth shapes |

Copy-Face first needs a verified camera and usable face/mouth analysis on the panel. Sound-triggered alertness and expressive listening are [family roadmap items](docs/ROADMAP.md), not completed capabilities. Physical prints are listed separately under [3D printed designs](projects/3d-printed-designs/README.md).

## One control language

**BOOT and RESET are recovery controls, never normal app controls.** Use exposed, qualified user buttons and touchscreens. The following is the new design standard; existing mappings listed below still need versioned migration.

| Action | User buttons | Touchscreen |
| --- | --- | --- |
| Play / react | Short Primary press performs the labelled app action | Touch the object: poke an eye, interact with water |
| Menu | Hold Primary for 0.8 seconds | Tap visible **Menu** in the safe header |
| Navigate / select | Next / Previous / Select; one-button devices use short-next, hold-select | Tap labelled items |
| Back | Back button or explicit Back item | Back item |
| Performance | **Display → Performance** hides information and control bars | Same menu item |
| Restore from performance | Hold Primary to restore information and open Menu | Hold top-center safe edge for 0.8 seconds |

Same menu order: **Return to play · Apps · Personality · Sound · Display · Controls · About**. Unsupported options are explained or omitted. Apps only launches firmware actually available on that device. Menu layouts scale to the screen; labels and meaning stay consistent. [Full rules, one-button navigation and acceptance checks](docs/CONTROLS.md).

## Controls today and migration

| Device | Current normal-play controls | Current menu / information controls | Next shared-control step |
| --- | --- | --- | --- |
| Feather Eyes | Stick, A/B/X/Y, Select/Start modifiers; [full mapping](projects/animatronic-eyes/README.md#controls) | No common menu; onboard buttons unassigned | Qualify exposed user buttons; add Menu. Gamepad entry needs conflict testing with the existing blink chord |
| Nesso Eyes | Poke eyes; motion rotates the face | No common menu documented | Add visible touch Menu and the shared performance restore gesture |
| Round Waveshare | Poke either eye | Top-cap hold toggles information; lower-left hold mutes; lower-right hold changes personality | Put settings in the labelled common menu; preserve eye pokes |
| RLCD Eyes 0.2.0 | KEY short: brief reaction; hold 0.8s: Menu; BOOT/RESET reserved | Menu short Next / hold Select; explicit Back; Display → Performance; hold KEY restores | KEY-only migration implemented; per-eye actions under Apps; see device verification for live checks and pending owner physical confirmation |
| CrowPanel Eyes | Poke eyes; touch personality/eco/information buttons | Footer controls; no common family menu yet | Consolidate settings into Menu; hide all control bars for performance; no BOOT/RESET mappings |
| Copy-Face | Planned camera-driven eyes and mouth | UI being developed | Adopt shared touch menu from first maintained UI; camera confidence/loss does not require a button |

Historical controls remain documented so existing installations are usable; they are not the new family standard. Each device migration must update its version, controls guide and tests. User buttons that double as boot straps must be excluded even if their board label is ambiguous.

## See the projects

| Eye personalities | Touch and rotation | Water on two devices |
| --- | --- | --- |
| [![Feather eye personalities](projects/animatronic-eyes/docs/images/feather-personalities.gif)](projects/animatronic-eyes/README.md) | [![Nesso eye behavior](projects/animatronic-eyes/docs/images/nesso-eyes-behavior.gif)](projects/animatronic-eyes/NESSO.md) | [![Gravity Water](docs/images/gravity-water-cross-platform-play.gif)](https://github.com/yury-g/OpenToy-Coursework/tree/feature/gravity-water-feather/projects/gravity-water) |

These are scripted source/model previews, not camera recordings of current installations. Further artwork, controls, wiring and evidence live in the project guides rather than repeated galleries here.

## Find source, history and recovery

[All projects and source locations](projects/README.md) distinguishes files on main from device/feature branches and existing standalone repositories. Downloading main does not include every app. Preserve original releases and renderer provenance when making a new port.

[3D printed designs](projects/3d-printed-designs/README.md) · [Historical XIAO / pre-Feather archive](archive/README.md) · [Playtesting](docs/PLAYTESTING.md) · [License](LICENSE) · [Third-party licenses](licenses/README.md)

The archived XIAO projects are distinct from the new optional CrowPanel companion. Keep device identities, credentials, compiled firmware, raw logs and flash backups private. Each device track owns its hardware operations; adding a README row does not make firmware interchangeable.

## Bill of materials — devices and sourcing

Choose the device for the app you want to build, then add only that project's accessories. These are alternatives, not one combined shopping list. Quantities below are **per device** unless a pair is explicitly stated. Manufacturer links and model names were checked **2026-11-17**; prices, stock and supplied revisions can change. Use the [app support matrix](#app-support-matrix) and linked build guides to check readiness before choosing hardware.

### Main device family

| Device to source | Quantity / role | Manufacturer source | Match before buying |
| --- | --- | --- | --- |
| **Adafruit ESP32-S3 Reverse TFT Feather — #5691** | 1 per Feather toy; **2 for Nearby Buddy** | [Adafruit shop](https://www.adafruit.com/product/5691) | Reverse TFT model, 4 MB flash / 2 MB PSRAM. Display is included. See [Feather projects](projects/feather/README.md) for each app's add-ons. |
| **Arduino Nesso N1 — TPX00227** | 1 per Nesso toy | [Arduino shop](https://store.arduino.cc/products/nesso-n1) | ESP32-C6 Nesso N1; built-in screen, touch and motion sensing. [Nesso projects](projects/nesso/README.md). |
| **Waveshare ESP32-S3-Touch-LCD-1.85C-BOX-EN — 30685, V2** | 1 per round-screen toy | [Waveshare shop](https://www.waveshare.com/esp32-s3-touch-lcd-1.85c.htm) · [model/revision guide](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.85C) | Select **BOX-EN**, V2 ES8311 audio; standard board 30683 and BOX 30684 are different packages. Current [OpenToy adapter](projects/waveshare-1.85c/README.md) also requires the enrolled case2 panel initialization. Confirm battery contents separately. |
| **Waveshare ESP32-S3-RLCD-4.2 — 33298 / -EN 33507** | 1 per reflective-screen toy | [Waveshare shop](https://www.waveshare.com/esp32-s3-rlcd-4.2.htm) · [manufacturer guide](https://docs.waveshare.com/ESP32-S3-RLCD-4.2) | 4.2-inch reflective LCD, not an e-paper panel. Confirm package/battery contents and board revision. [OpenToy source and setup are on `device/waveshare-rlcd-4.2`](https://github.com/yury-g/OpenToy-Coursework/tree/device/waveshare-rlcd-4.2/projects/waveshare-rlcd-4.2). |
| **Elecrow CrowPanel Advanced 9-inch ESP32-P4 — DHE04209D, V1.0** | 1 per large-screen toy | [Elecrow shop](https://www.elecrow.com/crowpanel-advanced-9inch-esp32-p4-hmi-ai-display-1024x600-ips-touch-screen-wifi-6-support.html) · [manufacturer guide](https://media-cdn.elecrow.com/wiki/CrowPanel_Advanced_9inch_ESP32-P4_HMI_AI_Display_1024x600_IPS.html) | OpenToy's enrolled target is **V1.0**. The listing spans revisions; do not assume a newly supplied panel matches its wiring. [Panel guide](projects/crowpanel/README.md) · [V1.0 wiring](projects/crowpanel/HARDWARE.md). Camera is optional. |
| **Seeed Studio XIAO ESP32-S3 — 113991114** | 0 or 1 optional CrowPanel companion | [Seeed shop](https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html) · [manufacturer guide](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/) | Standard ESP32-S3, not automatically Sense/Plus or another XIAO chip. Confirm actual revision and flash size. Local CrowPanel Eyes needs no XIAO; follow [separate-power/connection guidance](projects/crowpanel/HARDWARE.md). |

### Project accessories

| Part to source | When / quantity | Manufacturer source and build notes |
| --- | --- | --- |
| **Adafruit Mini I2C Gamepad with seesaw — #5743** | 1 for Feather Eyes 2 or classic Eyes 1; optional for Eyes 1 personalities and Feather Gravity Water | [Adafruit shop](https://www.adafruit.com/product/5743) · [Eyes controls/build](projects/animatronic-eyes/README.md) · [Water setup](https://github.com/yury-g/OpenToy-Coursework/blob/feature/gravity-water-feather/projects/gravity-water/FEATHER.md). Battery Faces gamepad code remains on its separate [`feature/seesaw-gamepad` branch](https://github.com/yury-g/OpenToy-Coursework/tree/main/companions/battery-faces/feature/seesaw-gamepad). |
| **Adafruit LSM6DS3TR-C 6-DoF IMU — #4503** | 1 for Feather Gravity Water or Orange Face | [Adafruit shop](https://www.adafruit.com/product/4503) · [Water mounting/wiring](https://github.com/yury-g/OpenToy-Coursework/blob/feature/gravity-water-feather/projects/gravity-water/FEATHER.md). Secure the sensor to the toy and follow the documented axis orientation. |
| **Adafruit MPR121 12-Key Gator touch breakout — #4830** | 1 for Orange Face | [Adafruit shop](https://www.adafruit.com/product/4830) · [Orange Face hookup](https://github.com/yury-g/OpenToy-Coursework/blob/feature/orange-face/projects/orange-face/README.md#plug-in-the-sensors). This is the Gator/QT board with 12 touch pads; the [printed faceplate](projects/3d-printed-designs/orange-face/README.md) exposes its back electrodes. Separate external electrodes/leads depend on your enclosure design. |
| **STEMMA QT / Qwiic cables, 4-pin JST-SH, 1.0 mm pitch** | 1 per connection: 1 for Feather → gamepad; 2 for Feather → MPR121 → IMU | [Adafruit #4210, 100 mm](https://www.adafruit.com/product/4210) is one sourcing option; choose length to fit the build. Adding another board requires its own connection. These are not larger JST-PH/Grove cables. |
| **SparkFun Qwiic Button** | 1 per equipped Battery Faces Feather; 2 for the Blue/Green pair | Manufacturer options: [red LED BOB-15932](https://www.sparkfun.com/sparkfun-qwiic-button-red-led.html) / [green LED BOB-16842](https://www.sparkfun.com/sparkfun-qwiic-button-green-led.html). The project records the Qwiic Button interface, not the fitted color SKU; color is not a board identity. [Accessory setup/source](https://github.com/yury-g/OpenToy-Coursework/tree/main/companions/battery-faces/feature/qwiic-accessories/docs/qwiic-accessories.md). |
| **SparkFun Qwiic Haptic Driver, DA7280 — ROB-17590** | 1 per equipped Battery Faces Feather; 2 for Blue/Green | [SparkFun shop](https://www.sparkfun.com/sparkfun-qwiic-haptic-driver-da7280.html). Includes the soldered LRA motor used by the [accessory branch](https://github.com/yury-g/OpenToy-Coursework/tree/main/companions/battery-faces/feature/qwiic-accessories); no separate motor is needed for that configuration. Use its wiring and motor settings. |
| **CrowPanel camera module** | Optional for the panel; required for real camera-driven Copy-Face | Use the optional camera offering on the [Elecrow panel listing](https://www.elecrow.com/crowpanel-advanced-9inch-esp32-p4-hmi-ai-display-1024x600-ips-touch-screen-wifi-6-support.html), after confirming V1.0 connector/module compatibility. **Exact fitted sensor and a separately orderable, verified module SKU are unresolved**; see [camera evidence](https://github.com/yury-g/OpenToy-Coursework/blob/feature/copy-face/projects/copy-face/docs/CAMERA-RESEARCH.md). A generic USB or XIAO camera is not the documented native CSI path. |

For a complete **Nearby Buddy pair**, start with two #5691 boards and use the [standalone project's parts/enclosure guide](https://github.com/yury-g/OpenToy-Coursework/tree/main/companions/nearby-buddy/main). For **Orange Face**, the documented electronics are one #5691, one #4503, one #4830 and two QT cables. Printed parts and their fit status are in the [3D design directory](projects/3d-printed-designs/README.md). Firmware on feature branches and in standalone repositories is not included merely by downloading OpenToy `main`.

### Related Battery Faces hardware

These belong to the [separate Battery Faces family](https://github.com/yury-g/OpenToy-Coursework/tree/main/companions/battery-faces/feature/qwiic-accessories#current-family). Their presence there does not establish an OpenToy Eyes/Water port. Feather, Nesso and RLCD sources are already listed above.

| Device / role | Quantity for that role | Source and selection note |
| --- | --- | --- |
| **M5Stack Tab5 — C145 / Kit K145**, dashboard | 1 dashboard | [M5Stack shop](https://shop.m5stack.com/products/m5stack-tab5-iot-development-kit-esp32-p4) · [revision guide](https://docs.m5stack.com/en/core/Tab5). Display/touch revisions have changed; match the project's driver before substituting a newer unit. Ancillary monitor, not required for standalone OpenToy toys. |
| **M5Stack StickS3 — K150**, Orange reporter | 1 | [M5Stack shop](https://shop.m5stack.com/products/m5sticks3-esp32s3-mini-iot-dev-kit). This role is separate from the Orange Face Feather app. |
| **Waveshare ESP32-S3-Touch-LCD-1.69 — 27350**, Yellow/Rainbow reporters | 1 per reporter; 2 to reproduce both roles | [Waveshare shop](https://www.waveshare.com/esp32-s3-touch-lcd-1.69.htm) · [revision/pin guide](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.69/FAQ). Match the project's printed-model revision; do not substitute the non-touch LCD-1.69. |
| **ESP32-2432S028 CYD, dual micro-USB + USB-C, resistive touch**, Purple companion | 1, USB powered in the documented build | [Project hardware reference](https://github.com/yury-g/OpenToy-Coursework/tree/main/companions/battery-faces/feature/qwiic-accessories/hardware/BOARDS.md). Exact manufacturer SKU/store is unverified; generic “CYD” listings span different displays and pin mappings. |

Historical XIAO/OLED/mechanical builds remain in the [archive](archive/README.md), with their original parts and source paths. Battery Faces' discontinued camera experiment remains on [`archive/camera-test-v17`](https://github.com/yury-g/OpenToy-Coursework/tree/main/companions/battery-faces/archive/camera-test-v17); its XIAO Sense camera is not needed by the current battery-only/accessory builds or the optional CrowPanel companion above.

**Power and cables:** use a USB **data** cable matching the selected device and its setup guide. CrowPanel's programming connection is **UART0 USB-C**; follow its [power instructions](projects/crowpanel/README.md#first-usb-connection). Battery packs are device-specific: match chemistry, voltage, connector polarity, charge-rate requirement and enclosure fit. For Feather, follow [Adafruit's battery guidance](https://learn.adafruit.com/esp32-s3-reverse-tft-feather/pinouts) and the [OpenToy power notes](projects/animatronic-eyes/docs/POWER.md), including the 250 mAh minimum for the stock charging setup. For other devices, use the manufacturer guides linked above and check what the chosen package includes. There is no universal OpenToy battery kit.
