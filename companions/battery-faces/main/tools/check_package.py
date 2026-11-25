"""Check staged publication content without printing any credential values."""
from pathlib import Path
import subprocess,re,xml.etree.ElementTree as ET
root=Path(__file__).resolve().parents[1]
git='/Users/example/.cache/codex-runtimes/codex-primary-runtime/dependencies/bin/fallback/git'
files=subprocess.check_output([git,'ls-files','-z'],cwd=root).decode().split('\0');files=[p for p in files if p]
assert files,'Stage the intended package first'
secret=(root/'firmware/FeatherFace/FaceLinkConfig.h').read_text()
values=[x.encode() for x in re.findall(r'=\s*"([^"]+)"',secret)]
assert len(values)==2,'Expected exactly two local network credentials to scan'
for name in files:
 p=root/name
 assert not name.startswith(('private-backup/','.venv/','vendor/')) and p.name!='FaceLinkConfig.h',name
 assert p.suffix not in ('.bin','.elf','.map','.pyc'),name
 data=subprocess.check_output([git,'show',':'+name],cwd=root);assert not any(v in data for v in values),('Private credential found',name)
 assert not re.search(rb'(?:gh[pousr]_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{20,}|sk-(?:proj-)?[A-Za-z0-9_-]{20,}|BEGIN (?:RSA |EC )?PRIVATE KEY)',data),('Token/key-like content',name)
 if p.suffix=='.svg':ET.fromstring(data)
 if p.suffix=='.md' and not name.startswith('hardware/references/'):
  for ref in re.findall(r'\]\(([^)]+)\)',data.decode()):
   if '://' in ref or ref.startswith('#'):continue
   assert (p.parent/ref.split('#')[0]).exists(),(name,ref)
print('PASS staged package:',len(files),'files; no local credentials/backups/binaries; Markdown targets and SVG XML valid')
