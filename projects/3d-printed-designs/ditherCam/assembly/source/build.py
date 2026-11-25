#!/usr/bin/env python3
"""Build deliverables/assembly/index.html from source/ and reference/model/."""

from __future__ import annotations

import base64
import json
import os
import struct
import subprocess
import sys
from pathlib import Path

SOURCE_DIR = Path(__file__).resolve().parent
ASSEMBLY_DIR = SOURCE_DIR.parent
DESIGN_DIR = ASSEMBLY_DIR.parent
MODEL_DIR = DESIGN_DIR / "cad"
DELIVERABLE_DIR = ASSEMBLY_DIR
BUNDLE_PATH = SOURCE_DIR / "assembly.bundle.js"
TEMPLATE_PATH = SOURCE_DIR / "assembly-template.html"

# IMPORTANT: Never include hw_xiao_pod.stl (combined battery+SD mesh).
# Only embed the bare stack hw_xiao_stack.stl + active OLED + RLCD + body + pod.
INCLUDED_MESHES = [
    "body",
    "pod",
    "hw_rlcd",
    "hw_xiao_stack",
    "hw_oled_active",
]
EXCLUDED_MESHES = ["hw_xiao_pod"]


def ascii_stl_to_binary(stl_path: Path) -> bytes:
    """Convert ASCII (or binary) STL to compact little-endian binary STL bytes."""
    raw = stl_path.read_bytes()
    # If already valid binary STL, return as-is.
    if len(raw) >= 84 and not raw.lstrip().startswith(b"solid"):
        tri_count = struct.unpack("<I", raw[80:84])[0]
        if len(raw) == 84 + tri_count * 50:
            return raw

    try:
        import trimesh  # type: ignore

        mesh = trimesh.load_mesh(stl_path)
        return mesh.export(file_type="stl")
    except Exception:
        pass

    # Pure-Python fallback parser for ASCII STL exported by OpenSCAD.
    text = raw.decode("utf-8", errors="replace")
    triangles: list[tuple[tuple[float, float, float], list[tuple[float, float, float]]]] = []
    normal = (0.0, 0.0, 0.0)
    verts: list[tuple[float, float, float]] = []
    for line in text.splitlines():
        parts = line.strip().split()
        if not parts:
            continue
        if parts[0] == "facet" and len(parts) >= 5 and parts[1] == "normal":
            normal = (float(parts[2]), float(parts[3]), float(parts[4]))
            verts = []
        elif parts[0] == "vertex" and len(parts) >= 4:
            verts.append((float(parts[1]), float(parts[2]), float(parts[3])))
        elif parts[0] == "endfacet" and len(verts) == 3:
            triangles.append((normal, verts))
            verts = []

    header = f"Binary STL converted from {stl_path.name}".encode("ascii")[:80].ljust(80, b"\x00")
    out = bytearray(header)
    out.extend(struct.pack("<I", len(triangles)))
    for n, (v0, v1, v2) in triangles:
        out.extend(struct.pack("<12fH", *n, *v0, *v1, *v2, 0))
    return bytes(out)


def find_node_binary() -> str:
    """Locate a working Node binary for running esbuild."""
    import shutil

    candidates = [
        os.environ.get("NODE_BIN"),
        shutil.which("node"),
        "/opt/homebrew/bin/node",
        "/usr/local/bin/node",
    ]
    for cand in candidates:
        if not cand:
            continue
        try:
            proc = subprocess.run([cand, "-v"], capture_output=True, text=True, check=False)
            if proc.returncode == 0:
                return cand
        except FileNotFoundError:
            continue
    raise RuntimeError("No working node runtime found to run esbuild.")


def bundle_js() -> str:
    """Bundle source/assembly.js with local esbuild."""
    esbuild_native = SOURCE_DIR / "node_modules" / "esbuild" / "bin" / "esbuild"
    esbuild_bin = SOURCE_DIR / "node_modules" / ".bin" / "esbuild"
    env = os.environ.copy()
    if esbuild_native.exists() and os.access(esbuild_native, os.X_OK):
        cmd = [
            str(esbuild_native),
            str(SOURCE_DIR / "assembly.js"),
            "--bundle",
            "--minify",
            "--format=iife",
            f"--outfile={BUNDLE_PATH}",
        ]
    elif esbuild_bin.exists():
        node = find_node_binary()
        env["PATH"] = f"{Path(node).parent}:{env.get('PATH', '')}"
        cmd = [
            node,
            str(esbuild_bin),
            str(SOURCE_DIR / "assembly.js"),
            "--bundle",
            "--minify",
            "--format=iife",
            f"--outfile={BUNDLE_PATH}",
        ]
    elif BUNDLE_PATH.is_file():
        return BUNDLE_PATH.read_text(encoding="utf-8")
    elif (DELIVERABLE_DIR / "index.html").is_file():
        existing = (DELIVERABLE_DIR / "index.html").read_text(encoding="utf-8")
        marker = '<script id="assets" type="application/json">'
        a_idx = existing.index(marker)
        s_start = existing.index("<script>", a_idx) + len("<script>")
        s_end = existing.rindex("</script>")
        return existing[s_start:s_end]
    else:
        raise FileNotFoundError("esbuild not found in source/node_modules and no prebuilt bundle present")

    subprocess.run(cmd, cwd=SOURCE_DIR, env=env, check=True)
    return BUNDLE_PATH.read_text(encoding="utf-8")


def build() -> Path:
    bundle_text = bundle_js()
    meshes: dict[str, str] = {}
    for name in INCLUDED_MESHES:
        stl_path = MODEL_DIR / f"{name}.stl"
        if not stl_path.exists():
            raise FileNotFoundError(f"Missing STL mesh: {stl_path}")
        binary_stl = ascii_stl_to_binary(stl_path)
        meshes[name] = base64.b64encode(binary_stl).decode("ascii")

    params = json.loads((MODEL_DIR / "parameters.json").read_text(encoding="utf-8"))
    insertion_path = DESIGN_DIR / "verification" / "independent-insertion.json"
    if not insertion_path.exists():
        insertion_path = ASSEMBLY_DIR / "reference" / "insertion.json"
    insertion = json.loads(insertion_path.read_text(encoding="utf-8")) if insertion_path.exists() else {}

    payload = {
        "meshes": meshes,
        "excludedMeshes": EXCLUDED_MESHES,
        "parameters": params,
        "insertionSummary": {
            "sampleCount": len(insertion.get("samples", [])),
            "worstMm3": insertion.get("worst", {}).get("rigid_interference_mm3"),
            "worstOffsetMm": insertion.get("worst", {}).get("offset_mm"),
            "seatedOverlapMm3": insertion.get("seated_full_stack_overlap_mm3"),
            "scope": insertion.get("scope", ""),
        },
    }

    payload_json = json.dumps(payload, separators=(",", ":")).replace("</script", "<\\/script")
    safe_script = bundle_text.replace("</script", "<\\/script")

    template = TEMPLATE_PATH.read_text(encoding="utf-8")
    html = template.replace("__ASSETS__", payload_json).replace("__SCRIPT__", safe_script)

    DELIVERABLE_DIR.mkdir(parents=True, exist_ok=True)
    out_html = DELIVERABLE_DIR / "index.html"
    out_html.write_text(html, encoding="utf-8")
    return out_html


if __name__ == "__main__":
    output = build()
    print(f"Built {output} ({output.stat().st_size:,} bytes)")
