#!/usr/bin/env python3
"""Host acceptance test runner: compiles & runs C++ logic tests + partition & security checks."""
from __future__ import annotations

import csv
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD_BIN = ROOT / "build" / "host_tests"


def parse_partitions(csv_path: Path) -> list[dict]:
    rows = []
    for raw in csv_path.read_text().splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        parts = [p.strip() for p in next(csv.reader([line]))]
        name, ptype, subtype, offset_s, size_s = parts[:5]
        rows.append(
            {
                "name": name,
                "type": ptype,
                "subtype": subtype,
                "offset": int(offset_s, 0),
                "size": int(size_s, 0),
            }
        )
    return rows


def check_partitions() -> None:
    rlcd = parse_partitions(ROOT / "partitions" / "rlcd_16MB_media.csv")
    expected_rlcd = [
        ("nvs", 0x9000, 0x5000),
        ("otadata", 0xE000, 0x2000),
        ("app0", 0x10000, 0x330000),
        ("app1", 0x340000, 0x330000),
        ("spiffs", 0x670000, 0x180000),
        ("coredump", 0x7F0000, 0x10000),
        ("media", 0x800000, 0x800000),
    ]
    assert [(r["name"], r["offset"], r["size"]) for r in rlcd] == expected_rlcd, rlcd
    assert rlcd[-1]["offset"] + rlcd[-1]["size"] == 16 * 1024 * 1024

    xiao = parse_partitions(ROOT / "partitions" / "xiao_8MB_media.csv")
    expected_xiao = [
        ("otadata", 0x9000, 0x2000),
        ("phy_init", 0xB000, 0x1000),
        ("app0", 0x10000, 0x1C0000),
        ("app1", 0x1D0000, 0x1C0000),
        ("eeprom", 0x390000, 0x1000),
        ("spiffs", 0x391000, 0xF000),
        ("nvs", 0x3A0000, 0x60000),
        ("media", 0x400000, 0x3F0000),
    ]
    assert [(r["name"], r["offset"], r["size"]) for r in xiao] == expected_xiao, xiao
    # Final 64 KiB (0x7f0000..0x800000) must remain untouched
    assert xiao[-1]["offset"] + xiao[-1]["size"] == 0x7F0000

    # Confirm sketch copies match canonical partitions/
    assert (ROOT / "firmware" / "RlcdCamera" / "partitions.csv").read_text() == (
        ROOT / "partitions" / "rlcd_16MB_media.csv"
    ).read_text()
    assert (ROOT / "firmware" / "SenseCamera" / "partitions.csv").read_text() == (
        ROOT / "partitions" / "xiao_8MB_media.csv"
    ).read_text()


def check_no_real_secrets_or_private_paths() -> None:
    ex_cfg = (ROOT / "shared" / "CompileConfig.example.h").read_text()
    assert "FLIPCAM_PAIR_CONFIG_IS_DUMMY 1" in ex_cfg
    assert "example" in ex_cfg.lower()
    # Ensure no privatePairConfig.h or private-localPairConfig.h committed inside public tree
    assert not (ROOT / "shared" / "privatePairConfig.h").exists()
    assert not (ROOT / "shared" / "private-localPairConfig.h").exists()


def run_cpp_host_tests() -> None:
    BUILD_BIN.parent.mkdir(parents=True, exist_ok=True)
    clt_clang = Path("/Library/Developer/CommandLineTools/usr/bin/clang++")
    clt_cc = Path("/Library/Developer/CommandLineTools/usr/bin/clang")
    clt_sdk = Path("/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk")
    cxx = str(clt_clang) if clt_clang.is_file() else "g++"
    cc = str(clt_cc) if clt_cc.is_file() else "gcc"
    env = dict(**__import__("os").environ)
    c_obj = BUILD_BIN.parent / "softjpgd.o"
    c_cmd = [
        cc,
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-O2",
        "-c",
        f"-I{ROOT / 'shared'}",
        str(ROOT / "shared" / "FlipCamSoftJpgd.c"),
        "-o",
        str(c_obj),
    ]
    cmd = [
        cxx,
        "-std=c++17",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-O2",
    ]
    if clt_clang.is_file() and clt_sdk.is_dir():
        env["DEVELOPER_DIR"] = "/Library/Developer/CommandLineTools"
        c_cmd.extend(["-isysroot", str(clt_sdk)])
        cmd.extend(["-isysroot", str(clt_sdk)])
    subprocess.run(c_cmd, cwd=ROOT, env=env, check=True)
    cmd.extend([
        f"-I{ROOT / 'shared'}",
        str(ROOT / "tests" / "host_tests.cpp"),
        str(c_obj),
        "-o",
        str(BUILD_BIN),
    ])
    subprocess.run(cmd, cwd=ROOT, env=env, check=True)
    subprocess.run([str(BUILD_BIN)], cwd=ROOT, check=True)


def main() -> int:
    check_partitions()
    check_no_real_secrets_or_private_paths()
    run_cpp_host_tests()
    print("tests/run_all.py: ALL HOST CHECKS PASSED.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
