# Orange Face hookup models

These are the source models used in the [README hookup drawing](../docs/images/orange-face-hookup.png).
They show the electronics face up, with the Feather's TFT on the opposite side.
All three boards and both cables are separate groups or collections.

| File | Use |
| --- | --- |
| [Blender scene](orange-face-hookup.blend) | Open directly in Blender 5.1 or later. The scene includes the materials, camera, lighting and labels used for the README image. Hide the “README artwork · hide for modeling” collection to work with just the boards and cables. |
| [FreeCAD document](orange-face-hookup.FCStd) | Open the source assembly in FreeCAD. Individual simplified solids are selectable inside each board and cable group. |
| [Full STEP assembly](orange-face-hookup.step) | Import the whole hookup into a CAD program with STEP support. |
| [Feather #5691 STEP](feather-5691.step) | Import just the controller with its underside TFT. |
| [MPR121 #4830 STEP](mpr121-4830.step) | Import just the touch board, including both sockets and all twelve electrode holes. |
| [LSM6DS3TR-C #4503 STEP](lsm6ds3tr-c-4503.step) | Import just the motion board and its two sockets. |

STEP geometry is in millimeters. The Blender scene uses metric units with a
millimeter display and stores the geometry at real-world scale. GitHub may
show a file information page for these formats. Use its **Download raw file**
button to save the file for your app.

## Dimensions and fit

| PCB model | Outline |
| --- | --- |
| Feather | 50.80 × 22.86 mm |
| MPR121 | 50.80 × 25.40 mm |
| LSM6DS3TR-C | 25.40 × 17.78 mm |

The outlines and major hole and connector positions come from Adafruit's PCB
layouts. The models simplify component shapes, heights and connector keyways.
The cable curves and spacing show one bench layout. For an enclosure, measure
your actual board revision and allow room for headers, inserted plugs, and cable
bends before printing.

Adafruit also provides a [manufacturer STEP, Fusion 360 and STL model of the
Feather](https://github.com/adafruit/Adafruit_CAD_Parts/tree/main/5691%20Feather%20ESP32%20S3%20Reverse%20TFT).

## Source and credits

Board geometry references are Adafruit Industries' [Reverse TFT Feather PCB](https://github.com/adafruit/Adafruit-ESP32-S3-Reverse-TFT-Feather-PCB),
[MPR121 Gator QT PCB](https://github.com/adafruit/Adafruit-MPR121-PCB) (the
`Adafruit MPR121Q QT Gator Breakout.brd` variant), and
[LSM6DS3TR-C PCB](https://github.com/adafruit/Adafruit-LSM6DS3TR-C-PCB).
The [STEMMA QT guide](https://learn.adafruit.com/introducing-adafruit-stemma-qt/technical-specs)
documents the 4-pin cable and wire colors.

Board references were designed by Limor Fried/Ladyada for Adafruit Industries.
Adafruit invests time and resources providing this open source design, please
support Adafruit and open-source hardware by purchasing products from [Adafruit](https://www.adafruit.com).
The derived CAD models, hookup drawing, and their generators are shared under
[CC BY-SA 3.0](CC-BY-SA-3.0.txt).

[FreeCAD generator](../tools/build_hookup_cad.py) and
[Blender scene generator](../tools/render_hookup_blender.py) reproduce the files.
From the chapter directory, run the CAD script with a Python interpreter that
can import FreeCAD and pass a temporary mesh JSON path. Then pass the same JSON
to Blender in background mode.

```sh
freecad-python tools/build_hookup_cad.py /tmp/orange-face-mesh.json
blender -b --python-exit-code 1 --python tools/render_hookup_blender.py -- /tmp/orange-face-mesh.json
```

Here `freecad-python` means the Python interpreter configured for your FreeCAD
installation. The files above are ready to open directly.
