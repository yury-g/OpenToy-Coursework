#!/usr/bin/env python3
"""Host evaluator and bounded retention helper for Vibe Coding Camera Eye evidence."""
import argparse
import hashlib
import json
import os
import shutil
from pathlib import Path

VALID_CLAIMS = {
    'static_visual',
    'motion_animation',
    'touch_physical',
    'audio_acoustic',
    'electrical_charging',
}

NON_VISUAL_CLAIMS = {
    'touch_physical',
    'audio_acoustic',
    'electrical_charging',
}


def analyze_image_metrics(image_path: Path, crop_box=None) -> dict:
    from PIL import Image, ImageChops, ImageStat

    img = Image.open(image_path).convert('RGB')
    if crop_box is not None:
        img = img.crop(tuple(crop_box))
    w, h = img.size
    if w == 0 or h == 0:
        return {
            'width': w,
            'height': h,
            'mean_luma': 0.0,
            'std_luma': 0.0,
            'saturated_frac': 0.0,
            'blank_or_saturated': True,
            'glare_excessive': False,
        }
    gray = img.convert('L')
    stat = ImageStat.Stat(gray)
    hist = gray.histogram()
    total = float(w * h)
    sat_frac = sum(hist[250:]) / total
    dark_frac = sum(hist[:5]) / total
    mean_luma = stat.mean[0]
    std_luma = stat.stddev[0]

    # Approximate sharpness via high-frequency neighbor difference.
    if w > 2 and h > 2:
        dx = ImageChops.difference(gray.crop((0, 0, w - 1, h)), gray.crop((1, 0, w, h)))
        edge_mean = ImageStat.Stat(dx).mean[0]
    else:
        edge_mean = 0.0

    blank_or_sat = (std_luma < 2.0) or (dark_frac > 0.995) or (sat_frac > 0.98)
    glare_excessive = (not blank_or_sat) and (sat_frac > 0.70 or (std_luma > 3.0 and edge_mean < 0.8))
    return {
        'width': w,
        'height': h,
        'mean_luma': round(mean_luma, 2),
        'std_luma': round(std_luma, 2),
        'saturated_frac': round(sat_frac, 4),
        'dark_frac': round(dark_frac, 4),
        'edge_mean': round(edge_mean, 2),
        'blank_or_saturated': blank_or_sat,
        'glare_excessive': glare_excessive,
    }


def evaluate_capture(
    provenance_path: Path,
    claim: str = 'static_visual',
    target_id: str = '',
    intended_version: str = '',
    max_fresh_age_ms: int = 250,
    crop_box=None,
    clipped_bounds: bool = False,
    inverted_or_corrupt: bool = False,
) -> dict:
    if claim not in VALID_CLAIMS:
        raise ValueError(f'Unsupported claim kind: {claim}')

    base_verdict = {
        'target_id': target_id,
        'claim': claim,
        'intended_version': intended_version,
        'observed_version': '',
        'provenance_file': str(provenance_path),
    }

    if claim in NON_VISUAL_CLAIMS:
        return {
            **base_verdict,
            'grade': 'INCONCLUSIVE',
            'reason': 'pixels cannot establish touch, audio, electrical, or charging claims',
        }

    if not provenance_path or not provenance_path.is_file():
        return {
            **base_verdict,
            'grade': 'INCONCLUSIVE',
            'reason': 'missing provenance metadata file',
        }

    prov = json.loads(provenance_path.read_text())
    observed_version = str(prov.get('observed_version') or '')
    base_verdict['observed_version'] = observed_version

    if prov.get('simulated', False):
        if prov.get('claimed_live', True):
            return {
                **base_verdict,
                'grade': 'FAIL',
                'reason': 'simulated scene presented as live hardware evidence',
            }
        return {
            **base_verdict,
            'grade': 'INCONCLUSIVE',
            'reason': 'labelled simulation cannot pass live hardware checks',
        }

    if intended_version and not observed_version:
        return {
            **base_verdict,
            'grade': 'INCONCLUSIVE',
            'reason': 'observed firmware version unavailable in provenance',
        }
    if intended_version and observed_version and intended_version != observed_version:
        return {
            **base_verdict,
            'grade': 'FAIL',
            'reason': f'observed version {observed_version} mismatches intended {intended_version}',
        }

    is_burst = 'frames' in prov
    if not is_burst:
        hdr = prov.get('frame_header')
        img_file = Path(prov.get('file', ''))
        if not hdr or not img_file.is_file():
            return {
                **base_verdict,
                'grade': 'INCONCLUSIVE',
                'reason': 'missing frame header or referenced image file',
            }
        actual_sha = hashlib.sha256(img_file.read_bytes()).hexdigest()
        if prov.get('sha256') and actual_sha != prov['sha256']:
            return {
                **base_verdict,
                'grade': 'FAIL',
                'reason': 'image SHA-256 does not match provenance digest',
            }
        if not hdr.get('live_hardware', False):
            return {
                **base_verdict,
                'grade': 'INCONCLUSIVE',
                'reason': 'frame lacks live_hardware flag',
            }
        age_ms = (int(hdr['sent_ms']) - int(hdr['captured_ms'])) & 0xFFFFFFFF
        if age_ms > max_fresh_age_ms:
            return {
                **base_verdict,
                'grade': 'INCONCLUSIVE',
                'reason': f'stale frame age {age_ms} ms > {max_fresh_age_ms} ms',
            }

        metrics = analyze_image_metrics(img_file, crop_box=crop_box)
        if clipped_bounds or inverted_or_corrupt or metrics['blank_or_saturated']:
            return {
                **base_verdict,
                'grade': 'FAIL',
                'reason': 'visual defect: clipped bounds, blank/saturated, or corrupt image',
                'metrics': metrics,
            }
        if metrics['glare_excessive']:
            return {
                **base_verdict,
                'grade': 'INCONCLUSIVE',
                'reason': 'excessive glare or blur obscures target',
                'metrics': metrics,
            }
        if claim == 'motion_animation':
            return {
                **base_verdict,
                'grade': 'INCONCLUSIVE',
                'reason': 'single still cannot prove motion or smoothness',
                'metrics': metrics,
            }
        return {
            **base_verdict,
            'grade': 'PASS',
            'reason': 'verified fresh live visual still',
            'age_ms': age_ms,
            'sequence': hdr['sequence'],
            'metrics': metrics,
        }

    # Burst / clip evaluation
    frames = prov.get('frames', [])
    if len(frames) < 3 and claim == 'motion_animation':
        return {
            **base_verdict,
            'grade': 'INCONCLUSIVE',
            'reason': f'only {len(frames)} frame(s); >=3 frames required for motion claim',
        }
    if not frames:
        return {
            **base_verdict,
            'grade': 'INCONCLUSIVE',
            'reason': 'burst contains zero frames',
        }

    base_dir = provenance_path.parent
    prev_seq = None
    distinct_hashes = set()
    frame_metrics = []
    max_age = 0
    for fr in frames:
        fpath = Path(fr['file'])
        if not fpath.is_absolute():
            fpath = base_dir / fpath
        if not fpath.is_file():
            return {
                **base_verdict,
                'grade': 'INCONCLUSIVE',
                'reason': f'missing burst frame {fpath.name}',
            }
        raw = fpath.read_bytes()
        fsha = hashlib.sha256(raw).hexdigest()
        if fr.get('sha256') and fsha != fr['sha256']:
            return {
                **base_verdict,
                'grade': 'FAIL',
                'reason': f'SHA-256 mismatch on burst frame {fpath.name}',
            }
        seq = int(fr['sequence'])
        if prev_seq is not None and seq <= prev_seq:
            return {
                **base_verdict,
                'grade': 'INCONCLUSIVE',
                'reason': f'non-advancing frame sequence {seq} <= {prev_seq}',
            }
        prev_seq = seq
        age_ms = (int(fr['sent_ms']) - int(fr['captured_ms'])) & 0xFFFFFFFF
        max_age = max(max_age, age_ms)
        if age_ms > max_fresh_age_ms:
            return {
                **base_verdict,
                'grade': 'INCONCLUSIVE',
                'reason': f'stale burst frame age {age_ms} ms > {max_fresh_age_ms} ms',
            }
        m = analyze_image_metrics(fpath, crop_box=crop_box)
        if m['blank_or_saturated']:
            return {
                **base_verdict,
                'grade': 'FAIL',
                'reason': f'blank or saturated image in burst frame {fpath.name}',
                'metrics': m,
            }
        frame_metrics.append(m)
        # Hash cropped pixels so static room background does not mask a frozen screen crop
        from PIL import Image

        crop_img = Image.open(fpath).convert('RGB')
        if crop_box is not None:
            crop_img = crop_img.crop(tuple(crop_box))
        distinct_hashes.add(hashlib.sha256(crop_img.tobytes()).hexdigest())

    if clipped_bounds or inverted_or_corrupt:
        return {
            **base_verdict,
            'grade': 'FAIL',
            'reason': 'visual defect: clipped bounds or inverted/corrupt crop',
        }
    if all(m['glare_excessive'] for m in frame_metrics):
        return {
            **base_verdict,
            'grade': 'INCONCLUSIVE',
            'reason': 'excessive glare or blur across all frames',
        }
    if claim == 'motion_animation' and len(distinct_hashes) < 2:
        return {
            **base_verdict,
            'grade': 'FAIL',
            'reason': f'frozen sequence: only {len(distinct_hashes)} unique cropped state across {len(frames)} frames',
            'frame_count': len(frames),
            'distinct_frames': len(distinct_hashes),
        }

    return {
        **base_verdict,
        'grade': 'PASS',
        'reason': 'verified fresh live bounded capture sequence',
        'frame_count': len(frames),
        'distinct_frames': len(distinct_hashes),
        'max_age_ms': max_age,
        'sample_metrics': frame_metrics[-1],
    }


def enforce_bounded_retention(captures_root: Path, max_keep: int = 8) -> list:
    if max_keep < 1 or not captures_root.is_dir():
        return []
    dirs = sorted(
        (d for d in captures_root.iterdir() if d.is_dir()),
        key=lambda p: p.stat().st_mtime,
    )
    removed = []
    while len(dirs) > max_keep:
        victim = dirs.pop(0)
        shutil.rmtree(victim)
        removed.append(str(victim))
    return removed


def main() -> int:
    if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
        raise RuntimeError('Optimized Python prohibited')

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('provenance', type=Path)
    parser.add_argument('--claim', default='static_visual', choices=sorted(VALID_CLAIMS))
    parser.add_argument('--target-id', default='')
    parser.add_argument('--intended-version', default='')
    parser.add_argument('--max-age-ms', type=int, default=250)
    parser.add_argument('--crop', nargs=4, type=int, metavar=('X0', 'Y0', 'X1', 'Y1'))
    parser.add_argument('--clipped-bounds', action='store_true')
    parser.add_argument('--inverted-or-corrupt', action='store_true')
    parser.add_argument('--prune-root', type=Path, default=None)
    parser.add_argument('--max-keep', type=int, default=8)
    args = parser.parse_args()

    res = evaluate_capture(
        args.provenance,
        claim=args.claim,
        target_id=args.target_id,
        intended_version=args.intended_version,
        max_fresh_age_ms=args.max_age_ms,
        crop_box=args.crop,
        clipped_bounds=args.clipped_bounds,
        inverted_or_corrupt=args.inverted_or_corrupt,
    )
    if args.prune_root is not None:
        res['pruned'] = enforce_bounded_retention(args.prune_root, max_keep=args.max_keep)
    print(json.dumps(res, indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
