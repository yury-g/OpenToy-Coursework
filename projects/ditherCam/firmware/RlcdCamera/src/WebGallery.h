// SPDX-License-Identifier: MIT
#pragma once
#include <pgmspace.h>

namespace rlcd_hw {

// Self-contained offline photos-only gallery and settings web UI served by RLCD SoftAP (192.168.4.1).
const char kWebGalleryHtml[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>FlipCam Photos Gallery</title>
<style>
  body{font-family:system-ui,-apple-system,sans-serif;background:#111614;color:#e7ece7;margin:0;padding:16px}
  h1{font-size:1.2rem;margin:0 0 8px}
  .bar{display:flex;flex-wrap:wrap;gap:8px;align-items:center;margin-bottom:14px;font-size:.9rem}
  .pill{background:#1e2723;border:1px solid #384942;border-radius:6px;padding:6px 10px}
  button,select{background:#23312b;color:#eef4ee;border:1px solid #486055;border-radius:6px;padding:6px 10px;cursor:pointer}
  button:hover{background:#2f423a}
  .grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(260px,1fr));gap:12px}
  .card{background:#18201d;border:1px solid #2e3b35;border-radius:8px;padding:10px;display:flex;flex-direction:column;gap:8px}
  .card img{width:100%;aspect-ratio:4/3;object-fit:contain;background:#0a0d0c;border-radius:4px}
  .row{display:flex;justify-content:space-between;align-items:center;gap:6px;flex-wrap:wrap;font-size:.82rem}
  a.btn{color:#cde7d8;text-decoration:none;border:1px solid #486055;padding:4px 8px;border-radius:4px}
</style>
</head>
<body>
<h1>FlipCam Photos Gallery &amp; Settings</h1>
<div class="bar">
  <span class="pill" id="st">Loading...</span>
  <button onclick="act('/api/capture')">Capture Photo</button>
  <button onclick="loadAll()">Refresh</button>
</div>
<div class="bar pill">
  <label>Look <select id="look" onchange="saveSet()"><option value="dither">Dither</option><option value="threshold">Threshold</option></select></label>
  <label>Contrast <select id="contrast" onchange="saveSet()"><option value="low">Low</option><option value="normal">Normal</option><option value="high">High</option></select></label>
  <label>Mirror <select id="mirror" onchange="saveSet()"><option value="0">Off</option><option value="1">On</option></select></label>
  <label>Orientation <select id="orientation" onchange="saveSet()"><option value="selfie">Selfie</option><option value="rear">Rear 180</option></select></label>
  <label>Sound <select id="sound" onchange="saveSet()"><option value="1">On</option><option value="0">Off</option></select></label>
</div>
<div class="grid" id="grid"></div>
<script>
async function api(url, opts){ const r = await fetch(url, opts); return r.json(); }
async function act(url){ const d = await api(url, {method:'POST'}); if(d.error) alert(d.error); loadAll(); }
async function saveSet(){
  const q = new URLSearchParams({
    look: document.getElementById('look').value,
    contrast: document.getElementById('contrast').value,
    mirror: document.getElementById('mirror').value,
    orientation: document.getElementById('orientation').value,
    sound: document.getElementById('sound').value,
  });
  await api('/api/settings?' + q.toString(), {method:'POST'});
  loadStatus();
}
async function loadStatus(){
  const s = await api('/api/status');
  document.getElementById('look').value = s.settings.look;
  document.getElementById('contrast').value = s.settings.contrast;
  document.getElementById('mirror').value = String(s.settings.mirror);
  document.getElementById('orientation').value = s.settings.orientation || 'selfie';
  document.getElementById('sound').value = String(s.settings.sound);
  const freeM = (s.fs_free / 1048576).toFixed(2);
  document.getElementById('st').textContent = `Link:${s.link} | Mode:${(s.settings.orientation||'selfie').toUpperCase()} | Free:${freeM} MiB | Photos:${s.still_count} | Codec:${s.codec_ok?'OK':'FAULT'}`;
}
async function delItem(id){
  if(!confirm('Delete photo #' + id + '?')) return;
  await api('/api/delete?id=' + id, {method:'POST'});
  loadAll();
}
async function loadList(){
  const d = await api('/api/list');
  const g = document.getElementById('grid');
  g.innerHTML = '';
  for(const it of d.items){
    const card = document.createElement('div');
    card.className = 'card';
    card.innerHTML = `
      <div class="row"><strong>${it.title}</strong><span>${it.width}x${it.height} · ${(it.bytes/1024).toFixed(1)} KB</span></div>
      <img src="/api/media?id=${it.id}&fmt=raw" alt="${it.title}">
      <div class="row">
        <a class="btn" href="/api/media?id=${it.id}&fmt=raw" download>Download JPG</a>
        <button onclick="delItem(${it.id})">Delete</button>
      </div>`;
    g.appendChild(card);
  }
}
async function loadAll(){ await loadStatus(); await loadList(); }
loadAll();
setInterval(loadStatus, 2500);
</script>
</body>
</html>)HTML";

}  // namespace rlcd_hw
