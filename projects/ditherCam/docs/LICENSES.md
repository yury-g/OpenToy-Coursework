# ditherCam Third-Party Notices & Attribution

[← ditherCam overview](../README.md) · [Repository license overview](../../../licenses/README.md)

OpenToy's original `ditherCam` firmware source, host tests, and documentation are licensed under the [MIT License](../../../LICENSE). Third-party components retain their original copyright and license notices:

- **TJpgDec (Tiny JPEG Decompressor) / LovyanGFX modifications:** Preserved in [`shared/FlipCamSoftJpgd.h`](../shared/FlipCamSoftJpgd.h), [`shared/FlipCamSoftJpgd.c`](../shared/FlipCamSoftJpgd.c), [`firmware/RlcdCamera/src/SoftJpgd.c`](../firmware/RlcdCamera/src/SoftJpgd.c), and [`firmware/SenseCamera/src/SoftJpgd.c`](../firmware/SenseCamera/src/SoftJpgd.c) (`(C) ChaN, 2019`, modified for LGFX by `lovyan03, 2020`).
- **Three.js (`0.185.1`), `OrbitControls`, and `STLLoader`:** Bundled in the offline 3D viewers under the MIT License; see [`projects/3d-printed-designs/ditherCam/source/THREE-LICENSE.txt`](../../3d-printed-designs/ditherCam/source/THREE-LICENSE.txt), [`source/THIRD-PARTY.md`](../../3d-printed-designs/ditherCam/source/THIRD-PARTY.md), and [`assembly/source/THIRD-PARTY.md`](../../3d-printed-designs/ditherCam/assembly/source/THIRD-PARTY.md).
- **Hardware reference geometry & schematics:** Derived from manufacturer resources for the [Waveshare ESP32-S3-RLCD-4.2](../../../hardware/waveshare-rlcd/README.md) and [Seeed Studio XIAO ESP32-S3 / Expansion Base](../../../hardware/xiao/README.md).
