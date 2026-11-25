# V1.0 battery evidence — 2026-11-17

## What is established

The owner reports a PKCELL battery marked **3.7 V, 2200 mAh** connected to the
CrowPanel. Exact pack model, charge limit, actual capacity and connector polarity
have not been independently inspected. No charger settings were changed.

The [exact V1.0 schematic](https://github.com/Elecrow-RD/CrowPanel-Advanced-9inch-ESP32-P4-HMI-AI-Display-1024x600-IPS-Touch-Screen/blob/342a40719c98601fbd282f72e7fc2cd413133dd8/Eagle_SCH%26PCB/1.0/ESP32-P4%20Display%209.0%20inch%20V1.0.sch)
was parsed directly as EAGLE XML:

| Circuit | Evidence |
| --- | --- |
| Battery connector J3 | Pin1 VBAT positive; pin2 GND; mechanical pins3/4 GND. These are schematic pin numbers, not a physical orientation guide. Match actual +/− markings. |
| Charger | U2 TP4059-SOT23-6; PRO pin through R16 2k to GND |
| Battery measurement | VBAT → R164 39k → ADC_VBAT → R186 100k → GND; node reaches U14 STC8 P3.2/ADC10. Divider multiplier 1.39. |
| Charger status | POWER_CHRG and POWER_DONE reach STC8 P3.3 and P3.4, respectively |
| STC8 connection | P1.4/P1.5 through R188/R189 to shared P4 GPIO45 SDA / GPIO46 SCL |
| LEDs | POWER_R_LED/G_LED are driven by STC8; visible color alone needs qualified firmware meaning |

The [charger manufacturer's datasheet](https://www.toppwr.com/uploadfile/file/20240913/66e3a53c397cc.pdf)
specifies a single-cell lithium-ion charger with 4.2 V regulation. Its 2k table entry
is typically 430 mA; this is a component specification, **not measured pack current**.
The reported 3.7 V rating fits that nominal voltage class, but exact pack charge
limit and polarity must be checked before declaring compatibility. Capacity
alone does not establish an allowed charge rate. USB presence is not charging
proof. No battery-current or independent meter measurement was made.

## Read-only candidate probe

The official [5-inch STC8 header](https://github.com/Elecrow-RD/-CrowPanel-Advanced-5inch-ESP32-P4-HMI-AI-Display-800x480-IPS-Touch-Screen/blob/7fb059ac5129030a22e022d65f61383bda6867ac/example/V1.0/idf-code/Lesson07-Turn_on_the_screen/peripheral/bsp_stc8h1kxx/include/bsp_stc8h1kxx.h)
and matching implementation describe I2C address 0x2f and registers 0–11 containing
ADC millivolts, battery millivolts, level, state and LED state. This is a candidate
format from a related product; the 9-inch source examined does not qualify it.

On this V1.0 panel, address 0x2f responds. During 0.2.1 qualification, bytes interpreted using the related model yielded
ADC 2710 mV, battery 3766 mV, level 38, state 1 and LED 1. Raw records stay private. The voltage pair agrees with the V1.0
divider. State 1 means charging in the 5-inch protocol. These are **candidate
interpretations**, not a verified 38% state of charge or confirmation of current
entering this battery. The exact9-inch protocol and independent calibration
remain open.

The final 0.2.2 bench returned candidate 3775–3779 mV over its sampled interval,
with successful reads and unchanged unknown charge semantics. This trend does
not independently establish charging.

Final 0.2.2 keeps the screen at neutral **BAT --** and exposes `candidate_mv`
only in diagnostics, with `battery_mv=-1` and `charge=unknown`. A temporary 0.2.1
bench build displayed the candidate voltage; it is superseded. No empty-battery
icon or percentage is used. Polling reads only these registers every 5 seconds,
with a 20 ms total deadline (subject to RTOS scheduling); it never writes GPIO,
PWM, power or charger controls. Bad/partial/incoherent reads invalidate the
candidate. Input timing is refreshed after the probe.

## Remaining qualification

Confirm the pack model/4.2 V charge specification and physical connector polarity.
Compare battery voltage with an independent measurement and obtain the exact
V1.0 STC8 register/firmware definition before promoting candidate values.
Qualify charging status against the charger signals or measured current; do not
substitute a voltage trend, USB power or a related-board status code. Runtime,
state-of-charge calibration and power-cycle behavior remain untested.
