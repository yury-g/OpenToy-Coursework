#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Validate ditherCam docs, links, image metadata, GIF decoding, CAD hashes, and privacy hygiene."""
from __future__ import annotations

import hashlib
import json
import re
import zipfile
from pathlib import Path

import numpy as np
from PIL import Image, ImageSequence

REPO_ROOT = Path(__file__).resolve().parents[3]
DITHERCAM_DIR = REPO_ROOT / "projects" / "ditherCam"
DESIGN_DIR = REPO_ROOT / "projects" / "3d-printed-designs" / "ditherCam"

MD_FILES = [
    REPO_ROOT / "README.md",
    REPO_ROOT / "projects" / "README.md",
    REPO_ROOT / "projects" / "3d-printed-designs" / "README.md",
    DITHERCAM_DIR / "README.md",
    DITHERCAM_DIR / "docs" / "DEVELOPMENT-LOG.md",
    DITHERCAM_DIR / "docs" / "SETUP.md",
    DITHERCAM_DIR / "docs" / "PROTOCOL.md",
    DITHERCAM_DIR / "docs" / "VERIFICATION.md",
    DITHERCAM_DIR / "docs" / "visual-history.md",
    DITHERCAM_DIR / "docs" / "LICENSES.md",
    DESIGN_DIR / "README.md",
    DESIGN_DIR / "assembly" / "README.md",
    DESIGN_DIR / "assembly" / "BUILD-RESULT.md",
    DESIGN_DIR / "blender" / "README.md",
    DESIGN_DIR / "reference-README.md",
    DESIGN_DIR / "reference-SOURCES.md",
]

LINK_RE = re.compile(r"(!?\[[^\]]*\]\(([^)]+)\)|<(?:img|a)\b[^>]*?\b(?:src|href)=\"([^\"]+)\")")


def check_markdown_links() -> int:
    checked = 0
    for md in MD_FILES:
        text = md.read_text(encoding="utf-8")
        for m in LINK_RE.finditer(text):
            raw_target = m.group(2) or m.group(3)
            if not raw_target or raw_target.startswith(("http://", "https://", "mailto:", "#")):
                continue
            clean = raw_target.split("#", 1)[0].split("?", 1)[0].strip()
            if not clean:
                continue
            target = (md.parent / clean).resolve()
            assert target.exists(), f"Broken link in {md.relative_to(REPO_ROOT)}: {raw_target} -> {target}"
            checked += 1
    return checked


def check_visual_history() -> int:
    inv = json.loads((DITHERCAM_DIR / "docs" / "design-history" / "inventory.json").read_text())
    vh_text = (DITHERCAM_DIR / "docs" / "visual-history.md").read_text()
    for entry in inv:
        rel_from_docs = entry["file"].removeprefix("docs/")
        assert rel_from_docs in vh_text, f"Missing visual history entry: {entry['file']}"
        p = DITHERCAM_DIR / entry["file"]
        with Image.open(p) as im:
            assert len(im.getexif()) == 0, f"EXIF remains in {p}"
            for k in ("File", "Date", "Time", "exif", "Camera", "Scene", "RenderTime"):
                assert k not in im.info, f"Text chunk {k} remains in {p}"
    return len(inv)


def check_generated_media() -> None:
    # UI PNGs
    ui_manifest = json.loads((DITHERCAM_DIR / "docs" / "images" / "ui-manifest.json").read_text())
    for fname, meta in ui_manifest["files"].items():
        p = DITHERCAM_DIR / "docs" / "images" / fname
        assert p.is_file(), f"Missing UI PNG: {p}"
        assert hashlib.sha256(p.read_bytes()).hexdigest() == meta["sha256"]
        with Image.open(p) as im:
            assert im.size == (meta["width"], meta["height"])
            assert len(im.getexif()) == 0

    # Blender hero & GIFs
    manifest = json.loads((DESIGN_DIR / "verification" / "media-manifest.json").read_text())
    hero_p = DESIGN_DIR / manifest["hero"]["file"]
    assert hero_p.is_file()
    with Image.open(hero_p) as im:
        assert im.size == (manifest["hero"]["width"], manifest["hero"]["height"])
        assert len(im.getexif()) == 0
        assert not any(k in im.info for k in ("File", "Date", "Time", "Camera", "Scene"))

    for anim in manifest["animations"]:
        gp = DESIGN_DIR / anim["file"]
        assert gp.is_file()
        assert gp.stat().st_size < 8 * 1024 * 1024
        with Image.open(gp) as gif:
            assert 720 <= gif.width <= 1000
            frames = [np.asarray(fr.convert("RGB"), dtype=np.int16) for fr in ImageSequence.Iterator(gif)]
            assert len(frames) >= 24, f"{gp.name}: only {len(frames)} frames"
            diffs = [float(np.mean(np.abs(frames[i] - frames[i - 1]))) for i in range(1, len(frames))]
            assert max(diffs) > 0.5, f"{gp.name}: zero motion"


def check_cad_hashes_and_3mf() -> None:
    expected = json.loads((DESIGN_DIR / "verification" / "model-sha256.json").read_text())
    for fname, digest in expected.items():
        cand = DESIGN_DIR / "cad" / fname
        if not cand.exists():
            cand = DESIGN_DIR / "print" / fname
        assert cand.exists(), f"Missing CAD/print file {fname}"
        actual = hashlib.sha256(cand.read_bytes()).hexdigest()
        assert actual == digest, f"Hash mismatch for {fname}: {actual} != {digest}"

    mf = DESIGN_DIR / "print" / "Camera-U1-PLA-preview.3mf"
    with zipfile.ZipFile(mf, "r") as zf:
        for name in zf.namelist():
            data = zf.read(name)
            for forbidden in (b"/Us" + b"ers/", b"mini" + b"narwhal", b"Co" + b"dex"):
                assert forbidden not in data, f"Forbidden {forbidden!r} in {mf.name}:{name}"


def check_privacy_scan() -> None:
    skip_dirs = {".frame-cache", "node_modules", "build", "artifacts", "__pycache__"}
    forbidden_re = re.compile(
        "|".join(["/Us" + "ers/", "mini" + "narwhal", "Co" + "dex"])
    )
    text_exts = {".md", ".py", ".cpp", ".c", ".h", ".ino", ".js", ".html", ".json", ".csv", ".scad", ".txt"}
    for root in (DITHERCAM_DIR, DESIGN_DIR):
        for p in root.rglob("*"):
            if not p.is_file():
                continue
            if any(part in skip_dirs for part in p.parts):
                continue
            if p.suffix.lower() not in text_exts:
                continue
            content = p.read_text(encoding="utf-8", errors="replace")
            m = forbidden_re.search(content)
            assert not m, f"Workstation/private string {m.group(0)!r} found in {p.relative_to(REPO_ROOT)}"


def main() -> int:
    links = check_markdown_links()
    hist = check_visual_history()
    check_generated_media()
    check_cad_hashes_and_3mf()
    check_privacy_scan()
    print(f"check_docs_and_assets.py: OK (checked {links} links, {hist} history images, media manifests, CAD hashes, 3MF & privacy scan).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
