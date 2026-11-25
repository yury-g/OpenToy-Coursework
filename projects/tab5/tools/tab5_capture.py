#!/usr/bin/env python3
"""Host capture client & OTCAM1 binary protocol validator for M5Stack Tab5."""
import argparse
import datetime
import fcntl
import hashlib
import json
import os
import re
import struct
import time
import zlib
from pathlib import Path

PACKET_MAGIC = b'\x89OTCAM1\r'
PACKET_TRAILER = b'END\n'
HEADER_META_BYTES = 32
HEADER_BYTES = 36
MAX_PAYLOAD_BYTES = 2 * 1024 * 1024
DEFAULT_MAX_AGE_MS = 250

FMT_JPEG = 1
FMT_RGB565 = 2

FLAG_LIVE_HARDWARE = 0x01
FLAG_AE_WARMED = 0x02
FLAG_BURST_LAST = 0x04


def encode_frame_header(
    payload_len: int,
    payload_crc32: int,
    sequence: int,
    captured_ms: int,
    sent_ms: int,
    width: int,
    height: int,
    sensor_id: int,
    fmt: int,
    flags: int,
    reserved: int = 0,
) -> bytes:
    meta = struct.pack(
        '<IIIIIHHHBB I',
        payload_len,
        payload_crc32,
        sequence,
        captured_ms,
        sent_ms,
        width,
        height,
        sensor_id,
        fmt,
        flags,
        reserved,
    )
    assert len(meta) == HEADER_META_BYTES
    hcrc = zlib.crc32(meta) & 0xFFFFFFFF
    return meta + struct.pack('<I', hcrc)


def decode_frame_header(raw: bytes) -> dict:
    if len(raw) != HEADER_BYTES:
        raise ValueError(f'Header must be {HEADER_BYTES} bytes, got {len(raw)}')
    meta = raw[:HEADER_META_BYTES]
    (got_hcrc,) = struct.unpack('<I', raw[HEADER_META_BYTES:HEADER_BYTES])
    expected_hcrc = zlib.crc32(meta) & 0xFFFFFFFF
    if got_hcrc != expected_hcrc:
        raise ValueError(f'Header CRC mismatch: 0x{got_hcrc:08x} != 0x{expected_hcrc:08x}')
    (
        payload_len,
        payload_crc32,
        sequence,
        captured_ms,
        sent_ms,
        width,
        height,
        sensor_id,
        fmt,
        flags,
        reserved,
    ) = struct.unpack('<IIIIIHHHBB I', meta)
    if payload_len == 0 or payload_len > MAX_PAYLOAD_BYTES:
        raise ValueError(f'Invalid payload_len={payload_len}')
    if width == 0 or height == 0:
        raise ValueError(f'Invalid dimensions {width}x{height}')
    if fmt not in (FMT_JPEG, FMT_RGB565):
        raise ValueError(f'Unsupported pixel format {fmt}')
    return {
        'payload_len': payload_len,
        'payload_crc32': payload_crc32,
        'sequence': sequence,
        'captured_ms': captured_ms,
        'sent_ms': sent_ms,
        'width': width,
        'height': height,
        'sensor_id': sensor_id,
        'format': fmt,
        'flags': flags,
        'live_hardware': bool(flags & FLAG_LIVE_HARDWARE),
        'ae_warmed': bool(flags & FLAG_AE_WARMED),
        'burst_last': bool(flags & FLAG_BURST_LAST),
    }


def validate_packet_payload(
    hdr: dict,
    payload: bytes,
    seq_before: int,
    max_age_ms: int = DEFAULT_MAX_AGE_MS,
    require_live_hardware: bool = True,
) -> None:
    if len(payload) != hdr['payload_len']:
        raise ValueError('Payload length mismatch')
    got_crc = zlib.crc32(payload) & 0xFFFFFFFF
    if got_crc != hdr['payload_crc32']:
        raise ValueError(f'Payload CRC mismatch: 0x{got_crc:08x} != 0x{hdr["payload_crc32"]:08x}')
    if hdr['sequence'] == 0 or ((hdr['sequence'] - seq_before) & 0xFFFFFFFF) == 0 or ((hdr['sequence'] - seq_before) & 0x80000000):
        raise ValueError(f'Stale or non-advancing sequence {hdr["sequence"]} <= {seq_before}')
    age_ms = (hdr['sent_ms'] - hdr['captured_ms']) & 0xFFFFFFFF
    if age_ms > max_age_ms:
        raise ValueError(f'Stale frame age {age_ms} ms > {max_age_ms} ms')
    if require_live_hardware and not hdr['live_hardware']:
        raise ValueError('Frame lacks live_hardware flag')
    if hdr['format'] == FMT_JPEG:
        if len(payload) < 4 or payload[:2] != b'\xff\xd8' or payload[-2:] != b'\xff\xd9':
            raise ValueError('Invalid JPEG SOI/EOI markers')
    elif hdr['format'] == FMT_RGB565:
        expected = hdr['width'] * hdr['height'] * 2
        if len(payload) != expected:
            raise ValueError(f'Invalid RGB565 length {len(payload)} != {expected}')


def extract_packet_from_buffer(
    buf: bytes,
    seq_before: int,
    max_age_ms: int = DEFAULT_MAX_AGE_MS,
    require_live_hardware: bool = True,
) -> tuple:
    idx = buf.find(PACKET_MAGIC)
    if idx < 0:
        raise ValueError('OTCAM1 packet magic not found')
    pos = idx + len(PACKET_MAGIC)
    if len(buf) < pos + HEADER_BYTES:
        raise ValueError('Truncated OTCAM1 header')
    hdr = decode_frame_header(buf[pos : pos + HEADER_BYTES])
    pos += HEADER_BYTES
    need = pos + hdr['payload_len'] + len(PACKET_TRAILER)
    if len(buf) < need:
        raise ValueError('Truncated OTCAM1 payload/trailer')
    payload = buf[pos : pos + hdr['payload_len']]
    trailer = buf[pos + hdr['payload_len'] : need]
    if trailer != PACKET_TRAILER:
        raise ValueError(f'Invalid OTCAM1 trailer: {trailer!r}')
    validate_packet_payload(
        hdr,
        payload,
        seq_before=seq_before,
        max_age_ms=max_age_ms,
        require_live_hardware=require_live_hardware,
    )
    return hdr, payload, buf[need:]


def parse_kv_line(line: str, prefix: str) -> dict:
    if not line.startswith(prefix):
        raise ValueError(f'Unexpected response line: {line!r}')
    result = {}
    for m in re.finditer(r'(\w+)=("[^"]*"|\S+)', line[len(prefix) :]):
        k, v = m.group(1), m.group(2)
        if v.startswith('"') and v.endswith('"'):
            result[k] = v[1:-1]
        elif v.startswith('0x'):
            result[k] = v
        elif re.fullmatch(r'-?\d+', v):
            result[k] = int(v)
        else:
            result[k] = v
    return result


def parse_status_line(line: str) -> dict:
    return parse_kv_line(line, 'OK STATUS ')


def open_enrolled_tab5(identity_json: Path, lock_path: Path):
    import serial
    from serial.tools.list_ports import comports

    enrolled = json.loads(identity_json.read_text())
    mac = enrolled['mac'].lower()
    lock_fh = lock_path.open('a+')
    fcntl.flock(lock_fh, fcntl.LOCK_EX | fcntl.LOCK_NB)

    matches = [
        p
        for p in comports()
        if p.vid == 0x303A and p.pid == 0x1001 and (p.serial_number or '').lower() == mac
    ]
    if len(matches) != 1:
        lock_fh.close()
        raise RuntimeError(f'Enrolled Tab5 USB JTAG/Serial not uniquely found (matches={len(matches)})')

    ser = serial.Serial()
    ser.port = matches[0].device
    ser.baudrate = 115200
    ser.timeout = 0.2
    ser.write_timeout = 1.0
    ser.dtr = False
    ser.rts = False
    ser.open()
    return ser, lock_fh


def read_line_until(ser, timeout_s: float) -> str:
    deadline = time.monotonic() + timeout_s
    buf = bytearray()
    while time.monotonic() < deadline:
        chunk = ser.read(1)
        if not chunk:
            continue
        if chunk == b'\n':
            text = buf.decode(errors='replace').strip()
            if text:
                return text
            buf.clear()
            continue
        if chunk != b'\r':
            buf.extend(chunk)
    raise TimeoutError(f'Timed out waiting for status line (partial={buf!r})')


def read_exact_stream(ser, count: int, deadline: float) -> bytes:
    out = bytearray()
    while len(out) < count:
        if time.monotonic() > deadline:
            raise TimeoutError(f'Timed out reading {count} bytes (got {len(out)})')
        chunk = ser.read(min(8192, count - len(out)))
        if chunk:
            out.extend(chunk)
    return bytes(out)


def read_next_packet(ser, seq_before: int, timeout_s: float = 3.5) -> tuple:
    deadline = time.monotonic() + timeout_s
    ring = bytearray()
    while time.monotonic() < deadline:
        b = ser.read(1)
        if not b:
            continue
        ring.extend(b)
        if len(ring) > 4096:
            del ring[: len(ring) - 4096]
        if ring.endswith(PACKET_MAGIC):
            raw_hdr = read_exact_stream(ser, HEADER_BYTES, deadline)
            hdr = decode_frame_header(raw_hdr)
            payload = read_exact_stream(ser, hdr['payload_len'], deadline)
            trailer = read_exact_stream(ser, len(PACKET_TRAILER), deadline)
            if trailer != PACKET_TRAILER:
                raise ValueError(f'Invalid packet trailer: {trailer!r}')
            validate_packet_payload(hdr, payload, seq_before=seq_before)
            return hdr, payload
        if ring.endswith(b'\n'):
            line = bytes(ring).decode(errors='replace').strip()
            if line.startswith('ERR '):
                raise RuntimeError(f'Device returned error: {line}')
            ring.clear()
    raise TimeoutError('Timed out waiting for OTCAM1 packet magic')


def query_prefixed_line(ser, cmd_bytes: bytes, prefix: str, timeout_s: float = 3.6) -> dict:
    ser.reset_input_buffer()
    deadline = time.monotonic() + timeout_s
    next_send = 0.0
    while time.monotonic() < deadline:
        now = time.monotonic()
        if now >= next_send:
            ser.write(cmd_bytes)
            ser.flush()
            next_send = now + 0.75
        try:
            line = read_line_until(ser, min(0.75, max(0.15, deadline - time.monotonic())))
        except TimeoutError:
            continue
        if line.startswith(prefix):
            return parse_kv_line(line, prefix)
    raise TimeoutError(f'No {prefix.strip()} line received')


def query_status(ser) -> dict:
    return query_prefixed_line(ser, b'STATUS\n', 'OK STATUS ')


def query_log(ser) -> str:
    ser.reset_input_buffer()
    ser.write(b'LOG\n')
    ser.flush()
    deadline = time.monotonic() + 2.5
    header = ''
    while time.monotonic() < deadline:
        line = read_line_until(ser, max(0.2, deadline - time.monotonic()))
        if line.startswith('OK LOG '):
            header = line
            break
    if not header:
        raise TimeoutError('No OK LOG header received')
    lines = [header]
    while time.monotonic() < deadline:
        line = read_line_until(ser, max(0.2, deadline - time.monotonic()))
        lines.append(line)
        if line == 'END_LOG':
            return '\n'.join(lines)
    raise TimeoutError('Timed out waiting for END_LOG')


def main() -> int:
    if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
        raise RuntimeError('Optimized Python prohibited')

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=['status', 'hw', 'face', 'mic', 'mode', 'log', 'snap', 'burst'])
    parser.add_argument('--identity-json', type=Path, required=True)
    parser.add_argument('--lock-file', type=Path, default=None)
    parser.add_argument('--out-dir', type=Path, default=Path('.'))
    parser.add_argument('--quality', type=int, default=82)
    parser.add_argument('--count', type=int, default=12)
    parser.add_argument('--interval-ms', type=int, default=220)
    parser.add_argument('--name', type=str, default='snap.jpg')
    parser.add_argument('--arg', type=str, default='', help='Argument for mic (0/1) or mode (CAMEYE/COPYFACE/EYES1)')
    parser.add_argument('--settle-ae', action='store_true', help='Wait up to 2s for AE warmup before capture')
    args = parser.parse_args()

    lock_file = args.lock_file or (args.identity_json.parent / 'hardware.lock')
    ser, lock_fh = open_enrolled_tab5(args.identity_json, lock_file)
    try:
        st = query_status(ser)
        if args.command in ('snap', 'burst') and args.settle_ae and st.get('ready') == 1 and st.get('ae') == 0:
            deadline = time.monotonic() + 2.0
            while time.monotonic() < deadline and st.get('ae') == 0:
                time.sleep(0.15)
                st = query_status(ser)
        if args.command == 'status':
            print(json.dumps(st, indent=2))
            return 0
        if args.command == 'hw':
            hw = query_prefixed_line(ser, b'HW\n', 'OK HW ')
            print(json.dumps({'status': st, 'hw': hw}, indent=2))
            return 0
        if args.command == 'face':
            if args.settle_ae:
                time.sleep(0.4)
            fc = query_prefixed_line(ser, b'FACE\n', 'OK FACE ')
            print(json.dumps({'status': st, 'face': fc}, indent=2))
            return 0
        if args.command == 'mic':
            cmd = f'MIC {args.arg}\n'.encode() if args.arg else b'MIC\n'
            mc = query_prefixed_line(ser, cmd, 'OK MIC ')
            print(json.dumps({'status': st, 'mic': mc}, indent=2))
            return 0
        if args.command == 'mode':
            cmd = f'MODE {args.arg}\n'.encode() if args.arg else b'MODE\n'
            md = query_prefixed_line(ser, cmd, 'OK MODE ')
            print(json.dumps({'status': st, 'mode': md}, indent=2))
            return 0
        if args.command == 'log':
            print(json.dumps(st, indent=2))
            print(query_log(ser))
            return 0

        if st.get('ready') != 1 or st.get('stream') != 1 or st.get('sensor') != '0xeb52':
            raise RuntimeError(f'Camera not ready for capture: {st}')

        args.out_dir.mkdir(parents=True, exist_ok=True)
        seq_before = int(st['seq'])

        if args.command == 'snap':
            ser.reset_input_buffer()
            ser.write(f'SNAP {args.quality}\n'.encode())
            ser.flush()
            hdr, payload = read_next_packet(ser, seq_before=seq_before, timeout_s=4.0)
            out_file = args.out_dir / args.name
            out_file.write_bytes(payload)
            prov = {
                'captured_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                'app': st.get('app'),
                'observed_version': st.get('ver'),
                'device_status': st,
                'frame_header': hdr,
                'file': str(out_file),
                'sha256': hashlib.sha256(payload).hexdigest(),
            }
            (args.out_dir / (out_file.stem + '.provenance.json')).write_text(
                json.dumps(prov, indent=2) + '\n'
            )
            print(json.dumps(prov, indent=2))
            return 0

        if args.command == 'burst':
            from PIL import Image
            import io

            ser.reset_input_buffer()
            ser.write(f'BURST {args.count} {args.interval_ms} {args.quality}\n'.encode())
            ser.flush()
            frames_meta = []
            pil_frames = []
            cur_seq = seq_before
            for i in range(args.count):
                hdr, payload = read_next_packet(
                    ser, seq_before=cur_seq, timeout_s=max(3.5, args.interval_ms / 1000.0 + 2.5)
                )
                cur_seq = hdr['sequence']
                fname = args.out_dir / f'burst_{i + 1:02d}.jpg'
                fname.write_bytes(payload)
                pil_frames.append(Image.open(io.BytesIO(payload)).convert('RGB'))
                frames_meta.append(
                    {
                        'index': i + 1,
                        'file': fname.name,
                        'sequence': hdr['sequence'],
                        'captured_ms': hdr['captured_ms'],
                        'sent_ms': hdr['sent_ms'],
                        'bytes': len(payload),
                        'sha256': hashlib.sha256(payload).hexdigest(),
                        'ae_warmed': hdr['ae_warmed'],
                    }
                )
            gif_path = args.out_dir / 'burst_clip.gif'
            if pil_frames:
                pil_frames[0].save(
                    gif_path,
                    save_all=True,
                    append_images=pil_frames[1:],
                    duration=args.interval_ms,
                    loop=0,
                )
            prov = {
                'captured_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                'app': st.get('app'),
                'observed_version': st.get('ver'),
                'device_status': st,
                'count': len(frames_meta),
                'interval_ms': args.interval_ms,
                'gif': str(gif_path),
                'frames': frames_meta,
            }
            (args.out_dir / 'burst.provenance.json').write_text(json.dumps(prov, indent=2) + '\n')
            print(json.dumps(prov, indent=2))
            return 0
    finally:
        try:
            ser.write(b'STOP\n')
            ser.flush()
        except Exception:
            pass
        ser.close()
        lock_fh.close()
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
