#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Generate synthetic test scene and render final firmware UI PNGs via FlipCam C++."""
from __future__ import annotations

import hashlib
import json
import os
import subprocess
import tempfile
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont, PngImagePlugin

ROOT = Path(__file__).resolve().parent.parent
OUT_DIR = ROOT / "docs" / "images"


def make_synthetic_landscape(w: int, h: int, variant: str = "live") -> np.ndarray:
    """Create a recognizable high-contrast 4:3 synthetic scene for 1-bit dithering."""
    img = Image.new("L", (w, h), 228)
    draw = ImageDraw.Draw(img)

    # Sky vertical gradient (light top to slightly softer horizon)
    arr = np.asarray(img, dtype=np.float32)
    yy = np.linspace(0.0, 1.0, h)[:, None]
    sky_mask = yy < 0.62
    arr = np.where(sky_mask, 245.0 - 70.0 * (yy / 0.62), arr)
    img = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8), mode="L")
    draw = ImageDraw.Draw(img)

    # Sun & soft clouds (top-left sky so 180-deg flip is unmistakably clear)
    sun_x, sun_y, sun_r = int(w * 0.18), int(h * 0.20), int(min(w, h) * 0.085)
    draw.ellipse((sun_x - sun_r - 8, sun_y - sun_r - 8, sun_x + sun_r + 8, sun_y + sun_r + 8), fill=255)
    draw.ellipse((sun_x - sun_r, sun_y - sun_r, sun_x + sun_r, sun_y + sun_r), fill=255, outline=85, width=2)
    # Cloud cluster top-right
    for cx, cy, rx, ry in [
        (0.68, 0.17, 0.09, 0.045),
        (0.75, 0.15, 0.08, 0.05),
        (0.81, 0.18, 0.07, 0.04),
    ]:
        draw.ellipse(
            (int(w * (cx - rx)), int(h * (cy - ry)), int(w * (cx + rx)), int(h * (cy + ry))),
            fill=252,
            outline=145,
            width=1,
        )

    # Layered mountains
    draw.polygon(
        [
            (0, int(h * 0.62)),
            (int(w * 0.16), int(h * 0.38)),
            (int(w * 0.34), int(h * 0.56)),
            (int(w * 0.56), int(h * 0.30)),
            (int(w * 0.78), int(h * 0.52)),
            (w, int(h * 0.40)),
            (w, int(h * 0.64)),
        ],
        fill=148,
        outline=45,
    )
    draw.polygon(
        [
            (0, int(h * 0.64)),
            (int(w * 0.26), int(h * 0.46)),
            (int(w * 0.48), int(h * 0.62)),
            (int(w * 0.72), int(h * 0.44)),
            (w, int(h * 0.60)),
            (w, int(h * 0.65)),
        ],
        fill=108,
        outline=30,
    )

    # Ground plane with shaded field + path
    draw.rectangle((0, int(h * 0.62), w, h), fill=172)
    draw.polygon(
        [
            (int(w * 0.48), int(h * 0.62)),
            (int(w * 0.56), int(h * 0.62)),
            (int(w * 0.76), h),
            (int(w * 0.34), h),
        ],
        fill=222,
        outline=95,
    )

    # Stylized pine tree on left
    tx, ty = int(w * 0.22), int(h * 0.78)
    draw.rectangle((tx - int(w * 0.018), ty - int(h * 0.06), tx + int(w * 0.018), ty + int(h * 0.06)), fill=42)
    for tier, (tw, th, top_y) in enumerate(
        [(0.075, 0.14, 0.44), (0.095, 0.15, 0.52), (0.115, 0.16, 0.60)]
    ):
        draw.polygon(
            [
                (tx, int(h * top_y)),
                (tx - int(w * tw), int(h * (top_y + th))),
                (tx + int(w * tw), int(h * (top_y + th))),
            ],
            fill=38 + tier * 14,
            outline=15,
        )

    # Small cabin/house on right-center
    hx0, hy0, hx1, hy1 = int(w * 0.54), int(h * 0.52), int(w * 0.74), int(h * 0.74)
    draw.rectangle((hx0, hy0, hx1, hy1), fill=232, outline=25, width=2)
    draw.polygon(
        [(hx0 - int(w * 0.025), hy0), (int((hx0 + hx1) / 2), int(h * 0.40)), (hx1 + int(w * 0.025), hy0)],
        fill=68,
        outline=20,
    )
    # Door & bright/dark window
    dw0, dw1 = int(w * 0.58), int(w * 0.63)
    draw.rectangle((dw0, int(h * 0.61), dw1, hy1), fill=52, outline=20, width=2)
    draw.rectangle((int(w * 0.66), int(h * 0.57), int(w * 0.71), int(h * 0.65)), fill=255, outline=25, width=2)
    draw.line((int(w * 0.685), int(h * 0.57), int(w * 0.685), int(h * 0.65)), fill=25, width=2)
    draw.line((int(w * 0.66), int(h * 0.61), int(w * 0.71), int(h * 0.61)), fill=25, width=2)

    # Foreground fence posts on lower-right for extra texture
    for fx in range(int(w * 0.78), int(w * 0.96), int(w * 0.045)):
        draw.rectangle((fx, int(h * 0.66), fx + int(w * 0.014), int(h * 0.80)), fill=70, outline=25)
    draw.rectangle((int(w * 0.76), int(h * 0.70), int(w * 0.97), int(h * 0.72)), fill=90, outline=25)

    if variant == "gallery":
        # Distinct saved-photo framing: add a crisp geometric target kite in sky
        kx, ky = int(w * 0.42), int(h * 0.24)
        draw.polygon([(kx, ky - 22), (kx + 18, ky), (kx, ky + 28), (kx - 18, ky)], fill=72, outline=20)
        draw.line((kx, ky + 28, kx + 10, ky + 56), fill=40, width=2)

    return np.asarray(img, dtype=np.uint8)


def save_clean_png(im: Image.Image, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    im.save(path, format="PNG", optimize=False, pnginfo=PngImagePlugin.PngInfo())


def load_pgm(path: Path) -> Image.Image:
    with Image.open(path) as im:
        return im.convert("L").copy()


def build_contact_panel(
    selfie: Image.Image,
    hold: Image.Image,
    rear: Image.Image,
    gallery: Image.Image,
    oled_selfie: Image.Image,
    oled_rear: Image.Image,
) -> Image.Image:
    """Arrange actual firmware outputs on a clean labeled panel (annotations strictly outside firmware images)."""
    bg = (242, 241, 236)
    ink = (28, 36, 35)
    muted = (84, 96, 94)
    card_border = (176, 184, 180)
    accent = (32, 92, 88)

    panel = Image.new("RGB", (1340, 880), bg)
    draw = ImageDraw.Draw(panel)
    title_font = ImageFont.load_default(size=24)
    sub_font = ImageFont.load_default(size=15)
    head_font = ImageFont.load_default(size=17)
    cap_font = ImageFont.load_default(size=14)

    draw.text((32, 22), "ditherCam - Final Firmware UI (C++ Renderer · Synthetic Scene)", font=title_font, fill=ink)
    draw.text(
        (32, 54),
        "Rendered directly by shared/FlipCamDisplay.h + FlipCamDither.h · 400x300 RLCD & 128x64 OLED · Battery '?' = unmeasured/demo (no fabricated charge)",
        font=sub_font,
        fill=muted,
    )

    # Top row: Selfie Live, Held Snap (65% progress), Rear Live (180-deg image, upright UI)
    top_cards = [
        (32, 96, selfie, "1. Selfie Live (90 deg CW baseline)", "Full 400x300 dithered view · Tap SNAP:STILL, Tap BROWSE:PREV"),
        (470, 96, hold, "2. Hold Snap (~1 s to switch mode)", "Badge fills with progress · Release after switch does not shoot"),
        (908, 96, rear, "3. Rear mode on SAME stationary frame", "Same input before physical flip; flipped pod shoots upright"),
    ]
    for x, y, im, heading, caption in top_cards:
        draw.text((x, y), heading, font=head_font, fill=accent)
        draw.rectangle((x - 2, y + 26, x + 402, y + 330), outline=card_border, width=2)
        panel.paste(im.convert("RGB"), (x, y + 28))
        draw.text((x, y + 336), caption, font=cap_font, fill=muted)

    # Bottom row: Gallery + two OLED views (3x integer scale: 384x192)
    gx, gy = 32, 476
    draw.text((gx, gy), "4. Gallery (newest -> older, stops at OLDEST)", font=head_font, fill=accent)
    draw.rectangle((gx - 2, gy + 26, gx + 402, gy + 330), outline=card_border, width=2)
    panel.paste(gallery.convert("RGB"), (gx, gy + 28))
    draw.text((gx, gy + 336), "Saved 600x450 stills are never re-rotated when modes change", font=cap_font, fill=muted)

    # OLED comparison card (right side bottom)
    ox, oy = 470, 476
    draw.text((ox, oy), "5. Mini OLED (128x64, shown 3x = 384x192) - Preview Unchanged Across Modes", font=head_font, fill=accent)
    # Selfie OLED
    oled_s_rgb = oled_selfie.resize((384, 192), Image.Resampling.NEAREST).convert("RGB")
    oled_r_rgb = oled_rear.resize((384, 192), Image.Resampling.NEAREST).convert("RGB")
    draw.rectangle((ox - 2, oy + 34, ox + 386, oy + 230), outline=card_border, width=2)
    panel.paste(oled_s_rgb, (ox, oy + 36))
    draw.text((ox, oy + 240), "Selfie / LIVE status · 68x51 4:3 preview · '?' battery", font=cap_font, fill=muted)

    ox2 = 908
    draw.rectangle((ox2 - 2, oy + 34, ox2 + 386, oy + 230), outline=card_border, width=2)
    panel.paste(oled_r_rgb, (ox2, oy + 36))
    draw.text((ox2, oy + 240), "Rear mode · Status changes to REAR · Preview pixels identical", font=cap_font, fill=muted)

    draw.text(
        (32, 844),
        "Note: firmware renderer · synthetic scene. Full-screen 400x300 prototype omits the family version/date status bar (tracked exception).",
        font=cap_font,
        fill=muted,
    )
    return panel


def main() -> int:
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    # Build 240x180 upright scene, rotate 90 deg CCW -> 240x180 (h=240, w=180), place centered in 320x240 raw sensor frame
    upright_scene = make_synthetic_landscape(240, 180, variant="live")
    raw_full = np.full((240, 320), 180, dtype=np.uint8)
    ccw = np.rot90(upright_scene, k=1)
    assert ccw.shape == (240, 180)
    raw_full[:, 70:250] = ccw
    raw_full[:, :70] = ccw[:, :1]
    raw_full[:, 250:] = ccw[:, -1:]

    # Physically flipped pod (180 deg outward) rotates sensor 180 deg -> CW instead of CCW
    raw_rear_flipped = np.full((240, 320), 180, dtype=np.uint8)
    cw = np.rot90(upright_scene, k=-1)
    assert cw.shape == (240, 180)
    raw_rear_flipped[:, 70:250] = cw
    raw_rear_flipped[:, :70] = cw[:, :1]
    raw_rear_flipped[:, 250:] = cw[:, -1:]

    gallery_600x450 = make_synthetic_landscape(600, 450, variant="gallery")

    with tempfile.TemporaryDirectory(prefix="dithercam-ui-") as tmp:
        tmp_dir = Path(tmp)
        raw_path = tmp_dir / "raw_320x240.bin"
        raw_rear_path = tmp_dir / "raw_rear_320x240.bin"
        gal_path = tmp_dir / "gallery_600x450.bin"
        raw_path.write_bytes(raw_full.tobytes())
        raw_rear_path.write_bytes(raw_rear_flipped.tobytes())
        gal_path.write_bytes(gallery_600x450.tobytes())

        exe = tmp_dir / "render_ui"
        clt_clang = Path("/Library/Developer/CommandLineTools/usr/bin/clang++")
        clt_sdk = Path("/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk")
        cxx = str(clt_clang) if clt_clang.is_file() else "c++"
        cmd = [cxx, "-std=c++17", "-Wall", "-Wextra", "-Werror", "-O2"]
        env = dict(os.environ)
        if clt_clang.is_file() and clt_sdk.is_dir():
            env["DEVELOPER_DIR"] = "/Library/Developer/CommandLineTools"
            cmd.extend(["-isysroot", str(clt_sdk)])
        cmd.extend([f"-I{ROOT / 'shared'}", str(ROOT / "tools" / "render_ui.cpp"), "-o", str(exe)])
        subprocess.run(cmd, env=env, check=True)

        res = subprocess.run([str(exe), str(raw_path), str(raw_rear_path), str(gal_path), str(tmp_dir)], capture_output=True, text=True, check=True)
        crcs = json.loads(res.stdout.strip())

        selfie = load_pgm(tmp_dir / "rlcd-selfie-live.pgm")
        hold = load_pgm(tmp_dir / "rlcd-snap-hold.pgm")
        rear = load_pgm(tmp_dir / "rlcd-rear-live.pgm")
        rear_upright = load_pgm(tmp_dir / "rlcd-rear-upright.pgm")
        gallery = load_pgm(tmp_dir / "rlcd-gallery.pgm")
        oled_selfie = load_pgm(tmp_dir / "oled-selfie-live.pgm")
        oled_rear = load_pgm(tmp_dir / "oled-rear-live.pgm")

    outputs = {
        "ui-selfie-live.png": selfie,
        "ui-snap-hold.png": hold,
        "ui-rear-live.png": rear,
        "ui-rear-upright.png": rear_upright,
        "ui-gallery.png": gallery,
        "ui-oled-selfie.png": oled_selfie.resize((384, 192), Image.Resampling.NEAREST),
        "ui-oled-rear.png": oled_rear.resize((384, 192), Image.Resampling.NEAREST),
        "ui-firmware-panel.png": build_contact_panel(selfie, hold, rear, gallery, oled_selfie, oled_rear),
    }

    manifest = {
        "generator": "projects/ditherCam/tools/generate_ui_previews.py",
        "renderer_cpp": "projects/ditherCam/tools/render_ui.cpp",
        "label": "firmware renderer · synthetic scene",
        "crc32": crcs,
        "files": {},
    }
    for name, im in outputs.items():
        out_p = OUT_DIR / name
        save_clean_png(im, out_p)
        manifest["files"][name] = {
            "width": im.width,
            "height": im.height,
            "sha256": hashlib.sha256(out_p.read_bytes()).hexdigest(),
            "bytes": out_p.stat().st_size,
        }

    (OUT_DIR / "ui-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print("Generated firmware UI previews:", json.dumps(manifest["files"], indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
