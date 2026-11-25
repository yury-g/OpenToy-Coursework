# Purple Face / CYD extension

Owner attached a CYD ESP32-2432S028 with micro-USB computer connection and USB-C power input; confirmed USB only, no battery/gauge module. Identify classic ESP32 and flash capacity, retain and hardware-digest-verify original flash, inspect partition table before app-only writing. Do not fabricate battery values or probe GPIO ADC inputs as a cell.

Add Purple as a stable new identity, USB-powered companion/viewer. Show a smiling Purple face with USB power / no gauge; remote batteries remain selectable. Tab5 includes Purple's live presence and explicitly shows no battery measurement. Four existing measured/estimated reporters remain unchanged; retired Green remains excluded. Adaptive layout must accommodate five live nodes.

Use the established CYD2USB ST7789 pinout (LCD14/13/12/15/DC2/BL21, XPT2046 touch25/32/39/33/36), verify actual panel/colors and touch. Classic UART needs bounded nonblocking ordinary debug output; no native USB-only API assumptions. Preserve aspect ratio at native240×320, microphone/speaker unused and amp output quiet.

Also make full Tab5 screenshot writes exact across partial serial writes, with a global deadline. Existing live radio/reflow assertions passed; final capture failed due a short USB write, not battery or radio state.
