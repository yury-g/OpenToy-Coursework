# CrowPanel project compatibility and test checklist

Baseline: OpenToy `main` 0925b22702588b4e170190cc8b97325ca52e3857,
refreshed 2026-11-17; [active directory](../README.md). These are separate apps,
not a finished multi-app launcher. **Eyes 1 0.2.2 is the last verified standalone installation with live
checks; [0.3.0 Menu source/build](MENU.md) awaits hardware handback and deployment; every other CrowPanel app remains unbuilt and uninstalled.** Owner visual
and physical-touch observations are tracked separately. Existing Feather/Nesso
results do not transfer.

Native means the app and display run on P4 after porting. Companion means the
external XIAO handles a specific input/radio role and exchanges bounded messages
with P4. Neither means source or binaries are already compatible.

| Active project / source | Native P4 assessment and missing hardware | Possible XIAO role | Project acceptance checks; Eyes 1 evidence below, others pending |
| --- | --- | --- | --- |
| [Animatronic Eyes 1](../animatronic-eyes/README.md), including [Nesso touch](../animatronic-eyes/NESSO.md) | Best first port: reuse eye model, personalities and reactions; replace display/touch drivers. Motion-based rotation needs an external IMU; fixed orientation can be explicit initial scope | None for screen, idle or touch; optional gamepad bridge later | All three personalities; 2× pupils and raised lids; distinct idle blink rhythms; left/right and simultaneous pokes; independent anger/recovery; touch release on failure; readable status; always-visible eco; serial backpressure |
| [Animatronic Eyes 2](../animatronic-eyes/README.md) | Reuse doodle springs and curves. QT gamepad needs a verified adapter/cable and 3.3V bus; touchscreen controls would be a new mapping | Optional gamepad bridge; not required if gamepad is wired safely to P4 | Every documented modifier/preset, jaw and wink layer; dead zone; disconnect/reconnect; frame bounds; no stale held controls |
| [Gravity Water — Feather](https://github.com/yury-g/OpenToy-Coursework/tree/feature/gravity-water-feather/projects/gravity-water) | Shared particles can run natively. Add LSM6DS3TR-C or another qualified IMU for actual tilt/shake; optional QT gamepad. Simulated gravity is not a motion port | Optional IMU/gamepad sampling bridge | 400 particles; tilt axis/sign after fixed sensor mounting; heat/cooling; shake; cursor; sensor loss; performance with full panel resolution |
| [Gravity Water — Nesso](https://github.com/yury-g/OpenToy-Coursework/tree/gravity-water-v0.3.0) | Same particle family, touchscreen and audio adaptation. Panel touch is available in hardware; actual motion still needs an IMU | Optional sensor bridge | Touch-to-particle alignment; heat input choice; shake/tilt; audio output and mute; simultaneous input; no claim that Nesso binaries run here |
| [Orange Face](https://github.com/yury-g/OpenToy-Coursework/tree/feature/orange-face/projects/orange-face) | Native face/reactions. Needs external IMU and MPR121 with electrodes for physical tickle behavior. On-screen zones are an explicitly different input version | Optional MPR121/IMU bridge | Three tickle zones and combinations; shake+dizzy; orientation; shade control; sensor disconnection; external NeoPixel only if fitted and qualified |
| [Nesso Face](https://github.com/yury-g/OpenToy-Coursework/tree/feature/nesso-n1/projects/nesso-n1) | Native renderer/touch zones; replace M5-specific services. IMU needed for gravity/shake; shade buttons need documented replacement controls | Candidate BLE-to-serial bridge if factory C6 transport lacks needed service | Touch thirds; shade controls; rotation with actual IMU; bounded command parsing; local/remote mode; reject invalid poses; radio loss; physical peer test only with a separately authorized peer |
| [Nearby Buddy](https://github.com/yury-g/OpenToy-Coursework/tree/main/companions/nearby-buddy/main), [OpenToy chapter](https://github.com/yury-g/OpenToy-Coursework/tree/feature/nearby-buddy/projects/nearby-buddy) | Native face feasible; BLE advertising/scanning must be supported through a verified radio path. Requires a second authorized peer. P4 has no native radio | Candidate BLE proximity radio; shared serial protocol and enrollment required | Enrollment rejection; reports/age; loss and recovery; actual distance/orientation/obstruction trials; calibration for new antenna/enclosure; never reuse Feather RSSI thresholds as verified distances |
| [Battery Faces — base](https://github.com/yury-g/OpenToy-Coursework/tree/main/companions/battery-faces/main) | Best initial role is USB-powered viewer. Local panel battery telemetry unverified; factory charger/management MCU is not a proven gauge API. External repository stays separate | Candidate ESP-NOW receiver/gateway; factory C6 Wi-Fi may support dashboard transport, subject to testing | Correct source labels and freshness; stale/reordered/duplicate data; reconnect; local `--`; remote-vs-local battery distinction; no invented SOC, current, capacity or charging state |
| [Battery Faces — gamepad](https://github.com/yury-g/OpenToy-Coursework/tree/main/companions/battery-faces/feature/seesaw-gamepad) | Base viewer plus verified QT gamepad; branch-specific implementation audit required before port | Optional gamepad and/or radio bridge | Joystick gaze; A happy/B sad/X surprised/Y angry; Start neutral/Select battery; missing-controller fallback; peer identity; battery semantics |
| [Battery Faces — buttons/haptics](https://github.com/yury-g/OpenToy-Coursework/tree/main/companions/battery-faces/feature/qwiic-accessories) | SparkFun Qwiic Buttons and DA7280 haptic motors needed. Speaker sound is not equivalent to haptic output; Tab5 dashboard logic needs its own adaptation | Optional button/haptic/radio bridge | Hold buzz and breathing toggle; button LEDs; Find Me; bounded motor output; disconnect stops output; stale network data; remote command bounds |
| [Orange Face printed faceplate](../3d-printed-designs/orange-face/README.md) | Existing dimensions target Feather and do not fit the 9-inch panel. Preserve design as history; any panel enclosure is new CAD | None | Measure actual panel; connector/speaker/touch access; power-switch access; ventilation and cable strain; CAD checks separate from print/physical fit |

Archived [XIAO and earlier projects](../../archive/README.md) remain archived.
The new XIAO companion role does not establish their old firmware is installed
or revive all historical projects. Audit a historical project only when selected.

## Gates for every runnable port

Track evidence separately under the shared
[device-family strategy](../../docs/DEVICE-FAMILY-ARCHITECTURE.md).
Eyes 1's version/date are **0.2.2 / 2026-11-17**. See its
[controls](EYES1.md) and [verification](EYES1-VERIFICATION.md).
The manufacturer screen example's build result is separate from this ledger.

| App | Host tests / target build | Installed | Live checks | Owner observation |
| --- | --- | --- | --- | --- |
| Eyes 1 | Host suite + P4 build pass | 0.2.2 | Animation, synthetic pokes/recovery, personas, header/ECO, timeout and serial load pass | Owner reported smooth 0.2.0 looks great; no detailed multi-touch observation |
| Eyes 2 | Pending | No | Pending | Pending |
| Gravity Water — Feather | Pending | No | Pending | Pending |
| Gravity Water — Nesso | Pending | No | Pending | Pending |
| Orange Face | Pending | No | Pending | Pending |
| Nesso Face | Pending | No | Pending | Pending |
| Nearby Buddy | Pending | No | Pending | Pending |
| Battery Faces — base | Pending | No | Pending | Pending |
| Battery Faces — gamepad | Pending | No | Pending | Pending |
| Battery Faces — buttons/haptics | Pending | No | Pending | Pending |

- [ ] **Sources:** pin the maintained app revision and board-support revision;
  preserve license notices; distinguish external repositories and branches.
- [ ] **Build/software:** compatible P4 toolchain and dependencies pinned;
  host model/input tests pass; image fits the live app slot; builds stay private.
- [ ] **Presentation:** meet the [shared status-bar standard](../../docs/APP-STATUS-BAR.md),
  including truthful local battery, exact version/date, reboot default, hide and
  restore, long labels, content bounds and touch transforms in both states.
- [ ] **Live telemetry:** confirm exact installed version, advancing frames,
  memory headroom, touch/input ages, bounded diagnostics and recovery after
  disconnected inputs. Synthetic diagnostic events are labeled synthetic.
- [ ] **Physical observations:** observe screen orientation, readable text,
  touch registration, animation behavior and any speaker/haptic output. Record
  who observed it; frame counters alone do not establish visible correctness.
- [ ] **Durability:** cold boot, repeat reset, sustained run and input/radio loss;
  confirm original recovery and non-app flash preserved after installation.
- [ ] **Revision record:** add version/date/source, what was actually tested,
  limitations and next action. Never change old release tags or archived binaries.

## Suggested sequence

1. Separate-board identification, verified backups and live layout decoding
   are complete. Qualify the V1.0 display/touch support while preserving that layout.
2. Eyes 1 with native touch and fixed orientation; then Eyes 2 with a documented
   input mapping. Demonstrate their status-bar controls and eco visibility.
3. Gravity Water and the other face renderers; add physical IMU/electrodes only
   when those features are being tested.
4. Qualify a radio path and companion wiring before Nearby Buddy or Battery
   Faces networking. A launcher that switches apps is separate future work.

## Source audit record

Reviewed guides in the fresh checkout and branch sources:

| Source | Reviewed checkpoint |
| --- | --- |
| OpenToy main / Eyes | `0925b22702588b4e170190cc8b97325ca52e3857` |
| Gravity Water | `f9402edb624442750fdf2c05d67cc058c34b2911` |
| Orange Face | `44ecd87575de75197d87457c2d00c7d2010a0eac` |
| Nearby Buddy OpenToy chapter | `abc451e2409d5fca5b1468bdc906ab988d61b134` |
| Nesso Face | `7196c33e75886d617a66a8cbef0d919a9cb8ef3f` |
| Shared status-bar standard | `1562dfafa43ed070a21feedd6f85be2ce59c46d7` |
| Device-family strategy | `a0f43e1` |
| Battery Faces main | `af0b776dddb324c1637f6ba5f1f263f5c713ddcd` |
| Battery Faces gamepad | `2305f0eb7f2c8fbf8cdbe847bee2aa7d702f992f` |
| Battery Faces accessories | `f83f2f985f18c4b3b122b9e9fa0e7b7948589256` |

Standalone Nearby Buddy README and authenticated Battery Faces main/gamepad/
accessory READMEs were read on 2026-11-17. Battery Faces implementation-level
port audits remain open. Source inspection is not a compiler check or a live
hardware result. Historical reported installations are not current bench facts.
