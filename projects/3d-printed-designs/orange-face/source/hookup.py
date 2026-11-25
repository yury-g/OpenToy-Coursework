#!/usr/bin/env python3
# SPDX-License-Identifier: CC-BY-SA-3.0
"""Generate the millimeter-scale Orange Face hookup models in FreeCAD."""
import json
import math
from pathlib import Path
import sys

import FreeCAD as App
import Import
import Part

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "mechanical"
COLORS = {
    "feather": "#291f26", "touch": "#256783", "imu": "#225776",
    "gold": "#d9ad62", "chip": "#20252b", "metal": "#b6bec6",
    "white": "#ebe3cf", "screen": "#151115", "button": "#4f5258",
    "green": "#88dc79", "amber": "#ff803e", "black": "#25292e",
    "red": "#e4503e", "blue": "#327be0", "yellow": "#ffbd27",
}
V = App.Vector
PCB = 1.6
parts = {}


def add(group, name, shape, material):
    if not shape.isValid() or shape.Volume <= 0:
        raise ValueError(f"Invalid solid: {name}")
    parts.setdefault(group, []).append((name, shape, material))


def box(x, y, z, width, depth, height):
    return Part.makeBox(width, depth, height, V(x, y, z))


def center_box(x, y, z, width, depth, height):
    return box(x - width / 2, y - depth / 2, z, width, depth, height)


def cylinder(x, y, z, radius, height):
    return Part.makeCylinder(radius, height, V(x, y, z))


def annulus(x, y, r, outer, z=PCB):
    return cylinder(x, y, z, outer, .08).cut(cylinder(x, y, z - .1, r, .28))


def board(group, width, depth, holes):
    bare = box(0, 0, 0, width, depth, PCB)
    bare = bare.makeFillet(2.54, [e for e in bare.Edges if e.BoundBox.ZLength > 1.5])
    cutters = [cylinder(x, y, -.1, r, PCB + .2) for x, y, r, _ in holes]
    add(group, "PCB", bare.cut(Part.makeCompound(cutters)), group)
    for x, y, radius, rim in holes:
        if rim:
            add(group, "Plated ring", annulus(x, y, radius, rim), "gold")


def port(group, x, y, side, depth=4.25):
    # Keyway faces the cable. Keep the simplified shell and contacts distinct.
    shell = box(-depth / 2, -3.05, 0, depth, 6.1, 2.95)
    slot = box(.25, -2.11, .35, depth / 2 + .3, 4.22, 2.05)
    shape = shell.cut(slot)
    if side < 0:
        shape.rotate(V(), V(0, 0, 1), 180)
    shape.translate(V(x, y, PCB))
    add(group, "JST-SH 4-pin socket", shape, "white")
    for pin in (-1.5, -.5, .5, 1.5):
        add(group, "JST-SH contact", center_box(x + side * .7, y + pin, PCB + .37,
                                                 1.6, .22, .18), "gold")


def feather():
    group = "feather"
    holes = [(2.54, y, 1.25, 1.6) for y in (2.54, 20.32)]
    holes += [(48.26, y, 1.1, 0) for y in (1.8415, 20.955)]
    holes += [(6.35 + i * 2.54, 1.27, .45, .83) for i in range(16)]
    holes += [(16.51 + i * 2.54, 21.59, .45, .83) for i in range(12)]
    board(group, 50.8, 22.86, holes)
    port(group, 22.799, 11.43, -1, 2.85)
    add(group, "USB-C", center_box(2.7, 11.43, PCB, 7.4, 8.9, 3.1), "metal")
    add(group, "USB-C opening", center_box(-1.03, 11.43, PCB + .65, .14, 7.2, 1.6), "chip")
    add(group, "Battery socket", center_box(10.795, 17.653, PCB, 5.4, 6, 3.8), "white")
    add(group, "ESP32-S3 module", center_box(38.354, 11.43, PCB, 20.5, 15.4, 1.05), "chip")
    add(group, "Module shield", center_box(35.2, 11.43, 2.65, 12.1, 14.7, 1.05), "metal")
    add(group, "NeoPixel", center_box(29.083, 7.6835, 3.70, 1.5, 1.5, .45), "amber")
    add(group, "TFT on opposite face", box(11.291, 2.503, -2.35, 31, 17.6, 2.35), "chip")
    add(group, "TFT glass", box(12.1, 3.35, -2.47, 28.9, 15.9, .12), "screen")
    for name, x, y, z in (
        ("Reset component side", 10.795, 5.334, PCB),
        ("D0 BOOT", 7.62, 4.445, -1.6), ("D1", 7.62, 11.43, -1.6),
        ("D2", 7.62, 18.415, -1.6), ("Reset display side", 44.45, 11.43, -1.6),
    ):
        add(group, name, center_box(x, y, z, 3.8, 2.6, 1.6), "button")


def touch():
    group = "touch"
    # Electrodes 0–5 run right to left across the top and 6–11 left to right across the bottom.
    top = [45.9105, 37.6555, 29.4005, 21.1455, 12.8905, 4.6355]
    bottom = [4.8895, 13.1445, 21.3995, 29.6545, 37.9095, 46.1645]
    holes = [(x, 22.606, 1.6, 2.35) for x in top]
    holes += [(x, 2.794, 1.6, 2.35) for x in bottom]
    holes += [(x, y, 1.25, 1.6) for x in (8.89, 41.91) for y in (7.62, 17.78)]
    board(group, 50.8, 25.4, holes)
    port(group, 2.501, 12.7, -1)
    port(group, 48.299, 12.7, 1)
    add(group, "MPR121", center_box(25.4, 12.7, PCB, 3, 3, .8), "chip")
    add(group, "Power LED", center_box(1.524, 16.764, PCB, 1.65, .85, .7), "green")
    add(group, "Touch IRQ LED", center_box(33.02, 10.033, PCB, 1.65, .85, .7), "amber")
    passives(group, [(29.59, 14.61), (29.59, 16.13), (29.59, 13.08), (12.83, 12.7),
                     (16.51, 12.45), (15.75, 15.62), (29.97, 10.03), (38.86, 13.21)])


def passives(group, points):
    for x, y in points:
        add(group, "Passive", center_box(x, y, PCB, 1.6, .9, .55), "chip")


def imu():
    group = "imu"
    holes = [(x, 15.24, 1.25, 1.6) for x in (2.54, 22.86)]
    holes += [(2.54 + 2.54 * i, 2.54, .5, .88) for i in range(9)]
    holes += [(7.62 + 2.54 * i, 15.24, .5, .88) for i in range(5)]
    board(group, 25.4, 17.78, holes)
    port(group, 2.501, 8.89, -1)
    port(group, 22.899, 8.89, 1)
    add(group, "LSM6DS3TR-C", center_box(12.7, 8.89, PCB, 3, 2.5, .85), "chip")
    add(group, "Power LED", center_box(3.556, 12.573, PCB, 1.65, .85, .7), "green")
    passives(group, [(5.02, 4.89), (8.13, 7.18), (12.52, 5.7),
                     (10.86, 5.63), (17.98, 10.77), (14.73, 11.81)])


def cubic(a, b, c, d, t):
    u = 1 - t
    return a * (u ** 3) + b * (3 * t * u ** 2) + c * (3 * t ** 2 * u) + d * (t ** 3)


def cable(group, a, a_side, b, b_side, segments, twist=False):
    for label, point, side in (("first", a, a_side), ("second", b, b_side)):
        p = V(*point)
        add(group, f"JST-SH plug {label}", center_box(
            p.x - side * 1.4, p.y, p.z - 1.0, 3.1, 4.65, 2.1), "white")

    centers = []
    for i, segment in enumerate(segments):
        control = [V(*point) for point in segment]
        centers.extend(cubic(*control, step / 16) for step in range(int(i > 0), 17))

    for offset, color in zip((1.5, .5, -.5, -1.5), ("black", "red", "blue", "yellow")):
        filament = []
        for i, point in enumerate(centers):
            tangent = centers[min(i + 1, len(centers) - 1)] - centers[max(i - 1, 0)]
            normal = V(-tangent.y, tangent.x, 0)
            normal.normalize()
            if twist:
                t = max(0, min(1, (i / (len(centers) - 1) - .25) * 2))
                angle = math.pi * t * t * (3 - 2 * t)
                # This cable starts at the right-edge socket, whose pin 1 is at -Y.
                normal = normal * -math.cos(angle) + V(0, 0, math.sin(angle))
            filament.append(point + normal * offset)
        sections = []
        for i, point in enumerate(filament):
            tangent = filament[min(i + 1, len(filament) - 1)] - filament[max(i - 1, 0)]
            sections.append(Part.Wire([Part.makeCircle(.29, point, tangent)]))
        add(group, color.title() + " conductor", Part.makeLoft(sections, True, True), color)


def objects(doc, group_name, rows, offset):
    collection = doc.addObject("App::DocumentObjectGroup", group_name.replace(" ", ""))
    collection.Label = group_name
    for name, shape, mat in rows:
        obj = doc.addObject("Part::Feature", "Part")
        obj.Label = name
        obj.Shape = shape.copy()
        obj.Placement.Base = offset
        collection.addObject(obj)
    return collection.Group


def export_step(features, filename):
    path = OUT / filename
    Import.export(features, str(path))
    lines = path.read_bytes().splitlines()
    path.write_bytes(b"\n".join(line.rstrip(b" 	") for line in lines) + b"\n")


def main():
    OUT.mkdir(exist_ok=True)
    feather()
    touch()
    imu()
    # Openings are based on Adafruit's connector footprints. Routes show slack at a bench.
    offsets = {"feather": V(0, 1.27, 0), "touch": V(69, 0, 0), "imu": V(143, 3.81, 0)}
    a, b = (18.5, 12.7, 3.15), (66.35, 12.7, 3.15)
    cable("cable1", a, -1, b, -1, [
        (a, (14, 12.7, 3.15), (14, -7, 4), (23, -9, 3.9)),
        ((23, -9, 3.9), (33, -11.22, 3.8), (50, -9, 3.9), (53, 3, 3.9)),
        ((53, 3, 3.9), (55, 11, 3.9), (61, 12.7, 3.15), b),
    ])
    a, b = (122.45, 12.7, 3.15), (140.35, 12.7, 3.15)
    cable("cable2", a, 1, b, -1, [
        (a, (127, 12.7, 3.15), (126, 4, 5.5), (131.5, 4, 5.5)),
        ((131.5, 4, 5.5), (137, 4, 5.5), (135.8, 12.7, 3.15), b),
    ], twist=True)

    names = {"feather": ("Feather 5691", "feather-5691"),
             "touch": ("MPR121 4830", "mpr121-4830"), "imu": ("LSM6DS3TR-C 4503", "lsm6ds3tr-c-4503"),
             "cable1": ("Cable 1 Feather to touch", ""), "cable2": ("Cable 2 touch to IMU", "")}
    assembly = App.newDocument("OrangeFaceHookup")
    all_objects = []
    geometry = []
    for group, rows in parts.items():
        title, filename = names[group]
        offset = offsets.get(group, V())
        all_objects.extend(objects(assembly, title, rows, offset))
        if filename:
            doc = App.newDocument("Board")
            features = objects(doc, title, rows, V())
            doc.recompute()
            export_step(features, f"{filename}.step")
            App.closeDocument(doc.Name)

        for name, original, material in rows:
            shape = original.copy()
            shape.translate(offset)
            vertices, faces = shape.tessellate(.08)
            geometry.append({"group": title, "name": name, "material": material,
                             "vertices_mm": [tuple(v) for v in vertices], "faces": faces})
    assembly.recompute()
    assembly.saveAs(str(OUT / "orange-face-hookup.FCStd"))
    export_step(all_objects, "orange-face-hookup.step")
    Path(sys.argv[1]).write_text(json.dumps({"palette": COLORS, "geometry": geometry}))


if __name__ == "__main__":
    main()
