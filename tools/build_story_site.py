#!/usr/bin/env python3
"""Export the OpenToy public story site from the same sources as its README."""
import argparse, csv, html, json, shutil, subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPO = 'https://github.com/yury-g/OpenToy-Coursework'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path, help='Empty directory for the static site')
    args = parser.parse_args()
    out = args.output.resolve()
    if out.exists() and any(out.iterdir()):
        parser.error('Output must be empty; use a fresh directory to avoid deleting other work.')
    out.mkdir(parents=True, exist_ok=True)
    def write(path, text):
        p = out / path
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(text)
    def copy(source, target=None):
        dest = out / (target or ('assets/' + source))
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / source, dest)
        return '/' + dest.relative_to(out).as_posix()
    def guide(path):
        return path if path.startswith('https://') else REPO + '/blob/main/' + path
    demos = {
        'dithercam': {'title':'ditherCam · flip the camera', 'path':'/viewers/dithercam.html', 'guide':'projects/3d-printed-designs/ditherCam/README.md', 'note':'Interactive CAD · orbit, flip and inspect the real enclosure geometry.'},
        'assembly': {'title':'ditherCam · assemble the camera', 'path':'/viewers/assembly.html', 'guide':'projects/3d-printed-designs/ditherCam/assembly/README.md', 'note':'Interactive assembly · follow the insertion and removal sequence.'},
        'trifold': {'title':'CYD tri-fold · A6', 'path':'/viewers/trifold/index.html', 'guide':'projects/3d-printed-designs/cyd-trifold/README.md', 'note':'Interactive CAD · one print, three captive frames. Physical fit remains open.'},
        'face': {'title':'Eyes 2 · doodle puppetry', 'path':'/viewers/face.html', 'guide':'projects/animatronic-eyes/README.md', 'note':'Interactive design simulation · bend eyes, brows and mouth with the sliders.'},
        'buddy': {'title':'Nearby Buddy · look inside', 'path':'/viewers/buddy.html', 'guide':'companions/nearby-buddy/main/enclosure/README.md', 'note':'Interactive enclosure study · orbit, explode the assembly and hide parts.'},
        'hinges': {'title':'CYD · compare hinge designs', 'path':'/viewers/hinges.html', 'guide':'projects/3d-printed-designs/cyd-trifold/README.md', 'note':'Interactive CAD comparison · explore how the hinge design developed.'},
        'notebooks': {'title':'CYD · four rounded notebooks', 'path':'/viewers/trifold/notebooks.html', 'guide':'projects/3d-printed-designs/cyd-trifold/README.md', 'note':'Interactive CAD · four rounded notebooks and captive hinges.'},
    }
    for key, src in [
        ('dithercam','projects/3d-printed-designs/ditherCam/index.html'),
        ('assembly','projects/3d-printed-designs/ditherCam/assembly/index.html'),
        ('buddy','companions/nearby-buddy/main/enclosure/viewer/index.html'),
        ('hinges','projects/3d-printed-designs/cyd-trifold/Hinge comparison/Compare hinges.html')]:
        s=(ROOT/src).read_text()
        s=s.replace('https://dithercam-assembly.narwhal2025.chatgpt.site/flip/', '/viewers/dithercam.html')
        s=s.replace('https://dithercam-assembly.narwhal2025.chatgpt.site/', '/viewers/assembly.html')
        s=s.replace('https://github.com/yury-g/OpenToy/', REPO+'/')
        write(demos[key]['path'].lstrip('/'), s)
    for p in (ROOT/'projects/3d-printed-designs/cyd-trifold/Folio Dashboard').glob('*.html'):
        copy(str(p.relative_to(ROOT)), 'viewers/trifold/'+p.name)
    face = (ROOT/'docs/simulations/face-v2-fragment.html').read_text()
    write('viewers/face.html', '''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>OpenToy · Doodle face</title><style>body{margin:0 auto;padding:28px;max-width:960px;background:#0b1017;color:#eef4f7;font:16px/1.5 system-ui}button{font:inherit;padding:8px 14px;margin:3px;border:1px solid #465563;border-radius:6px;background:#1c2835;color:inherit;cursor:pointer}button[aria-pressed=true]{background:#bcebd5;color:#122821}.viz-row{margin:12px 0}label{display:block}input[type=range]{width:100%;accent-color:#bcebd5}legend{font-weight:bold}select{font:inherit}a{color:#bcebd5}</style>'''+face+'</html>')
    cards=[]
    metadata=json.loads((ROOT/'docs/site/projects.json').read_text())
    rows=list(csv.DictReader((ROOT/'docs/home-gallery.tsv').open(),delimiter='\t'))
    for i,r in enumerate(rows):
        image=copy(r['image'])
        meta=metadata.get(r['image'], {})
        category=meta.get('category', 'play')
        demo=meta.get('demo')
        title,alt,caption=map(html.escape,[r['title'],r['alt'],r['caption']])
        link=guide(r['guide'])
        launch=f'<a class="launch" href="/viewer/?demo={demo}">Open interactive viewer</a>' if demo else ''
        cards.append(f'<article class="project" data-category="{category}"><a href="{link}"><img loading="lazy" src="{image}" alt="{alt}"></a><div class="project-body"><span class="eyebrow">{category} / project guide</span><h3><a href="{link}">{title}</a></h3><p>{caption}</p>{launch}</div></article>')
    for src in ['projects/3d-printed-designs/ditherCam/images/ditherCam-hero.png','projects/3d-printed-designs/cyd-trifold/folded.png']:
        copy(src)
    demos_html=''.join(f'<a class="demo-link" href="/viewer/?demo={key}"><strong>{html.escape(d["title"])}</strong><span>{html.escape(d["note"])}</span></a>' for key,d in demos.items())
    devices=[('Feather','feather/previews/feather-model-thumb.png','feather'),('Nesso N1','nesso-n1/previews/nesso-model.png','nesso-n1'),('Round Waveshare','waveshare-round/previews/speaker-box-envelope.svg','waveshare-round'),('Reflective LCD','waveshare-rlcd/previews/product.webp','waveshare-rlcd'),('CrowPanel P4',None,'crowpanel'),('Tab5','tab5/previews/Tab5-front.png','tab5'),('UNO Q','uno-q/previews/uno-model.png','uno-q'),('XIAO / Sense','xiao/previews/base-model-thumb.png','xiao')]
    device_html=[]
    for title,img,slug in devices:
        if img:
            url=copy('hardware/'+img)
            visual=f'<img loading="lazy" src="{url}" alt="{title} hardware reference">'
        else:
            visual='<span class="panel-symbol" aria-label="CrowPanel large rectangular display diagram">9″<small>CrowPanel P4</small></span>'
        device_html.append(f'<a class="device" href="{REPO}/tree/main/hardware/{slug}">{visual}<strong>{title}</strong></a>')
    template=(ROOT/'docs/site/index.html').read_text()
    for key,value in {'{{COUNT}}':str(len(rows)),'{{PROJECTS}}':'\n'.join(cards),'{{DEMOS}}':demos_html,'{{DEVICES}}':''.join(device_html),'{{REPO}}':REPO}.items():
        template=template.replace(key,value)
    write('index.html',template)
    wrapper=(ROOT/'docs/site/viewer.html').read_text().replace('{{DEMOS_JSON}}',json.dumps(demos).replace('</','<\\/')).replace('{{REPO}}',REPO)
    write('viewer/index.html',wrapper)
    for name in ['style.css','site.js']:
        copy('docs/site/'+name,name)
    revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    write('source.json', json.dumps({'repository':REPO,'revision':revision,'catalog':'docs/home-gallery.tsv','generator':'tools/build_story_site.py'},indent=2)+'\n')
    copy('LICENSE','LICENSE.txt')
    print(f'Exported {len(rows)} projects, {len(demos)} interactive entries, {len(devices)} devices. Source {revision}')

if __name__ == '__main__':
    main()
