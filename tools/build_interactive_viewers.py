#!/usr/bin/env python3
"""Export only interactive HTML viewers for GitHub Pages. READMEs remain the home."""
import argparse, json, re, shutil, subprocess
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[1]
REPO = 'https://github.com/yury-g/OpenToy-Coursework'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path, help='Empty output directory')
    args = parser.parse_args()
    out = args.output.resolve()
    if out.exists() and any(out.iterdir()):
        parser.error('Output must be empty.')
    out.mkdir(parents=True, exist_ok=True)
    def write(path, text):
        p = out / path
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(text)
    def copy(source, target):
        p = out / target
        p.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / source, p)
    flip = (ROOT/'projects/3d-printed-designs/ditherCam/index.html').read_text()
    flip = flip.replace('https://dithercam-assembly.narwhal2025.chatgpt.site/', 'assembly/')
    write('dithercam/index.html', flip)
    assembly = (ROOT/'projects/3d-printed-designs/ditherCam/assembly/index.html').read_text()
    assembly = assembly.replace('https://dithercam-assembly.narwhal2025.chatgpt.site/flip/', '../')
    assembly = assembly.replace('https://github.com/yury-g/OpenToy/', REPO+'/')
    write('dithercam/assembly/index.html', assembly)
    for p in (ROOT/'projects/3d-printed-designs/cyd-trifold/Folio Dashboard').glob('*.html'):
        copy(p.relative_to(ROOT), 'cyd-trifold/'+p.name)
    copy('projects/3d-printed-designs/cyd-trifold/Hinge comparison/Compare hinges.html', 'cyd-trifold/hinges.html')
    copy('companions/nearby-buddy/main/enclosure/viewer/index.html', 'nearby-buddy/index.html')
    face=(ROOT/'docs/simulations/face-v2-fragment.html').read_text()
    write('face-v2/index.html', '''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>OpenToy · Doodle face</title><style>body{margin:0 auto;padding:28px;max-width:960px;background:#0b1017;color:#eef4f7;font:16px/1.5 system-ui}button{font:inherit;padding:8px 14px;margin:3px;border:1px solid #465563;border-radius:6px;background:#1c2835;color:inherit;cursor:pointer}button[aria-pressed=true]{background:#bcebd5;color:#122821}.viz-row{margin:12px 0}label{display:block}input[type=range]{width:100%;accent-color:#bcebd5}legend{font-weight:bold}select{font:inherit}a{color:#bcebd5}</style><a href="'''+REPO+'''/tree/main/projects/animatronic-eyes">Animatronic Eyes guide</a>'''+face+'</html>')
    write('index.html', '<!doctype html><html lang="en"><meta charset="utf-8"><meta http-equiv="refresh" content="0;url='+REPO+'#readme"><title>OpenToy</title><a href="'+REPO+'#readme">OpenToy on GitHub</a></html>')
    write('.nojekyll', '')
    revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    write('source.json',json.dumps({'repository':REPO,'revision':revision,'generator':'tools/build_interactive_viewers.py'},indent=2)+'\n')
    copy('LICENSE','LICENSE.txt')
    # All copied viewers are self-contained. Check local navigation after rewriting hosting links.
    errors=[]
    for p in out.rglob('*.html'):
        for link in re.findall(r'(?:src|href)=[\"\']([^\"\']+)',p.read_text()):
            if link.startswith(('http:', 'https:', '#', 'data:', 'blob:')) or '${' in link:
                continue
            path=unquote(link.split('?')[0].split('#')[0])
            if path and not (p.parent/path).exists():
                errors.append((str(p.relative_to(out)),link))
    if errors:
        raise SystemExit('Broken local viewer links: '+repr(errors))
    print(f'Exported {len(list(out.rglob("*.html")))-1} viewer pages; local links passed. Source {revision}')

if __name__ == '__main__':
    main()
