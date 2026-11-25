#!/usr/bin/env python3
"""Export the actual native eye renderer with scripted input. Requires Pillow."""
import hashlib,json,os,shlex,subprocess,tempfile
from pathlib import Path
from PIL import Image
root=Path(__file__).resolve().parents[1];out=root/'images';out.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='crowpanel-preview-') as temp:
 exe=Path(temp)/'preview'
 subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-O2','-std=c++14',str(root/'tools/preview.cpp'),'-o',str(exe)],check=True)
 process=subprocess.Popen([str(exe)],stdout=subprocess.PIPE)
 frames=[]
 for i in range(120):
  data=process.stdout.read(1024*600*3)
  if len(data)!=1024*600*3:raise RuntimeError('Incomplete preview frame')
  frames.append(Image.frombytes('RGB',(1024,600),data))
 assert process.wait()==0
 frames[10].save(out/'eyes-smooth.png')
 frames[0].save(out/'eyes-smooth.gif',save_all=True,append_images=frames[1:],duration=100,loop=0,optimize=True)
 record={'kind':'actual native renderer with scripted input; not hardware video or speed evidence; header/footer omitted','frames':120,'step_ms':100,'left_poke_ms':[2000,2400],
         'renderer_sha256':hashlib.sha256((root/'firmware/main/Smooth.h').read_bytes()).hexdigest(),
         'metadata':json.loads((root/'firmware/metadata.json').read_text())}
 (out/'preview-manifest.json').write_text(json.dumps(record,indent=2)+'\n')
 print('Native renderer preview:',out/'eyes-smooth.png')
