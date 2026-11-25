#!/usr/bin/env python3
"""Stage and build M5Stack Tab5 0.2.0 Camera Eye + Copy-Face + Eyes1 firmware offline using ESP-IDF 5.4.2."""
import argparse
import hashlib
import json
import os
import shutil
import subprocess
from pathlib import Path

REQUIRED_SDKCONFIG = {
    'CONFIG_IDF_TARGET': '"esp32p4"',
    'CONFIG_ESPTOOLPY_FLASHMODE_DIO': 'y',
    'CONFIG_ESPTOOLPY_FLASHFREQ_80M': 'y',
    'CONFIG_ESPTOOLPY_FLASHSIZE_16MB': 'y',
    'CONFIG_ESP32P4_REV_MIN_1': 'y',
    'CONFIG_PARTITION_TABLE_CUSTOM': 'y',
    'CONFIG_PARTITION_TABLE_OFFSET': '0x8000',
    'CONFIG_SPIRAM_SPEED_200M': 'y',
    'CONFIG_USJ_ENABLE_USB_SERIAL_JTAG': 'y',
    'CONFIG_CAMERA_SC202CS': 'y',
    'CONFIG_ESP_VIDEO_ENABLE_MIPI_CSI_VIDEO_DEVICE': 'y',
    'CONFIG_HUMAN_FACE_DETECT_MODEL_IN_FLASH_RODATA': 'y',
}

APP_SLOT_OFFSET = 0x10000
APP_SLOT_CAPACITY = 0x300000


def find_cached_component(cache_dir: Path, prefix: str) -> Path:
    matches = sorted(p for p in cache_dir.iterdir() if p.is_dir() and p.name.startswith(prefix))
    if not matches:
        raise FileNotFoundError(f'Missing cached component {prefix}* under {cache_dir}')
    return matches[-1]


def sync_tree(src: Path, dst: Path) -> None:
    dst.mkdir(parents=True, exist_ok=True)
    for item in src.rglob('*'):
        rel = item.relative_to(src)
        target = dst / rel
        if item.is_dir():
            target.mkdir(parents=True, exist_ok=True)
        elif item.is_file():
            data = item.read_bytes()
            if not target.exists() or target.read_bytes() != data:
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(data)


def stage_project(project_root: Path, stage_dir: Path, cache_dir: Path) -> dict:
    sync_tree(project_root / 'firmware', stage_dir)
    sync_tree(project_root / 'adapter', stage_dir / 'main' / 'adapter')

    # Stage shared SoundWake.h from sibling animatronic-eyes on main.
    shared_dst = stage_dir / 'main' / 'shared'
    shared_dst.mkdir(parents=True, exist_ok=True)
    sound_wake_src = project_root.parent / 'animatronic-eyes' / 'shared' / 'SoundWake.h'
    shutil.copy2(sound_wake_src, shared_dst / 'SoundWake.h')

    comp_dst = stage_dir / 'components'
    comp_dst.mkdir(parents=True, exist_ok=True)
    component_map = {
        'cmake_utilities': 'espressif__cmake_utilities_0.5.3_',
        'esp_sccb_intf': 'espressif__esp_sccb_intf_1.0.0_',
        'esp_cam_sensor': 'espressif__esp_cam_sensor_2.0.1_',
        'esp_ipa': 'espressif__esp_ipa_1.3.1_',
        'esp_video': 'espressif__esp_video_2.0.0_',
        'dl_fft': 'espressif__dl_fft_0.8.0_',
        'esp_new_jpeg': 'espressif__esp_new_jpeg_1.0.2_',
        'esp-dl': 'espressif__esp-dl_3.3.15_',
        'human_face_detect': 'espressif__human_face_detect_0.5.2_',
        'esp_lcd_ili9881c': 'espressif__esp_lcd_ili9881c_1.1.0_',
        'esp_lcd_st7123': 'espressif__esp_lcd_st7123_1.0.2_',
        'esp_lcd_touch': 'espressif__esp_lcd_touch_1.2.1_',
        'esp_lcd_touch_gt911': 'espressif__esp_lcd_touch_gt911_1.2.0',
        'esp_lcd_touch_st7123': 'espressif__esp_lcd_touch_st7123_1.0.2_',
        'esp_codec_dev': 'espressif__esp_codec_dev_1.5.10_',
    }
    resolved = {}
    for target_name, prefix in component_map.items():
        src = find_cached_component(cache_dir, prefix)
        target_comp = comp_dst / target_name
        if not target_comp.exists():
            shutil.copytree(src, target_comp)
        resolved[target_name] = src.name

    # In offline IDF_COMPONENT_MANAGER=0 builds, idf_component.yml dependencies are not auto-injected
    # into touch sub-drivers that declare only REQUIRES "esp_lcd" in CMakeLists.txt.
    for touch_drv in ('esp_lcd_touch_gt911', 'esp_lcd_touch_st7123'):
        cmake_path = comp_dst / touch_drv / 'CMakeLists.txt'
        if cmake_path.is_file():
            txt = cmake_path.read_text()
            if 'REQUIRES "esp_lcd"' in txt and 'esp_lcd_touch' not in txt.split('REQUIRES')[1]:
                cmake_path.write_text(
                    txt.replace('REQUIRES "esp_lcd"', 'REQUIRES "esp_lcd" "esp_lcd_touch"')
                )

    metadata = json.loads((project_root / 'firmware' / 'metadata.json').read_text())
    digest = hashlib.sha256()
    sources = list((project_root / 'firmware').rglob('*')) + list((project_root / 'adapter').rglob('*'))
    for src in sorted(p for p in sources if p.is_file()):
        digest.update(src.relative_to(project_root).as_posix().encode())
        digest.update(src.read_bytes())
    metadata['source_sha256'] = digest.hexdigest()
    metadata['components'] = resolved
    return metadata


def verify_app_image_header(bin_path: Path) -> dict:
    from esptool.bin_image import LoadFirmwareImage

    data = bin_path.read_bytes()
    if not (32 < len(data) <= APP_SLOT_CAPACITY):
        raise RuntimeError(f'Binary size {len(data)} outside slot capacity {APP_SLOT_CAPACITY}')
    im = LoadFirmwareImage('esp32p4', str(bin_path))
    im.verify()
    if im.chip_id != 18:
        raise RuntimeError(f'Expected ESP32-P4 chip_id 18, got {im.chip_id}')
    if im.flash_mode != 2:
        raise RuntimeError(f'Expected DIO flash_mode 2, got {im.flash_mode}')
    if im.flash_size_freq != 0x4F:
        raise RuntimeError(f'Expected flash_size_freq 0x4F, got 0x{im.flash_size_freq:02x}')
    if im.calculate_checksum() != im.checksum:
        raise RuntimeError('Image checksum mismatch')
    if not im.append_digest or im.stored_digest != im.calc_digest:
        raise RuntimeError('Image SHA-256 digest mismatch')
    if not (im.min_rev_full <= 103 <= im.max_rev_full):
        raise RuntimeError(
            f'Silicon rev 1.3 (103) outside [{im.min_rev_full}, {im.max_rev_full}]'
        )
    return {
        'chip_id': im.chip_id,
        'flash_mode': im.flash_mode,
        'flash_size_freq': f'0x{im.flash_size_freq:02x}',
        'min_rev_full': im.min_rev_full,
        'max_rev_full': im.max_rev_full,
        'image_bytes': len(data),
        'image_sha256': hashlib.sha256(data).hexdigest(),
        'slot_offset': f'0x{APP_SLOT_OFFSET:x}',
        'slot_capacity': f'0x{APP_SLOT_CAPACITY:x}',
    }


def main() -> int:
    if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
        raise RuntimeError('Optimized Python prohibited')

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work', type=Path, required=True, help='Build work directory outside Git')
    parser.add_argument('--idf-path', type=Path, required=True, help='Path to ESP-IDF 5.4.2')
    parser.add_argument('--idf-tools-path', type=Path, required=True, help='Path to IDF_TOOLS_PATH')
    parser.add_argument(
        '--component-cache',
        type=Path,
        default=Path.home() / 'Library/Caches/Espressif/ComponentManager/service_d92d8f1e',
    )
    args = parser.parse_args()

    project_root = Path(__file__).resolve().parents[1]
    repo_root = project_root.parents[1]
    work = args.work.resolve()
    if work == repo_root or repo_root in work.parents:
        parser.error('Use a private build directory outside Git')

    stage = work / 'source'
    build = work / 'build'
    # Force cleansdkconfig regeneration when sdkconfig.defaults changes
    if (stage / 'sdkconfig').is_file():
        (stage / 'sdkconfig').unlink()
    metadata = stage_project(project_root, stage, args.component_cache)

    env = os.environ.copy()
    env['DEVELOPER_DIR'] = '/Library/Developer/CommandLineTools'
    env['IDF_PATH'] = str(args.idf_path.resolve())
    env['IDF_TOOLS_PATH'] = str(args.idf_tools_path.resolve())
    env['IDF_COMPONENT_MANAGER'] = '0'

    export_cmd = (
        f'. "{args.idf_path.resolve()}/export.sh" >/dev/null 2>&1 && '
        f'idf.py -C "{stage}" -B "{build}" build'
    )
    subprocess.run(['/bin/bash', '-c', export_cmd], env=env, check=True)

    sdkconfig_map = {}
    for line in (stage / 'sdkconfig').read_text().splitlines():
        if line.startswith('CONFIG_') and '=' in line:
            k, v = line.split('=', 1)
            sdkconfig_map[k] = v
    for k, expected in REQUIRED_SDKCONFIG.items():
        if sdkconfig_map.get(k) != expected:
            raise RuntimeError(f'Unexpected {k}: {sdkconfig_map.get(k)} != {expected}')

    app_bin = build / 'cam_eye.bin'
    if not app_bin.is_file():
        raise FileNotFoundError(f'Missing built binary: {app_bin}')

    try:
        image_info = verify_app_image_header(app_bin)
    except ImportError:
        py_envs = list((args.idf_tools_path / 'python_env').glob('*/bin/python'))
        if not py_envs:
            raise
        probe = (
            'import json,sys; '
            'from pathlib import Path; '
            'sys.path.insert(0, sys.argv[1]); '
            'import build as b; '
            'print(json.dumps(b.verify_app_image_header(Path(sys.argv[2]))))'
        )
        out = subprocess.check_output(
            [str(py_envs[0]), '-c', probe, str(Path(__file__).resolve().parent), str(app_bin)],
            text=True,
        )
        image_info = json.loads(out)

    metadata['build'] = image_info
    metadata['idf_version'] = '5.4.2'
    (work / 'build-record.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print(json.dumps({'status': 'BUILT', 'binary': str(app_bin), **image_info}, indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
