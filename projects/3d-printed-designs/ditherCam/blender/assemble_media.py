#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Assemble ditherCam hero PNG and labeled animated GIFs from Blender frames.

Strips all EXIF/text metadata, adds plain-language CAD animation captions,
enforces <8 MB size budget, verifies decoded GIF frame count and pixel motion
variance, and writes verification/media-manifest.json.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageSequence, PngImagePlugin

BLENDER_DIR = Path(__file__).resolve().parent
DESIGN_DIR = BLENDER_DIR.parent
CACHE_DIR = BLENDER_DIR / ".frame-cache"
OUT_IMAGES = DESIGN_DIR / "images"
CAD_DIR = DESIGN_DIR / "cad"
MANIFEST_PATH = DESIGN_DIR / "verification" / "media-manifest.json"


def strip_png_inplace(path: Path) -> tuple[int, int]:
    with Image.open(path) as im:
        arr = np.asarray(im)
        mode = im.mode
        w, h = im.size
    clean = Image.fromarray(arr, mode=mode)
    clean.save(path, format="PNG", optimize=True, pnginfo=PngImagePlugin.PngInfo())
    return w, h


def annotate_frame(
    raw_im: Image.Image,
    title: str,
    subtitle: str,
    footer: str,
) -> Image.Image:
    """Place rendered CAD frame inside a clean 800x600 labeled canvas."""
    w, h = raw_im.size  # Expected 800x520
    canvas = Image.new("RGB", (w, h + 80), (242, 240, 234))
    canvas.paste(raw_im.convert("RGB"), (0, 46))
    draw = ImageDraw.Draw(canvas)

    f_title = ImageFont.load_default(size=18)
    f_sub = ImageFont.load_default(size=13)
    f_foot = ImageFont.load_default(size=13)

    draw.rectangle((0, 0, w, 46), fill=(26, 48, 52))
    draw.text((16, 6), title, font=f_title, fill=(246, 245, 240))
    draw.text((16, 27), subtitle, font=f_sub, fill=(195, 218, 216))

    draw.rectangle((0, h + 46, w, h + 80), fill=(230, 227, 219))
    draw.line((0, h + 46, w, h + 46), fill=(190, 186, 176), width=1)
    draw.text((16, h + 55), footer, font=f_foot, fill=(45, 56, 54))
    return canvas


def encode_and_verify_gif(
    frames_rgb: list[Image.Image],
    durations_ms: list[int],
    out_path: Path,
    min_distinct_frames: int = 24,
) -> dict:
    assert len(frames_rgb) == len(durations_ms)
    assert len(frames_rgb) >= min_distinct_frames

    pal_frames = [
        im.quantize(colors=256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.FLOYDSTEINBERG)
        for im in frames_rgb
    ]
    out_path.parent.mkdir(parents=True, exist_ok=True)
    pal_frames[0].save(
        out_path,
        save_all=True,
        append_images=pal_frames[1:],
        duration=durations_ms,
        loop=0,
        disposal=2,
        optimize=True,
    )

    size_bytes = out_path.stat().st_size
    assert size_bytes < 8 * 1024 * 1024, f"{out_path.name} exceeded 8 MB: {size_bytes}"

    # Decode back and verify frame count, loop, duration, and non-trivial pixel variance
    decoded_arrays: list[np.ndarray] = []
    decoded_durations: list[int] = []
    with Image.open(out_path) as gif:
        assert gif.info.get("loop") == 0
        for fr in ImageSequence.Iterator(gif):
            decoded_durations.append(int(fr.info.get("duration", 0)))
            decoded_arrays.append(np.asarray(fr.convert("RGB"), dtype=np.int16))

    assert len(decoded_arrays) >= min_distinct_frames, f"{out_path.name}: {len(decoded_arrays)}"
    diffs = [
        float(np.mean(np.abs(decoded_arrays[i] - decoded_arrays[i - 1])))
        for i in range(1, len(decoded_arrays))
    ]
    max_diff = max(diffs)
    mean_diff = float(np.mean(diffs))
    assert max_diff > 0.5, f"{out_path.name} has near-zero motion: {max_diff}"

    return {
        "file": str(out_path.relative_to(DESIGN_DIR)),
        "width": frames_rgb[0].width,
        "height": frames_rgb[0].height,
        "frame_count": len(decoded_arrays),
        "total_duration_ms": sum(decoded_durations),
        "bytes": size_bytes,
        "mean_interframe_l1": round(mean_diff, 3),
        "max_interframe_l1": round(max_diff, 3),
        "sha256": hashlib.sha256(out_path.read_bytes()).hexdigest(),
    }


def build_flip_gif() -> dict:
    paths = sorted((CACHE_DIR / "flip").glob("frame_*.png"))
    assert len(paths) >= 24, f"Expected >=24 flip frames, got {len(paths)}"
    frames: list[Image.Image] = []
    durations: list[int] = []
    for i, p in enumerate(paths):
        ang = float(p.stem.split("_")[-1])
        pose_txt = (
            "0 deg Forward (Outward)"
            if ang <= 2.0
            else "180 deg Selfie"
            if ang >= 178.0
            else f"{ang:.0f} deg Flipping"
        )
        with Image.open(p) as im:
            frames.append(
                annotate_frame(
                    im,
                    f"ditherCam - Captive 180 deg Pod Flip  |  {pose_txt}",
                    "CAD animation (not hardware footage) · Dark teal body + warm orange pod",
                    "Manual SELFIE/REAR mode toggle on RLCD (no hinge sensor) · Standing 108 x 58 mm base",
                )
            )
        if i == 0:
            durations.append(950)
        elif ang >= 179.9:
            durations.append(1200)
        else:
            durations.append(90)
    return encode_and_verify_gif(frames, durations, OUT_IMAGES / "ditherCam-flip.gif")


def build_rlcd_gif() -> dict:
    paths = sorted((CACHE_DIR / "rlcd").glob("frame_*.png"))
    assert len(paths) >= 24, f"Expected >=24 RLCD frames, got {len(paths)}"
    frames: list[Image.Image] = []
    durations: list[int] = []
    for i, p in enumerate(paths):
        tag = "_".join(p.stem.split("_")[2:])
        if tag == "1_align":
            phase = "Step 1: Offer RLCD glass-down into open rear bay (stand legs removed)"
        elif tag == "2_tuck_tabs":
            phase = "Step 2: Illustrative 16 deg tilt tucking top edge under rigid coral tabs"
        else:
            phase = "Step 3: Level past 4 cyan clips in board plane (clips shown rigid, unbent)"
        with Image.open(p) as im:
            frames.append(
                annotate_frame(
                    im,
                    "ditherCam - RLCD Rear Loading (Illustrative Tilt Path)",
                    "CAD animation (not hardware footage) · Coral = rigid top tabs · Cyan = compliant clips",
                    phase,
                )
            )
        if i == 0:
            durations.append(900)
        elif i == len(paths) - 1:
            durations.append(1400)
        else:
            durations.append(105)
    return encode_and_verify_gif(frames, durations, OUT_IMAGES / "ditherCam-rlcd-insert.gif")


def build_pod_gif() -> dict:
    paths = sorted((CACHE_DIR / "pod").glob("frame_*.png"))
    assert len(paths) >= 24, f"Expected >=24 pod frames, got {len(paths)}"
    frames: list[Image.Image] = []
    durations: list[int] = []
    for i, p in enumerate(paths):
        z_mm = float(p.stem.split("_")[-1])
        phase = (
            "Seated behind stepped faceplate; coral bottom hole detents engage PCB holes"
            if z_mm <= 0.2
            else f"Top-down slide along bare-PCB guides ({z_mm:.0f} mm -> 0 mm) · No SD or battery"
        )
        with Image.open(p) as im:
            frames.append(
                annotate_frame(
                    im,
                    "ditherCam - Camera Stack Top Loading (Pod at 90 deg)",
                    "CAD animation (not hardware footage) · Plugged XIAO + Sense + Expansion Base + OLED",
                    phase,
                )
            )
        if i == 0:
            durations.append(900)
        elif i == len(paths) - 1:
            durations.append(1400)
        else:
            durations.append(95)
    return encode_and_verify_gif(frames, durations, OUT_IMAGES / "ditherCam-pod-insert.gif")


def main() -> int:
    hero_path = OUT_IMAGES / "ditherCam-hero.png"
    hw, hh = strip_png_inplace(hero_path)

    hero_entry = {
        "file": str(hero_path.relative_to(DESIGN_DIR)),
        "width": hw,
        "height": hh,
        "bytes": hero_path.stat().st_size,
        "sha256": hashlib.sha256(hero_path.read_bytes()).hexdigest(),
        "provenance": "Blender 5.1.1 BLENDER_EEVEE render of exact CAD STLs in standing Selfie (180 deg) pose with firmware-rendered ui-selfie-live.png texture on ST7305 active area; no lens FOV cone",
    }

    manifest = {
        "generator_blender": "projects/3d-printed-designs/ditherCam/blender/render_dithercam.py",
        "generator_assembler": "projects/3d-printed-designs/ditherCam/blender/assemble_media.py",
        "source_stl_sha256": {
            p.name: hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(CAD_DIR.glob("*.stl"))
        },
        "hero": hero_entry,
        "animations": [
            build_flip_gif(),
            build_rlcd_gif(),
            build_pod_gif(),
        ],
    }

    # Build coordinator visual review contact sheet (first/mid/last frames of each GIF)
    sheet = Image.new("RGB", (1260, 1020), (240, 238, 232))
    draw = ImageDraw.Draw(sheet)
    f_head = ImageFont.load_default(size=18)
    draw.text((24, 16), "ditherCam Visual Review Contact Sheet (Hero + Start / Mid / End Frames)", font=f_head, fill=(24, 36, 36))
    rows = [
        ("Flip 0->180->0", sorted((CACHE_DIR / "flip").glob("frame_*.png"))),
        ("RLCD Rear Load", sorted((CACHE_DIR / "rlcd").glob("frame_*.png"))),
        ("Pod Top Load", sorted((CACHE_DIR / "pod").glob("frame_*.png"))),
    ]
    for r_idx, (label, plist) in enumerate(rows):
        y = 54 + r_idx * 320
        draw.text((24, y), label, font=f_head, fill=(24, 70, 74))
        picks = [plist[0], plist[len(plist) // 2], plist[-1]]
        for c_idx, fp in enumerate(picks):
            with Image.open(fp) as fr:
                thumb = fr.convert("RGB").resize((390, 254), Image.Resampling.LANCZOS)
            x = 24 + c_idx * 408
            sheet.paste(thumb, (x, y + 26))
            draw.rectangle((x, y + 26, x + 390, y + 280), outline=(140, 146, 142), width=1)
    sheet.save(CACHE_DIR / "review-contact-sheet.png", format="PNG", pnginfo=PngImagePlugin.PngInfo())

    MANIFEST_PATH.parent.mkdir(parents=True, exist_ok=True)
    MANIFEST_PATH.write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
