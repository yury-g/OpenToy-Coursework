# SPDX-License-Identifier: CC-BY-SA-3.0
"""Open FreeCAD's tessellated parts in Blender and render the README hookup."""
import json
from pathlib import Path
import sys

import bpy

ROOT = Path(__file__).resolve().parents[1]
MM = .001
geometry = json.loads(Path(sys.argv[sys.argv.index("--") + 1]).read_text())
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = "METRIC"
scene.unit_settings.length_unit = "MILLIMETERS"
scene.view_settings.view_transform = "Standard"


def collection(name):
    result = bpy.data.collections.new(name)
    scene.collection.children.link(result)
    return result


def linear(value):
    value /= 255
    return value / 12.92 if value < .04045 else ((value + .055) / 1.055) ** 2.4


def material(name, color, lit=False):
    rgb = tuple(linear(int(color[i:i + 2], 16)) for i in (1, 3, 5))
    result = bpy.data.materials.new(name)
    result.diffuse_color = (*rgb, 1)
    result.use_nodes = True
    nodes = result.node_tree.nodes
    if lit:
        shader = nodes.get("Principled BSDF")
        shader.inputs["Base Color"].default_value = (*rgb, 1)
        shader.inputs["Roughness"].default_value = .62
    else:
        shader = nodes.new("ShaderNodeEmission")
        shader.inputs["Color"].default_value = (*rgb, 1)
        result.node_tree.links.new(shader.outputs[0], nodes.get("Material Output").inputs["Surface"])
    return result


materials = {name: material(name, color, True) for name, color in geometry["palette"].items()}
groups = {}
for item in geometry["geometry"]:
    group = item["group"]
    if group not in groups:
        groups[group] = collection(group)
    mesh = bpy.data.meshes.new(item["name"])
    vertices = item["vertices_mm"]
    origin = tuple((min(v[i] for v in vertices) + max(v[i] for v in vertices)) / 2 for i in range(3))
    mesh.from_pydata([tuple((v[i] - origin[i]) * MM for i in range(3)) for v in vertices],
                     [], item["faces"])
    mesh.update()
    obj = bpy.data.objects.new(item["name"], mesh)
    obj.location = tuple(value * MM for value in origin)
    groups[group].objects.link(obj)
    obj.data.materials.append(materials[item["material"]])

presentation = collection("README artwork · hide for modeling")
ink = material("Text / espresso", "#321c13")
muted = material("Text / cocoa", "#664631")
white = material("Text / ivory", "#fff8eb")
orange = material("Accent / orange", "#d54a19")
leader = material("Leader lines", "#a45b3a")
background = material("Backdrop / warm ivory", "#eedbcb", True)


def label(value, x, y, size, mat=ink, align="LEFT", z=6.5):
    value = str(value)
    font = bpy.data.curves.new(value, "FONT")
    font.body = value
    font.align_x = align
    font.size = size * MM
    obj = bpy.data.objects.new(value, font)
    presentation.objects.link(obj)
    obj.location = (x * MM, y * MM, z * MM)
    font.materials.append(mat)


def line(name, points, mat, width=.13):
    data = bpy.data.curves.new(name, "CURVE")
    data.dimensions = "3D"
    data.bevel_depth = width * MM
    data.bevel_resolution = 3
    spline = data.splines.new("POLY")
    spline.points.add(len(points) - 1)
    for item, point in zip(spline.points, points):
        item.co = (*(v * MM for v in point), 1)
    obj = bpy.data.objects.new(name, data)
    presentation.objects.link(obj)
    data.materials.append(mat)


label("ORANGE FACE  /  HARDWARE", -8, 51, 3.1, orange)
label("STEMMA QT daisy chain", -8, 41.5, 7)
label("Use two 4-pin JST-SH cables  •  1.0 mm pitch", -7.5, 34.5, 3.1, muted)
label("Feather #5691", 0, 29.6, 3.3)
label("MPR121 #4830", 69, 29.6, 3.3)
label("LSM6DS3TR-C #4503", 143, 29.6, 3.3)
label("Component side · TFT underneath", 0, -16.3, 2.6, muted)
label("12 touch pads · I²C 0x5A", 69, -16.3, 2.6, muted)
label("Motion · I²C 0x6A", 143, -16.3, 2.6, muted)
label("QT", 23.5, 15.8, 1.8, white, "CENTER", 4.79)
label("USB-C", -2.8, 10.6, 1.65, ink, "RIGHT")
label("Other QT port", 156, -5.2, 2.35, muted)
label("can stay empty", 156, -8.6, 2.35, muted)
line("Unused socket callout", [(173, -2.1, 5.7), (173, 12.7, 5.7), (169, 12.7, 5.7)], leader)
label("Cable 1", 36.2, -5.3, 2.6, ink, "CENTER")
label("Cable 2", 131.8, -3.25, 2.6, ink, "CENTER")
line("Legend divider", [(-8, -20.9, 5.7), (181, -20.9, 5.7)], leader, .095)
for x, color, name in ((-7.5, "black", "GND"), (11, "red", "3.3 V"), (30, "blue", "SDA"), (48.5, "yellow", "SCL")):
    line(name, [(x, -26.5, 5.7), (x + 3.1, -26.5, 5.7)], materials[color], .48)
    label(name, x + 4.1, -27.25, 2.45, muted)
label("Both sensors share power and the same I²C bus.", 77, -27.25, 2.65, muted)

touch_top = [45.9105, 37.6555, 29.4005, 21.1455, 12.8905, 4.6355]
touch_bottom = [4.8895, 13.1445, 21.3995, 29.6545, 37.9095, 46.1645]
PCB_TOP = 1.69
for pad, x in enumerate(touch_top):
    label(pad, 69 + x, 18.8, 1.55, white, "CENTER", PCB_TOP)
for pad, x in enumerate(touch_bottom, 6):
    label(pad, 69 + x, 5.5, 1.55, white, "CENTER", PCB_TOP)

bpy.ops.mesh.primitive_plane_add(size=.6, location=(.085, .01, -.0031))
plane = bpy.context.object
plane.name = "Backdrop"
for parent in list(plane.users_collection):
    parent.objects.unlink(plane)
presentation.objects.link(plane)
plane.data.materials.append(background)

camera_data = bpy.data.cameras.new("Top view")
camera = bpy.data.objects.new("Top view", camera_data)
presentation.objects.link(camera)
camera.location = (.0865, .012, .290)
camera_data.type = "ORTHO"
camera_data.ortho_scale = .201
scene.camera = camera

for name, site, energy, radius in (
    ("Softbox", (.030, .02, .16), .19, .18),
    ("Fill", (.165, -.035, .10), .10, .16),
):
    light = bpy.data.lights.new(name, "AREA")
    light.energy, light.shape, light.size = energy, "DISK", radius
    obj = bpy.data.objects.new(name, light)
    obj.location = site
    presentation.objects.link(obj)

world = bpy.data.worlds.new("Studio")
world.use_nodes = True
world.node_tree.nodes.get("Background").inputs["Color"].default_value = (.65, .65, .65, 1)
world.node_tree.nodes.get("Background").inputs["Strength"].default_value = .55
scene.world = world
scene.render.engine = "CYCLES"
scene.cycles.samples = 20
scene.cycles.use_denoising = True
scene.render.resolution_x = 1608
scene.render.resolution_y = 736
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.render.filepath = "//../docs/images/orange-face-hookup.png"

for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type == "VIEW_3D":
            area.spaces.active.region_3d.view_perspective = "CAMERA"
            area.spaces.active.shading.color_type = "MATERIAL"

bpy.context.preferences.filepaths.save_version = 0
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT / "mechanical/orange-face-hookup.blend"), compress=True)
bpy.ops.render.render(write_still=True)
