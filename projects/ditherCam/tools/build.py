#!/usr/bin/env python3
"""Reproducible builder for RlcdCamera and SenseCamera ESP32-S3 firmware targets."""
from __future__ import annotations

import json
import os
import shutil
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def find_arduino_cli() -> str:
    """Locate arduino-cli from env override or PATH."""
    for env_key in ("FLIPCAM_ARDUINO_CLI", "ARDUINO_CLI"):
        val = os.environ.get(env_key)
        if val:
            return val
    found = shutil.which("arduino-cli")
    if found:
        return found
    raise FileNotFoundError(
        "arduino-cli not found on PATH; install Arduino CLI or set ARDUINO_CLI"
    )


def find_gen_esp32part(arduino_cli: str) -> Path:
    """Locate gen_esp32part.py from env override or arduino-cli data directory."""
    env_override = os.environ.get("FLIPCAM_GEN_ESP32PART")
    if env_override:
        p = Path(env_override).expanduser()
        if p.is_file():
            return p
        raise FileNotFoundError(f"FLIPCAM_GEN_ESP32PART does not exist: {p}")

    proc = subprocess.run(
        [arduino_cli, "config", "get", "directories.data", "--format", "json"],
        capture_output=True,
        text=True,
        check=False,
    )
    if proc.returncode == 0 and proc.stdout.strip():
        data_dir = Path(json.loads(proc.stdout))
    else:
        dump = subprocess.run(
            [arduino_cli, "config", "dump", "--format", "json"],
            capture_output=True,
            text=True,
            check=False,
        )
        if dump.returncode != 0:
            raise RuntimeError(
                f"Failed to query arduino-cli config ({dump.returncode}): {dump.stderr.strip()}"
            )
        cfg = json.loads(dump.stdout)
        dirs = cfg.get("config", {}).get("directories", {})
        if "data" not in dirs:
            raise RuntimeError("arduino-cli config did not report directories.data; set FLIPCAM_GEN_ESP32PART")
        data_dir = Path(dirs["data"])
    preferred = data_dir / "packages" / "esp32" / "hardware" / "esp32" / "3.3.10" / "tools" / "gen_esp32part.py"
    if preferred.is_file():
        return preferred
    candidates = sorted(
        (data_dir / "packages" / "esp32" / "hardware" / "esp32").glob("*/tools/gen_esp32part.py")
    )
    if candidates:
        return candidates[-1]
    raise FileNotFoundError(
        f"gen_esp32part.py not found under {data_dir / 'packages/esp32/hardware/esp32'} "
        "(expected esp32 core 3.3.10 or compatible)"
    )

TARGETS = {
    "rlcd": {
        "sketch": ROOT / "firmware" / "RlcdCamera",
        "fqbn": "esp32:esp32:esp32s3:FlashSize=16M,FlashMode=dio,PSRAM=opi,CDCOnBoot=cdc,USBMode=hwcdc",
        "slot_limit": 0x330000,
        "part_csv": ROOT / "partitions" / "rlcd_16MB_media.csv",
        "bin_name": "RlcdCamera.ino.bin",
    },
    "xiao": {
        "sketch": ROOT / "firmware" / "SenseCamera",
        "fqbn": "esp32:esp32:XIAO_ESP32S3:FlashSize=8M,FlashMode=dio,PSRAM=opi,USBMode=hwcdc",
        "slot_limit": 0x1C0000,
        "part_csv": ROOT / "partitions" / "xiao_8MB_media.csv",
        "bin_name": "SenseCamera.ino.bin",
    },
}


def verify_esp32s3_app_bin(bin_path: Path, limit: int, label: str) -> int:
    data = bin_path.read_bytes()
    if len(data) == 0 or len(data) > limit:
        raise RuntimeError(
            f"{label}: app binary size {len(data)} (0x{len(data):x}) exceeds slot limit 0x{limit:x}"
        )
    if data[0] != 0xE9:
        raise RuntimeError(f"{label}: invalid ESP32 image magic 0x{data[0]:02x} (expected 0xE9)")
    chip_id = struct.unpack_from("<H", data, 12)[0]
    if chip_id != 9:
        raise RuntimeError(f"{label}: invalid chip_id {chip_id} (expected 9 for ESP32-S3)")
    return len(data)


def build_target(
    key: str,
    cfg: dict,
    arduino_cli: str,
    gen_part: Path,
    extra_include_dir: Path | None = None,
) -> dict:
    out_dir = ROOT / "artifacts" / key
    build_dir = ROOT / "build" / key
    out_dir.mkdir(parents=True, exist_ok=True)
    build_dir.mkdir(parents=True, exist_ok=True)

    # Ensure sketch-local partitions.csv matches canonical partitions/ CSV
    shutil.copy2(cfg["part_csv"], cfg["sketch"] / "partitions.csv")

    cmd = [
        arduino_cli,
        "compile",
        "--fqbn",
        cfg["fqbn"],
        "--build-path",
        str(build_dir),
        "--output-dir",
        str(out_dir),
        "--warnings",
        "all",
    ]
    inc_flags = []
    if extra_include_dir and extra_include_dir.is_dir():
        inc_flags.append(f"-I{extra_include_dir}")
    inc_flags.append(f"-I{ROOT / 'shared'}")
    inc_flags.append(f"-I{ROOT / 'reference'}")
    extra_inc = " ".join(inc_flags)
    cmd.extend(["--build-property", f"compiler.cpp.extra_flags={extra_inc}"])
    cmd.extend(["--build-property", f"compiler.c.extra_flags={extra_inc}"])
    cmd.append(str(cfg["sketch"]))

    proc = subprocess.run(
        cmd,
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=False,
    )
    log_path = out_dir / "build.log"
    log_path.write_text(proc.stdout)
    if proc.returncode != 0:
        print(proc.stdout)
        raise RuntimeError(f"Build failed for {key} (exit {proc.returncode})")

    app_bin = out_dir / cfg["bin_name"]
    if not app_bin.is_file():
        raise RuntimeError(f"Missing expected app binary: {app_bin}")
    size = verify_esp32s3_app_bin(app_bin, cfg["slot_limit"], key)

    # Also compile standalone partition binary for coordinator inspection
    part_bin = out_dir / f"{key}_partitions.bin"
    subprocess.run(
        [sys.executable, str(gen_part), "-q", str(cfg["part_csv"]), str(part_bin)],
        check=True,
    )

    print(
        f"[{key}] OK: {app_bin.relative_to(ROOT)} = {size} bytes "
        f"(limit {cfg['slot_limit']} bytes, {100.0 * size / cfg['slot_limit']:.1f}%)"
    )
    return {
        "app_bin": str(app_bin.relative_to(ROOT)),
        "build_log": str(log_path.relative_to(ROOT)),
    }


def main() -> int:
    arduino_cli = find_arduino_cli()
    gen_part = find_gen_esp32part(arduino_cli)
    private_dir_env = os.environ.get("FLIPCAM_PRIVATE_INC")
    private_dir = Path(private_dir_env).resolve() if private_dir_env else None
    artifacts = {}
    for key, cfg in TARGETS.items():
        artifacts[key] = build_target(key, cfg, arduino_cli, gen_part, private_dir)
    (ROOT / "artifacts.json").write_text(json.dumps(artifacts, indent=2) + "\n")
    print("Wrote artifacts.json successfully.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
