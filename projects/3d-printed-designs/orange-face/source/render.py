# SPDX-License-Identifier: CC-BY-SA-3.0
"""Assemble and render the Orange Face paperback with native Blender tools."""
from pathlib import Path
import json
import math
import sys

import bpy
from mathutils import Matrix, Vector

root = Path(__file__).resolve().parent
out = root.parent
data = json.loads((root / "assembly.json").read_text())
bpy.ops.wm.read_factory_settings(use_empty=True)
MM = .001

def xyz(values):
    return Vector(values) * MM

def tint(value):
    rgb = [int(value[i:i+2],16)/255 for i in (1,3,5)]
    return tuple(c/12.92 if c < .04045 else ((c+.055)/1.055)**2.4 for c in rgb)

def coat(name, value, metal=0, rough=.47, emission=0):
    mat = bpy.data.materials.new(name)
    rgba = (*tint(value), 1)
    mat.diffuse_color = rgba
    mat.use_nodes = True
    shader = mat.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = rgba
    shader.inputs["Metallic"].default_value = metal
    shader.inputs["Roughness"].default_value = rough
    shader.inputs["Emission Color"].default_value = rgba
    shader.inputs["Emission Strength"].default_value = emission
    return mat

materials = {k: coat(k, v, .42 if k in ("metal","gold") else 0) for k,v in data["palette"].items()}
materials["touch"] = coat("MPR121 / charcoal soldermask", "#272e31")
materials["ink"] = coat("Espresso ink", "#351e19")
materials["cream"] = coat("Warm foil", "#fff0cc")
materials["foil"] = coat("Copper electrodes / illustration", "#c48253", .68)
materials["ledge"] = coat("Page grooves", "#bb916e")
materials["display"] = coat("Display orange / illustrative", "#ff8648", 0, .34, .28)
materials["floor"] = coat("Warm gray studio", "#c8b9a8", rough=.65)
materials["callout"] = coat("Layout label", "#30211a")
materials["callout2"] = coat("Layout subtitle", "#75452c")

def collection(scene, name):
    group = bpy.data.collections.new(name)
    scene.collection.children.link(group)
    return group

def mesh_item(item, group, shift=(0,0,0)):
    mesh = bpy.data.meshes.new(item["name"])
    mesh.from_pydata([xyz(p) for p in item["vertices"]], [], item["triangles"])
    mesh.update()
    obj = bpy.data.objects.new(item["name"], mesh)
    group.objects.link(obj)
    obj.location = xyz(shift)
    mesh.materials.append(materials[item["material"]])
    return obj

def relocate(obj, group):
    for source in list(obj.users_collection):
        source.objects.unlink(obj)
    group.objects.link(obj)

def cube(name, center, size, mat, group, bevel=0):
    bpy.ops.mesh.primitive_cube_add(size=1, location=xyz(center))
    ob=bpy.context.object
    ob.name=name
    relocate(ob, group)
    ob.dimensions=xyz(size)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    ob.data.materials.append(materials[mat])
    if bevel:
        bd=ob.modifiers.new("Rounded edges", "BEVEL")
        bd.width=bevel*MM
        bd.segments=4
        ob.modifiers.new("Face normals", "WEIGHTED_NORMAL")
    return ob

def disc(name, center, radius, height, mat, group, vertices=72):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius*MM,
                                       depth=height*MM, location=xyz(center))
    ob=bpy.context.object
    ob.name=name
    relocate(ob, group)
    ob.data.materials.append(materials[mat])
    return ob

def line(name, points, radius, mat, group, smooth=False):
    data = bpy.data.curves.new(name, "CURVE")
    data.dimensions="3D"
    data.bevel_depth=radius*MM
    data.bevel_resolution=3
    if smooth:
        spline=data.splines.new("BEZIER")
        spline.bezier_points.add(len(points)-1)
        for point, position in zip(spline.bezier_points,points):
            point.co=xyz(position)
            point.handle_left_type=point.handle_right_type="AUTO"
    else:
        spline=data.splines.new("POLY")
        spline.points.add(len(points)-1)
        for point, position in zip(spline.points, points):
            point.co=(*xyz(position),1)
    ob=bpy.data.objects.new(name,data)
    group.objects.link(ob)
    data.materials.append(materials[mat])
    return ob

font_dir=root / "fonts"
serif=bpy.data.fonts.load(str(font_dir/"LibreBaskerville.ttf"))
sans=bpy.data.fonts.load(str(font_dir/"BarlowCondensed-SemiBold.ttf"))
regular=bpy.data.fonts.load(str(font_dir/"BarlowCondensed-Regular.ttf"))
for font in (serif,sans,regular):
    font.pack()

def text(group, body, position, size, font, mat, center=False, angle=0):
    data=bpy.data.curves.new(body,"FONT")
    data.body=body
    data.font=font
    data.size=size*MM
    data.align_x="CENTER" if center else "LEFT"
    ob=bpy.data.objects.new(body,data)
    ob.location=xyz(position)
    ob.rotation_euler.z=math.radians(angle)
    group.objects.link(ob)
    data.materials.append(materials[mat])
    return ob

def camera(scene, group, where, look, span):
    data=bpy.data.cameras.new(scene.name)
    ob=bpy.data.objects.new(scene.name,data)
    group.objects.link(ob)
    ob.location=xyz(where)
    ob.rotation_euler=(xyz(look)-ob.location).to_track_quat("-Z","Y").to_euler()
    data.type="ORTHO"
    data.clip_start=.001
    data.ortho_scale=span*MM
    scene.camera=ob

def softbox(group, name, where, look, energy, radius):
    data=bpy.data.lights.new(name, "AREA")
    data.energy = energy
    data.shape="DISK"
    data.size=radius*MM
    ob=bpy.data.objects.new(name,data)
    group.objects.link(ob)
    ob.location=xyz(where)
    ob.rotation_euler=(xyz(look)-ob.location).to_track_quat("-Z","Y").to_euler()

def setup(scene, where, look, span, width, height):
    bpy.context.window.scene=scene
    scene.unit_settings.system="METRIC"
    scene.unit_settings.length_unit="MILLIMETERS"
    scene.view_settings.view_transform="AgX"
    scene.render.engine="CYCLES"
    scene.cycles.samples=36
    scene.cycles.use_denoising=True
    scene.render.resolution_x=width
    scene.render.resolution_y=height
    scene.render.resolution_percentage=100
    scene.render.image_settings.file_format="PNG"
    group=collection(scene,"Studio and camera")
    cube("Studio floor", (55,88,-1.5),(1800,1600,2),"floor",group)
    camera(scene,group,where,look,span)
    softbox(group,"Large window", (-80,160,300),look,.74,240)
    softbox(group,"Bounce", (280,-40,190),look,.21,185)
    softbox(group,"Top rim", (150,290,240),look,.51,140)
    world=bpy.data.worlds.new(scene.name)
    scene.world=world
    world.use_nodes=True
    world.node_tree.nodes.get("Background").inputs[0].default_value=(.63,.58,.53,1)
    world.node_tree.nodes.get("Background").inputs[1].default_value=.28
    return scene

def shell(scene, cover_shift=None, tray_shift=(0,0,0), add_cover=True):
    group=collection(scene,"CAD shell / millimeter-scale")
    tray = mesh_item(data["shell"][1], group, tray_shift)
    if add_cover:
        lid=mesh_item(data["shell"][0],group,cover_shift or (0,0,0))
    return group

def hardware(scene):
    groups={k:collection(scene,name) for k,name in (
        ("feather","Feather / display up"),("touch","MPR121 / twelve electrode pads"),
        ("imu","LSM6DS3TR-C / rigidly mounted"),("cable1","QT 1 / Feather to MPR121"),
        ("cable2","QT 2 / MPR121 to IMU"))}
    for item in data["hardware"]:
        mesh_item(item,groups[item["group"]])
    return groups

def cover_graphics(group, offset=(0,0,0), show_display=True):
    dx,dy,dz=offset
    def pos(x,y,z=21.22):
        return x+dx,y+dy,z+dz
    text(group,"OPEN TOY",pos(16,157),5.6,sans,"cream")
    text(group,"Orange",pos(16,104),18.2,serif,"cream")
    text(group,"Face",pos(16,84),19,serif,"cream")
    for number, x in enumerate((45.9105,37.6555,29.4005,21.1455,12.8905,4.6355)):
        text(group, str(number), pos(29.6+x,46.1), 2.5, sans, "cream", True)
    for number, x in enumerate((4.8895,13.1445,21.3995,29.6545,37.9095,46.1645), 6):
        text(group, str(number), pos(29.6+x,77.1), 2.5, sans, "cream", True)
    line("Cover rule", [pos(16,40),pos(94,40)], .19,"cream",group)
    text(group,"TOUCH THE GOLD PADS",pos(16,30),4.1,sans,"cream")
    text(group,"Touch. Turn. Shake.",pos(16,21),4.3,regular,"cream")
    for x,y,label in [(13,140.55,"D0"),(13,133.57,"D1"),(13,126.58,"D2"),(50.8,133.57,"RST")]:
        text(group,label,pos(x+(5.8 if label=="RST" else -7.1),y-1),2.4,sans,"cream",True)
    if show_display:
        cube("TFT artwork plane",pos(31.55,133.57,20.005),(28.75,15.76,.06),"display",group,.022)
        for x in (25.6,37.3):
            disc("Eye / TFT artwork",pos(x,135.5,20.07),1.45,.07,"ink",group)
        line("Smile / TFT artwork", [pos(x,y,20.1) for x,y in [(28.3,131.9),(31.6,130.4),(34.8,131.9)]],
             .39,"ink",group,True)


front_transform=Matrix.Translation(xyz((0,176,16))) @ Matrix.Rotation(math.pi,4,"X")
def body(scene, loaded=False, matrix=None):
    group=collection(scene,"One integral printed faceplate")
    item=data["installed_shell" if loaded else "shell"][0]
    ob=mesh_item(item,group)
    if matrix is not None: ob.matrix_world=matrix
    return group

def boards(scene,matrix=None):
    group=collection(scene,"Installed boards and routed QT leads")
    for item in data["hardware"]:
        ob=mesh_item(item,group)
        if matrix is not None: ob.matrix_world=matrix
    return group

front=setup(bpy.context.scene,(-155,-128,320),(53,87,7),225,1400,1200)
front.name="01 | Front / flush touch"
body(front,True,front_transform);boards(front,front_transform)
art=collection(front,"Cover artwork / render only")
cover_graphics(art,(0,0,-5.2))
rear=setup(bpy.data.scenes.new("02 | Rear / installed"),(180,285,350),(55,88,6),222,1400,1200)
body(rear,True);boards(rear)
art=collection(rear,"Rear labels / render only")
text(art,"ONE PIECE",(92,153,3.22),6,sans,"cream",angle=180)
text(art,"PRESS IN. FLEX TO RELEASE.",(92,143,3.22),3.8,regular,"cream",angle=180)
empty=setup(bpy.data.scenes.new("03 | Empty / print orientation"),(180,285,350),(55,88,6),222,1400,1200)
body(empty)
art=collection(empty,"Explanatory labels / render only")
text(art,"ONE PRINT / TEN SPRINGS",(96,153,3.22),5,sans,"cream",angle=180)
text(art,"Flat face on the build plate",(96,143,3.22),3.8,regular,"cream",angle=180)
layout=setup(bpy.data.scenes.new("04 | Parts outside"),(75,96,460),(75,96,0),300,1560,1200)
body(layout,False,Matrix.Translation(xyz((-35,0,0))))
group=collection(layout,"Detached electronics / same scale")
for item in data["hardware"]:
    key=item["group"]
    offsets={"feather":(90,105,0),"touch":(65,-35,1),"imu":(25,-13,0)}
    if key in offsets:mesh_item(item,group,offsets[key])
art=collection(layout,"Parts labels / render only")
for label,x,y in [("DISPLAY / PROCESSOR",94,162),("FLUSH TOUCH",94,101),("MOTION SENSOR",94,59)]:
    text(art,label,(x,y,0),4,sans,"callout")
text(art,"ONE PRINTED PART",(-27,185,0),5,sans,"callout")
# Detached QT cables kept together in a separate, bounded area.
for y in (5,13):
    for offset,color in zip((-.9,-.3,.3,.9),("black","red","blue","yellow")):
        line("Detached QT wire",[(96,y+offset,1),(110,y+5+offset,1),(128,y+5+offset,1),(144,y+offset,1)],.22,color,group,True)
    for x in (94,146):cube("QT plug",(x,y,1),(4.7,5.4,2.1),"white",group,.3)
text(art,"QT LEADS",(94,22,0),4,sans,"callout")
scenes=[(front,"01-Front"),(rear,"02-Electronics-Held-in-Faceplate"),(empty,"03-One-Printed-Part"),(layout,"04-Parts-Outside")]
for scene,_ in scenes:
    scene.cycles.samples=28
    scene.render.threads_mode="AUTO"
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type=="VIEW_3D":
                area.spaces.active.region_3d.view_perspective="CAMERA"
                area.spaces.active.clip_start=.001
bpy.context.window.scene=rear
bpy.context.preferences.filepaths.save_version=0
bpy.ops.wm.save_as_mainfile(filepath=str(out/"blender/Orange-Face-One-Piece.blend"))
for scene,name in scenes:
    bpy.context.window.scene=scene
    scene.render.filepath=str(out/"images"/(name+".png"))
    bpy.ops.render.render(write_still=True)
    print("RENDER_OK",name,flush=True)
