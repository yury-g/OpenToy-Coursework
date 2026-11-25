# Mechanical references

- Waveshare V2 manufacturer STEP and drawing, dated 2026-10-10: https://docs.waveshare.com/ESP32-S3-RLCD-4.2/Resources-And-Documents
- Manufacturer overview and V1/V2 notes: https://docs.waveshare.com/ESP32-S3-RLCD-4.2
- Seeed Expansion Base vendor Eagle outline: `Seeeduino_XIAO_Expansion_board_v1.0_200824.brd`; Sense vendor STEP (`hardware/xiao/source/xiao-esp32s3-sense.step`) and registered stack geometry (`cad/hw_xiao_stack.stl`).
- Canonical CAD source and parameters: [`cad/Camera.scad`](cad/Camera.scad) and [`cad/parameters.json`](cad/parameters.json).
- Independent geometry, insertion, OLED visibility, and balance verification records: [`verification/`](verification/).

The camera/battery/header/antenna geometry is not a physical measurement of this particular assembly. The model uses approximate envelopes and the fit assumptions in [README.md](README.md).

- RLCD button refdes and nets: https://files.waveshare.com/wiki/ESP32-S3-RLCD-4.2/ESP32-S3-RLCD-4.2-schematic.pdf (KEY4/KEY/GPIO18 → Snap; KEY1/BOOT/GPIO0 → Browse; KEY3/PWR separate). These are assigned application roles, not manufacturer functions or a firmware change.
