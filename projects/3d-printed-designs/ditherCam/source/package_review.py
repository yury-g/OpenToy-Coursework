"""Repackage the offline dashboard after re-exporting the inspection meshes."""
from pathlib import Path
import base64
import json
import trimesh

source = Path(__file__).resolve().parent
out = source.parent
cad_dir = out / "cad"
assets = {}
for n in ["body", "pod", "hw_rlcd", "hw_xiao_pod", "hw_oled_active"]:
    m = trimesh.load_mesh(cad_dir / (n + ".stl"))
    assets[n] = base64.b64encode(m.export(file_type="stl")).decode()
payload = {
    "meshes": assets,
    "parameters": json.loads((cad_dir / "parameters.json").read_text()),
}
html = (
    (source / "review-template.html")
    .read_text()
    .replace("__ASSETS__", json.dumps(payload))
    .replace(
        "__SCRIPT__",
        (source / "review.bundle.js").read_text().replace("</script", "<\\/script"),
    )
)
(out / "index.html").write_text(html)
print(out / "index.html")
