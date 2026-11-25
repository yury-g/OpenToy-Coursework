# SPDX-License-Identifier: MIT
"""Render ditherCam hero still and animated CAD sequences with Blender CLI.

Uses exact CAD STL meshes from ../cad/ and exact mechanical transforms from
Camera.scad / assembly/source/assembly.js. Computes orthographic camera framing
across all keyframes of each sequence so the full assembly and moving boards
retain generous margins without clipping.
"""
from __future__ import annotations

import math
import struct
from pathlib import Path

import bpy
from mathutils import Matrix, Vector

BLENDER_DIR = Path(__file__).resolve().parent
DESIGN_DIR = BLENDER_DIR.parent
CAD_DIR = DESIGN_DIR / "cad"
REPO_ROOT = DESIGN_DIR.parents[2]
UI_DIR = REPO_ROOT / "projects" / "ditherCam" / "docs" / "images"
CACHE_DIR = BLENDER_DIR / ".frame-cache"
OUT_IMAGES = DESIGN_DIR / "images"

MM = 0.001
AXIS_Z_PRINT = 22.0
Y_SOLE_PRINT = 104.5
RLCD_PIVOT = Vector((0.0, 28.1, 19.3)) * MM


def xyz(v: tuple[float, float, float]) -> Vector:
    return Vector(v) * MM


def hex_to_linear(hex_str: str) -> tuple[float, float, float]:
    rgb = [int(hex_str[i : i + 2], 16) / 255.0 for i in (1, 3, 5)]
    return tuple(c / 12.92 if c < 0.04045 else ((c + 0.055) / 1.055) ** 2.4 for c in rgb)


def make_material(
    name: str,
    hex_color: str,
    roughness: float = 0.45,
    metallic: float = 0.0,
    alpha: float = 1.0,
    emission_strength: float = 0.0,
) -> bpy.types.Material:
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    rgba = (*hex_to_linear(hex_color), 1.0)
    mat.diffuse_color = (*rgba[:3], alpha)
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = rgba
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Alpha"].default_value = alpha
    if "Emission Color" in bsdf.inputs:
        bsdf.inputs["Emission Color"].default_value = rgba
        bsdf.inputs["Emission Strength"].default_value = emission_strength
    if alpha < 0.99:
        if hasattr(mat, "surface_render_method"):
            mat.surface_render_method = "BLENDED"
        if hasattr(mat, "blend_method"):
            mat.blend_method = "BLEND"
    return mat


def make_textured_screen_material(
    name: str,
    image_path: Path,
    tint_hex: str,
    emission_strength: float,
    roughness: float = 0.50,
) -> bpy.types.Material:
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes.get("Principled BSDF")
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Metallic"].default_value = 0.0

    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.interpolation = "Closest"
    img = bpy.data.images.load(str(image_path), check_existing=True)
    img.colorspace_settings.name = "sRGB"
    tex.image = img

    mix = nt.nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    mix.blend_type = "MULTIPLY"
    mix.inputs["Factor"].default_value = 1.0
    mix.inputs[7].default_value = (*hex_to_linear(tint_hex), 1.0)

    nt.links.new(tex.outputs["Color"], mix.inputs[6])
    nt.links.new(mix.outputs[2], bsdf.inputs["Base Color"])
    if "Emission Color" in bsdf.inputs:
        nt.links.new(mix.outputs[2], bsdf.inputs["Emission Color"])
        bsdf.inputs["Emission Strength"].default_value = emission_strength
    return mat


def parse_stl_geometry(stl_path: Path) -> tuple[list[tuple[float, float, float]], list[tuple[int, int, int]]]:
    """Parse binary or ASCII STL in millimeter coordinates -> (verts_m, faces)."""
    raw = stl_path.read_bytes()
    verts: list[tuple[float, float, float]] = []
    faces: list[tuple[int, int, int]] = []
    vert_map: dict[tuple[float, float, float], int] = {}

    def add_vert(pt: tuple[float, float, float]) -> int:
        idx = vert_map.get(pt)
        if idx is None:
            idx = len(verts)
            vert_map[pt] = idx
            verts.append((pt[0] * MM, pt[1] * MM, pt[2] * MM))
        return idx

    is_binary = False
    if len(raw) >= 84 and not raw.lstrip().startswith(b"solid"):
        tri_count = struct.unpack("<I", raw[80:84])[0]
        if len(raw) == 84 + tri_count * 50:
            is_binary = True

    if is_binary:
        tri_count = struct.unpack("<I", raw[80:84])[0]
        off = 84
        for _ in range(tri_count):
            vals = struct.unpack_from("<12fH", raw, off)
            off += 50
            i0 = add_vert((vals[3], vals[4], vals[5]))
            i1 = add_vert((vals[6], vals[7], vals[8]))
            i2 = add_vert((vals[9], vals[10], vals[11]))
            if i0 != i1 and i1 != i2 and i2 != i0:
                faces.append((i0, i1, i2))
    else:
        tri: list[int] = []
        for line in raw.decode("utf-8", errors="replace").splitlines():
            parts = line.strip().split()
            if len(parts) >= 4 and parts[0] == "vertex":
                tri.append(add_vert((float(parts[1]), float(parts[2]), float(parts[3]))))
                if len(tri) == 3:
                    if tri[0] != tri[1] and tri[1] != tri[2] and tri[2] != tri[0]:
                        faces.append((tri[0], tri[1], tri[2]))
                    tri = []

    return verts, faces


def build_meshes(geom_cache: dict[str, tuple[list, list]]) -> dict[str, bpy.types.Mesh]:
    out: dict[str, bpy.types.Mesh] = {}
    for name, (verts, faces) in geom_cache.items():
        mesh = bpy.data.meshes.new(name)
        mesh.from_pydata(verts, [], faces)
        mesh.update()
        out[name] = mesh
    return out


def add_mesh_object(name: str, mesh: bpy.types.Mesh, mat: bpy.types.Material, parent: bpy.types.Object) -> bpy.types.Object:
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj.parent = parent
    if len(obj.data.materials) == 0:
        obj.data.materials.append(mat)
    else:
        obj.data.materials[0] = mat
    return obj


def make_textured_quad(
    name: str,
    corners_mm: list[tuple[float, float, float]],
    parent: bpy.types.Object,
    mat: bpy.types.Material,
) -> bpy.types.Object:
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([xyz(c) for c in corners_mm], [], [(0, 1, 2, 3)])
    uv_layer = mesh.uv_layers.new(name="UVMap")
    for loop_idx, uv in enumerate([(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0)]):
        uv_layer.data[loop_idx].uv = uv
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj.parent = parent
    obj.data.materials.append(mat)
    return obj


def add_marker_sphere(name: str, pos_mm: tuple[float, float, float], radius_mm: float, mat: bpy.types.Material, parent: bpy.types.Object) -> bpy.types.Object:
    bpy.ops.mesh.primitive_uv_sphere_add(segments=20, ring_count=12, radius=radius_mm * MM, location=xyz(pos_mm))
    obj = bpy.context.active_object
    obj.name = name
    obj.parent = parent
    obj.data.materials.append(mat)
    return obj


def add_direction_arrow(
    name: str,
    start_mm: tuple[float, float, float],
    end_mm: tuple[float, float, float],
    mat: bpy.types.Material,
    parent: bpy.types.Object,
) -> list[bpy.types.Object]:
    p0 = xyz(start_mm)
    p1 = xyz(end_mm)
    vec = p1 - p0
    length = vec.length
    head_len = min(7.0 * MM, length * 0.30)
    shaft_len = max(1.0 * MM, length - head_len)

    bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=1.25 * MM, depth=shaft_len)
    shaft = bpy.context.active_object
    shaft.name = f"{name}_shaft"
    quat = vec.normalized().to_track_quat("Z", "Y")
    shaft.rotation_mode = "QUATERNION"
    shaft.rotation_quaternion = quat
    shaft.location = p0 + vec.normalized() * (shaft_len * 0.5)
    shaft.parent = parent
    shaft.data.materials.append(mat)

    bpy.ops.mesh.primitive_cone_add(vertices=20, radius1=3.1 * MM, radius2=0.0, depth=head_len)
    cone = bpy.context.active_object
    cone.name = f"{name}_head"
    cone.rotation_mode = "QUATERNION"
    cone.rotation_quaternion = quat
    cone.location = p0 + vec.normalized() * (shaft_len + head_len * 0.5)
    cone.parent = parent
    cone.data.materials.append(mat)
    return [shaft, cone]


def setup_studio(width: int, height: int, samples: int = 24) -> tuple[bpy.types.Object, bpy.types.Object]:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.length_unit = "MILLIMETERS"
    scene.render.engine = "BLENDER_EEVEE"
    scene.eevee.taa_render_samples = samples
    scene.render.resolution_x = width
    scene.render.resolution_y = height
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.look = "AgX - Base Contrast"

    world = bpy.data.worlds.new("StudioWorld")
    scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    bg.inputs[0].default_value = (*hex_to_linear("#bab3a7"), 1.0)
    bg.inputs[1].default_value = 0.35

    bpy.ops.mesh.primitive_plane_add(size=3.0, location=(0.0, 0.0, -0.0005))
    floor = bpy.context.active_object
    floor.name = "StudioFloor"
    floor.data.materials.append(make_material("Floor", "#b2aba0", roughness=0.82))

    def add_area(name: str, where: tuple[float, float, float], look: tuple[float, float, float], energy: float, size_mm: float):
        ldata = bpy.data.lights.new(name, "AREA")
        ldata.energy = energy
        ldata.shape = "DISK"
        ldata.size = size_mm * MM
        lobj = bpy.data.objects.new(name, ldata)
        scene.collection.objects.link(lobj)
        lobj.location = xyz(where)
        lobj.rotation_euler = (xyz(look) - lobj.location).to_track_quat("-Z", "Y").to_euler()

    add_area("KeyLight", (190, -250, 310), (0, 20, 50), 5.2, 280)
    add_area("FillLight", (-240, -150, 210), (0, 20, 50), 2.3, 340)
    add_area("RimLight", (60, 270, 270), (0, 20, 50), 3.4, 240)

    cam_data = bpy.data.cameras.new("MainCamera")
    cam_data.type = "ORTHO"
    cam_data.clip_start = 0.005
    cam_data.clip_end = 5.0
    cam_obj = bpy.data.objects.new("MainCamera", cam_data)
    scene.collection.objects.link(cam_obj)
    scene.camera = cam_obj

    root = bpy.data.objects.new("root_print_space", None)
    scene.collection.objects.link(root)
    return cam_obj, root


def fit_ortho_camera(
    cam_obj: bpy.types.Object,
    tracked_objs: list[bpy.types.Object],
    view_dir_from_target: tuple[float, float, float],
    pose_callbacks: list,
    width: int,
    height: int,
    margin: float = 0.09,
) -> None:
    """Aim ortho camera and choose ortho_scale covering all poses with guaranteed margins."""
    direction = Vector(view_dir_from_target).normalized()
    rot_quat = (-direction).to_track_quat("-Z", "Y")
    cam_rot_inv = rot_quat.to_matrix().to_4x4().inverted()

    xs: list[float] = []
    ys: list[float] = []
    for cb in pose_callbacks:
        cb()
        bpy.context.view_layer.update()
        for obj in tracked_objs:
            if obj.hide_render or obj.type != "MESH":
                continue
            mw = obj.matrix_world
            for corner in obj.bound_box:
                p_cam = cam_rot_inv @ (mw @ Vector(corner))
                xs.append(p_cam.x)
                ys.append(p_cam.y)

    cx = 0.5 * (min(xs) + max(xs))
    cy = 0.5 * (min(ys) + max(ys))
    span_x = max(xs) - min(xs)
    span_y = max(ys) - min(ys)

    aspect = width / float(height)
    scale_x = span_x / (1.0 - 2.0 * margin)
    scale_y = (span_y * aspect) / (1.0 - 2.0 * margin)
    cam_obj.data.ortho_scale = max(scale_x, scale_y)

    # Place camera 600 mm back from projected center along view_dir_from_target
    rot_mat = rot_quat.to_matrix().to_4x4()
    center_world = rot_mat @ Vector((cx, cy, 0.0))
    cam_obj.location = center_world + direction * 0.70
    cam_obj.rotation_euler = rot_quat.to_euler()


def set_root_pose(root: bpy.types.Object, pose: str) -> None:
    if pose == "use":
        root.matrix_world = Matrix.Translation((0.0, 0.0, Y_SOLE_PRINT * MM)) @ Matrix.Rotation(math.radians(-90.0), 4, "X")
    else:
        root.matrix_world = Matrix.Identity(4)


def set_pod_angle(pod_group: bpy.types.Object, angle_deg: float) -> None:
    axis = Vector((0.0, 0.0, AXIS_Z_PRINT * MM))
    rot = Matrix.Rotation(math.radians(angle_deg - 90.0), 4, "X")
    pod_group.matrix_local = Matrix.Translation(axis) @ rot @ Matrix.Translation(-axis)


def build_rig(
    meshes: dict[str, bpy.types.Mesh],
    root: bpy.types.Object,
    transparent_body: bool = False,
    transparent_pod: bool = False,
) -> dict:
    body_mat = make_material("BodyTeal", "#143f44", roughness=0.45, alpha=0.34 if transparent_body else 1.0)
    pod_mat = make_material("PodOrange", "#c4531b", roughness=0.42, alpha=0.36 if transparent_pod else 1.0)
    rlcd_pcb_mat = make_material("RlcdPcb", "#203b31", roughness=0.50, metallic=0.12)
    xiao_mat = make_material("XiaoStack", "#293340", roughness=0.42, metallic=0.28)
    oled_bezel_mat = make_material("OledBezel", "#0a1116", roughness=0.25)

    rlcd_screen_mat = make_textured_screen_material(
        "RlcdScreen", UI_DIR / "ui-selfie-live.png", "#dce2d8", emission_strength=0.24, roughness=0.55
    )
    oled_screen_mat = make_textured_screen_material(
        "OledScreen", UI_DIR / "ui-oled-selfie.png", "#d4f7f5", emission_strength=0.85, roughness=0.25
    )

    pod_group = bpy.data.objects.new("pod_group", None)
    bpy.context.scene.collection.objects.link(pod_group)
    pod_group.parent = root

    rlcd_pivot = bpy.data.objects.new("rlcd_pivot_group", None)
    bpy.context.scene.collection.objects.link(rlcd_pivot)
    rlcd_pivot.parent = root
    rlcd_pivot.location = RLCD_PIVOT

    rlcd_offset = bpy.data.objects.new("rlcd_mesh_offset", None)
    bpy.context.scene.collection.objects.link(rlcd_offset)
    rlcd_offset.parent = rlcd_pivot
    rlcd_offset.location = -RLCD_PIVOT

    xiao_group = bpy.data.objects.new("xiao_stack_group", None)
    bpy.context.scene.collection.objects.link(xiao_group)
    xiao_group.parent = pod_group

    body_obj = add_mesh_object("body", meshes["body"], body_mat, root)
    pod_obj = add_mesh_object("pod", meshes["pod"], pod_mat, pod_group)
    rlcd_obj = add_mesh_object("hw_rlcd", meshes["hw_rlcd"], rlcd_pcb_mat, rlcd_offset)
    # ST7305 active display quad in CAD print space: x[-42.4,42.4], y[31.2,94.8], z=5.08
    rlcd_quad = make_textured_quad(
        "rlcd_ui_plane",
        [(-42.4, 94.8, 5.08), (42.4, 94.8, 5.08), (42.4, 31.2, 5.08), (-42.4, 31.2, 5.08)],
        rlcd_offset,
        rlcd_screen_mat,
    )
    xiao_obj = add_mesh_object("hw_xiao_stack", meshes["hw_xiao_stack"], xiao_mat, xiao_group)
    oled_box = add_mesh_object("hw_oled_active", meshes["hw_oled_active"], oled_bezel_mat, xiao_group)
    # OLED active display quad cleanly in front of hw_oled_active.stl (y=-26.02 vs min_y=-25.95)
    oled_quad = make_textured_quad(
        "oled_ui_plane",
        [(-5.07, -26.02, 18.52), (16.67, -26.02, 18.52), (16.67, -26.02, 29.38), (-5.07, -26.02, 29.38)],
        xiao_group,
        oled_screen_mat,
    )

    return {
        "pod_group": pod_group,
        "rlcd_pivot": rlcd_pivot,
        "rlcd_offset": rlcd_offset,
        "xiao_group": xiao_group,
        "rlcd_screen_mat": rlcd_screen_mat,
        "oled_screen_mat": oled_screen_mat,
        "tracked": [body_obj, pod_obj, rlcd_obj, rlcd_quad, xiao_obj, oled_box, oled_quad],
    }


def render_to(filepath: Path) -> None:
    filepath.parent.mkdir(parents=True, exist_ok=True)
    bpy.context.scene.render.filepath = str(filepath)
    bpy.ops.render.render(write_still=True)


def main() -> None:
    mesh_names = ["body", "pod", "hw_rlcd", "hw_xiao_stack", "hw_oled_active"]
    geom_cache = {name: parse_stl_geometry(CAD_DIR / f"{name}.stl") for name in mesh_names}

    # 1. Hero Still (1200x900, standing use pose, 180 deg Selfie, full margins, visible OLED + RLCD UI)
    cam, root = setup_studio(1200, 900, samples=48)
    meshes = build_meshes(geom_cache)
    rig = build_rig(meshes, root, transparent_body=False, transparent_pod=False)
    set_root_pose(root, "use")
    fit_ortho_camera(
        cam,
        rig["tracked"],
        (0.46, -0.78, 0.34),
        [lambda: set_pod_angle(rig["pod_group"], 180.0)],
        1200,
        900,
        margin=0.09,
    )
    set_pod_angle(rig["pod_group"], 180.0)
    render_to(OUT_IMAGES / "ditherCam-hero.png")

    # 2. Looping 180-deg Flip GIF frames (800x520, standing use pose, 36 frames: 0 -> 180 -> 0)
    cam, root = setup_studio(800, 520, samples=24)
    meshes = build_meshes(geom_cache)
    rig = build_rig(meshes, root, transparent_body=False, transparent_pod=False)
    img_selfie = bpy.data.images.load(str(UI_DIR / "ui-selfie-live.png"), check_existing=True)
    img_selfie.colorspace_settings.name = "sRGB"
    img_rear_up = bpy.data.images.load(str(UI_DIR / "ui-rear-upright.png"), check_existing=True)
    img_rear_up.colorspace_settings.name = "sRGB"
    img_oled_s = bpy.data.images.load(str(UI_DIR / "ui-oled-selfie.png"), check_existing=True)
    img_oled_s.colorspace_settings.name = "sRGB"
    img_oled_r = bpy.data.images.load(str(UI_DIR / "ui-oled-rear.png"), check_existing=True)
    img_oled_r.colorspace_settings.name = "sRGB"
    rlcd_tex = next(n for n in rig["rlcd_screen_mat"].node_tree.nodes if n.type == "TEX_IMAGE")
    oled_tex = next(n for n in rig["oled_screen_mat"].node_tree.nodes if n.type == "TEX_IMAGE")

    set_root_pose(root, "use")
    fit_ortho_camera(
        cam,
        rig["tracked"],
        (0.62, -0.68, 0.34),
        [
            lambda: set_pod_angle(rig["pod_group"], 0.0),
            lambda: set_pod_angle(rig["pod_group"], 90.0),
            lambda: set_pod_angle(rig["pod_group"], 180.0),
        ],
        800,
        520,
        margin=0.09,
    )
    flip_dir = CACHE_DIR / "flip"
    for f in flip_dir.glob("*.png"):
        f.unlink()

    angles = [90.0 * (1.0 - math.cos(math.pi * (i / 18.0))) for i in range(19)]
    angles += [90.0 * (1.0 + math.cos(math.pi * (i / 18.0))) for i in range(1, 18)]

    for idx, ang in enumerate(angles):
        set_pod_angle(rig["pod_group"], ang)
        is_selfie = ang >= 90.0
        rlcd_tex.image = img_selfie if is_selfie else img_rear_up
        oled_tex.image = img_oled_s if is_selfie else img_oled_r
        render_to(flip_dir / f"frame_{idx:03d}_{ang:06.2f}.png")

    # 3. RLCD Rear-Loading GIF frames (800x520, print pose, transparent teal body)
    cam, root = setup_studio(800, 520, samples=24)
    meshes = build_meshes(geom_cache)
    rig = build_rig(meshes, root, transparent_body=True, transparent_pod=False)
    set_root_pose(root, "print")
    set_pod_angle(rig["pod_group"], 90.0)
    for child in rig["xiao_group"].children:
        child.hide_render = True

    coral_mat = make_material("CoralTab", "#e11d48", roughness=0.35, emission_strength=0.55)
    cyan_mat = make_material("CyanClip", "#0891b2", roughness=0.35, emission_strength=0.55)
    extras = [
        add_marker_sphere("KeeperL", (-21.5, 28.1, 20.0), 3.0, coral_mat, root),
        add_marker_sphere("KeeperR", (21.5, 28.1, 20.0), 3.0, coral_mat, root),
        add_marker_sphere("RetainerBL", (-38.0, 99.5, 20.0), 2.8, cyan_mat, root),
        add_marker_sphere("RetainerBR", (38.0, 99.5, 20.0), 2.8, cyan_mat, root),
        add_marker_sphere("RetainerSL", (-45.6, 79.0, 20.0), 2.8, cyan_mat, root),
        add_marker_sphere("RetainerSR", (48.9, 79.0, 20.0), 2.8, cyan_mat, root),
        *add_direction_arrow("RlcdEntryArrow", (0.0, 64.0, 84.0), (0.0, 64.0, 54.0), cyan_mat, root),
    ]

    rlcd_keyframes = []
    for i in range(8):
        p = i / 7.0
        rlcd_keyframes.append((54.0 * (1.0 - p) + 34.0 * p, 0.0, "1_align"))
    for i in range(1, 15):
        p = i / 14.0
        tilt = 16.0 * math.sin(min(1.0, p * 2.0) * math.pi * 0.5) if p < 0.5 else 16.0
        rlcd_keyframes.append((34.0 * (1.0 - p), tilt, "2_tuck_tabs"))
    for i in range(1, 11):
        p = i / 10.0
        rlcd_keyframes.append((0.0, 16.0 * (1.0 - p), "3_seat_clips"))

    def set_rlcd_pose(z_off: float, tilt_deg: float):
        rig["rlcd_pivot"].location = RLCD_PIVOT + Vector((0.0, 0.0, z_off * MM))
        rig["rlcd_pivot"].rotation_euler = (math.radians(tilt_deg), 0.0, 0.0)

    fit_ortho_camera(
        cam,
        rig["tracked"] + extras,
        (0.50, 0.62, 0.58),
        [
            lambda: set_rlcd_pose(54.0, 0.0),
            lambda: set_rlcd_pose(17.0, 16.0),
            lambda: set_rlcd_pose(0.0, 0.0),
        ],
        800,
        520,
        margin=0.09,
    )
    rlcd_dir = CACHE_DIR / "rlcd"
    for f in rlcd_dir.glob("*.png"):
        f.unlink()

    for idx, (z_off, tilt_deg, tag) in enumerate(rlcd_keyframes):
        set_rlcd_pose(z_off, tilt_deg)
        render_to(rlcd_dir / f"frame_{idx:03d}_{tag}.png")

    # 4. Camera-stack top-loading GIF frames (800x520, print pose, pod at 90 deg, transparent orange pod)
    cam, root = setup_studio(800, 520, samples=24)
    meshes = build_meshes(geom_cache)
    rig = build_rig(meshes, root, transparent_body=False, transparent_pod=True)
    set_root_pose(root, "print")
    set_pod_angle(rig["pod_group"], 90.0)

    coral_mat = make_material("CoralDetent", "#e11d48", roughness=0.35, emission_strength=0.55)
    orange_arrow_mat = make_material("OrangeArrow", "#e66a22", roughness=0.35, emission_strength=0.40)
    extras = [
        add_marker_sphere("DetentL", (-25.01, -21.10, 6.35), 2.6, coral_mat, rig["pod_group"]),
        add_marker_sphere("DetentR", (25.01, -21.10, 6.35), 2.6, coral_mat, rig["pod_group"]),
        *add_direction_arrow("PodEntryArrow", (0.0, -28.0, 108.0), (0.0, -28.0, 74.0), orange_arrow_mat, rig["pod_group"]),
    ]

    def set_xiao_z(z_mm: float):
        rig["xiao_group"].location = Vector((0.0, 0.0, z_mm * MM))

    fit_ortho_camera(
        cam,
        rig["tracked"] + extras,
        (-0.48, -0.58, 0.56),
        [lambda: set_xiao_z(50.0), lambda: set_xiao_z(0.0)],
        800,
        520,
        margin=0.09,
    )
    pod_dir = CACHE_DIR / "pod"
    for f in pod_dir.glob("*.png"):
        f.unlink()

    for idx in range(30):
        p = idx / 29.0
        eased = 0.5 * (1.0 - math.cos(math.pi * p))
        z_off = 50.0 * (1.0 - eased)
        set_xiao_z(z_off)
        render_to(pod_dir / f"frame_{idx:03d}_{z_off:05.1f}.png")

    print("Blender renders completed cleanly.")


if __name__ == "__main__":
    main()
