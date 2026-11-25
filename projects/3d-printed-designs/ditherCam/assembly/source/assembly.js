import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import { STLLoader } from 'three/examples/jsm/loaders/STLLoader.js';

const $ = (id) => document.getElementById(id);

const payload = JSON.parse($('assets').textContent);
const assets = payload.meshes || {};
const params = payload.parameters || {};
const hingeParams = params.hinge || {};

const AXIS_Z_PRINT = hingeParams.axis_z_print_mm ?? 22.0;
const Y_SOLE_PRINT = hingeParams.y_sole_print_mm ?? 104.5;

// Exact grounded coordinates in Print Space (mm)
const COORDS = {
  hingeAxis: [0, 0, AXIS_Z_PRINT],
  lensCenter: [-25.15, -45.36, 23.85],
  oledCenter: [5.8, -25.65, 23.95],
  detentLeft: [-25.01, -21.10, 6.35],
  detentRight: [25.01, -21.10, 6.35],
  keeperLeft: [-21.5, 28.1, 20.0],
  keeperRight: [21.5, 28.1, 20.0],
  bottomRetainerLeft: [-38.0, 99.5, 20.0],
  bottomRetainerRight: [38.0, 99.5, 20.0],
  sideRetainerLeft: [-45.6, 79.0, 20.0],
  sideRetainerRight: [48.9, 79.0, 20.0],
  rlcdPivot: [0, 28.1, 19.3],
  batteryCenter: [0, -12.8, 23.85],
  batteryDims: [58.0, 4.0, 42.5],
};

const STEPS = [
  {
    id: 'rlcd-orient',
    guide: 'rlcd',
    number: 1,
    title: 'Identify & Orient RLCD',
    shortTitle: 'Lay housing flat & orient RLCD',
    sub: 'RLCD 1/3 · Screen down · Buttons at top',
    orientation: 'print',
    summary:
      'Lay the printed housing flat face-down on a soft cloth as shown to protect the front bezel and glass, with the open rear bay facing up. Remove the original acrylic stand legs from the RLCD—leave the protective rear acrylic backing on—and align the top button edge to SNAP, PWR, and BROWSE.',
    bullets: [
      'Lay housing face-down on a soft mat or cloth so the open rear bay faces straight up.',
      'Remove the acrylic stand legs only; keep the protective rear acrylic backing installed.',
      'Orient the display glass facing downward toward the front bezel opening.',
      'Line up the top button edge with the SNAP, PWR, and BROWSE markings.',
    ],
    clips: [
      { label: 'Front Bezel Opening (facing bench)', coords: 'Rest housing on a soft cloth to protect screen glass', pod: false },
      { label: 'Top Button Edge', coords: 'Aligns SNAP, PWR, and BROWSE toward the top crown', pod: false },
    ],
    alerts: [
      { type: 'warn', title: 'Check stand legs & backing first', text: 'If the RLCD will not enter the rear bay, verify the acrylic stand legs are removed while keeping the protective rear acrylic backing.' },
    ],
    defaultProgress: 0.25,
    foldAngle: 90,
    focus: 'rlcd-rear',
    callouts: [
      { id: 'back', text: 'Rear entry (open bay facing up)', color: 'teal', space: 'root', pos: [0, 63, 52] },
      { id: 'buttons', text: 'Top button edge (SNAP · PWR · BROWSE)', color: 'teal', space: 'rlcd', pos: [1.6, 28.4, 14] },
      { id: 'rlcd-assy', text: 'RLCD assembly (glass faces down)', color: 'teal', space: 'rlcd', pos: [0, 66, 19.5] },
    ],
  },
  {
    id: 'rlcd-keepers',
    guide: 'rlcd',
    number: 2,
    title: 'Tuck Top Edge Under Rigid Tabs',
    shortTitle: 'Tuck top edge under rigid tabs',
    sub: 'RLCD 2/3 · Top tabs are rigid',
    orientation: 'print',
    summary:
      'Lower the RLCD slightly tilted into the open rear bay and tuck the top button edge beneath the two fixed rigid top tabs. These top tabs are rigid—do not bend or pry them.',
    bullets: [
      'Offer the top button edge first at a gentle angle while lowering downward.',
      'Slide the top edge snugly underneath both highlighted rigid top tabs.',
      'Keep the bottom edge slightly raised so the lower clips stay clear.',
    ],
    clips: [
      { label: 'Rigid Top Tabs (2×)', coords: 'Fixed tabs above button edge — do NOT bend or pry', pod: false },
    ],
    alerts: [
      { type: 'info', title: 'Illustrative preview — not validated fit', text: 'The tilted insertion motion shown here is illustrative only and is not validated in CAD or physical print. No tilt angle guarantees clearance.' },
    ],
    defaultProgress: 0.55,
    foldAngle: 90,
    focus: 'rlcd-keepers',
    callouts: [
      { id: 'kl', text: 'Rigid top tab L (do not bend)', color: 'danger', space: 'root', pos: COORDS.keeperLeft },
      { id: 'kr', text: 'Rigid top tab R (do not bend)', color: 'danger', space: 'root', pos: COORDS.keeperRight },
      { id: 'top-edge', text: 'Top button edge tucks under tabs', color: 'teal', space: 'rlcd', pos: [0, 29.5, 19] },
    ],
  },
  {
    id: 'rlcd-retainers',
    guide: 'rlcd',
    number: 3,
    title: 'Ease Lower Clips & Seat RLCD',
    shortTitle: 'Ease 4 clips outward & seat flat',
    sub: 'RLCD 3/3 · Flex clips in board plane',
    orientation: 'print',
    summary:
      'With the top edge tucked under the rigid tabs, lower the bottom edge downward into the pocket while gently easing the two bottom clips and two side clips outward in the board plane—never pulling away from the screen.',
    bullets: [
      'Support only at the perimeter; never press on the screen glass, headers, or ribbon.',
      'Ease the two bottom clips outward toward the foot edge (in the board plane).',
      'Ease the two side clips slightly outward to the left and right (in the board plane).',
    ],
    clips: [
      { label: 'Lower Clips (2×)', coords: 'Ease gently outward in the board plane toward the foot wall', pod: false },
      { label: 'Side Clips (2×)', coords: 'Ease gently outward left/right in the board plane', pod: false },
    ],
    alerts: [
      { type: 'stop', title: 'STOP if binding or PLA whitening', text: 'Stop immediately if you feel hard resistance or see white PLA stress marks, and identify which highlighted tab or clip is making contact. Never force glass, headers, or ribbon.' },
      { type: 'info', title: 'Illustrative clip preview', text: 'Clip elasticity is not physically validated and clips are not animated flexing.' },
    ],
    defaultProgress: 0.45,
    foldAngle: 90,
    focus: 'rlcd-retainers',
    callouts: [
      { id: 'brl', text: 'Lower clip L (ease outward)', color: 'teal', space: 'root', pos: COORDS.bottomRetainerLeft },
      { id: 'brr', text: 'Lower clip R (ease outward)', color: 'teal', space: 'root', pos: COORDS.bottomRetainerRight },
      { id: 'srl', text: 'Side clip L (ease left)', color: 'teal', space: 'root', pos: COORDS.sideRetainerLeft },
      { id: 'srr', text: 'Side clip R (ease right)', color: 'teal', space: 'root', pos: COORDS.sideRetainerRight },
    ],
  },
  {
    id: 'pod-entry',
    guide: 'pod',
    number: 4,
    title: 'Point Pod Up & Align Camera Stack',
    shortTitle: 'Pod at 90° · Align camera stack',
    sub: 'Camera Pod 1/3 · No cables or SD card',
    orientation: 'print',
    summary:
      'Keep the housing resting flat with the captive pod pointing straight up (90°). Leave the XIAO, Sense camera, and Expansion Base plugged together as one unit, remove all USB cables and the MicroSD card, and align the lens and small screen toward the stepped front faceplate.',
    bullets: [
      'Keep XIAO, Sense camera module, and Expansion Base plugged together as a single unit.',
      'Disconnect USB cables and remove any MicroSD card before sliding in.',
      'Face the camera lens and small screen toward the stepped front faceplate.',
      'Lower straight downward into the open top of the pod (without battery installed).',
    ],
    clips: [
      { label: 'Pod Top Entry', coords: 'Pod pointing straight up (90°) · Clear opening at top', pod: true },
      { label: 'Stepped Front Faceplate', coords: 'Camera lens opening + recessed small-screen window', pod: true },
    ],
    alerts: [
      { type: 'warn', title: 'Do not rotate 180° or force', text: 'If lens or small screen alignment looks wrong, recheck faceplate orientation and cable/SD removal—never rotate 180° or force. Actual board revision or header heights may differ.' },
    ],
    defaultProgress: 0.35,
    foldAngle: 90,
    focus: 'pod-entry',
    callouts: [
      { id: 'pod-entry', text: 'Pod top entry (lower downward)', color: 'orange', space: 'xiao', pos: [0, -25, 48] },
      { id: 'faceplate', text: 'Front stepped faceplate', color: 'orange', space: 'pod', pos: [-8, -46, 36] },
      { id: 'lens', text: 'Camera lens', color: 'orange', space: 'xiao', pos: COORDS.lensCenter },
      { id: 'oled', text: 'Small screen', color: 'teal', space: 'xiao', pos: COORDS.oledCenter },
    ],
  },
  {
    id: 'pod-detents',
    guide: 'pod',
    number: 5,
    title: 'Slide Downward & Seat Detent Clips',
    shortTitle: 'Slide downward into bottom detents',
    sub: 'Camera Pod 2/3 · Bottom hole detents (2×)',
    orientation: 'print',
    summary:
      'Slide the plugged camera stack straight downward behind the stepped faceplate until seated. The bare board edges ride inside the front hold-down rails and click onto two bottom mounting-hole detents near the base.',
    bullets: [
      'Keep the stack straight as you slide downward to the bottom of the pod.',
      'Two bottom detent pins engage the Expansion Base corner mounting holes.',
      'To release or ease seating, pull the side finger tabs gently away from the circuit board.',
    ],
    clips: [
      { label: 'Bottom Detent Clips (2×)', coords: 'Engage board mounting holes · Pull tabs toward back wall to release', pod: true },
    ],
    alerts: [
      { type: 'info', title: 'Digital slide check vs physical springs', text: 'Rigid CAD slide path passed clearance checks excluding the detent tips; physical PLA spring click and deflection remain untested.' },
      { type: 'stop', title: 'Stop if hung up just before seated', text: 'If the stack stops just above the bottom, check the two highlighted detent tabs and release gently away from the board—never push on the camera ribbon or headers.' },
    ],
    defaultProgress: 0.4,
    foldAngle: 90,
    focus: 'pod-detents',
    callouts: [
      { id: 'dl', text: 'Detent clip L (release toward back)', color: 'danger', space: 'pod', pos: COORDS.detentLeft },
      { id: 'dr', text: 'Detent clip R (release toward back)', color: 'danger', space: 'pod', pos: COORDS.detentRight },
      { id: 'back-pod', text: 'Back wall (release direction)', color: 'orange', space: 'pod', pos: [0, -10, 12] },
    ],
  },
  {
    id: 'inspect-finish',
    guide: 'pod',
    number: 6,
    title: 'Stand Upright, Inspect & Optional Battery',
    shortTitle: 'Stand upright, flip test & battery',
    sub: 'Camera Pod 3/3 · Upright pose · Optional battery',
    orientation: 'use',
    summary:
      'Stand the enclosure upright on its flat foot sole. Confirm the camera lens and small screen line up cleanly in the stepped faceplate, verify SNAP / PWR / BROWSE access, and test the captive 0°–180° flip. If adding an optional LiPo battery, toggle Optional Battery and slide it in from the right side with the bay latch released.',
    bullets: [
      'Confirm lens and small screen windows are clear in both Forward (0°) and Selfie (180°) positions.',
      'Test the captive hinge smoothly across 0°–180° without forcing past the stops.',
      'Optional battery: toggle Optional Battery to preview sliding into the separate rear bay from the right side.',
      'Release the battery bay latch before inserting; never use the soft pouch to force the latch aside.',
    ],
    clips: [
      { label: 'Faceplate Windows', coords: 'Verify lens opening and small screen clarity', pod: true },
      { label: 'Optional Battery Bay', coords: 'Separate side-loading pocket with finger release latch', pod: true },
    ],
    alerts: [
      { type: 'warn', title: 'Optional battery & hardware envelope note', text: 'Battery is optional and OFF by default; actual battery thickness with wrap, leads, and connector is unmeasured. Camera model uses an approximate vendor envelope (actual sensor: OV2640).' },
    ],
    defaultProgress: 1.0,
    foldAngle: 180,
    focus: 'inspect',
    callouts: [
      { id: 'lens-f', text: 'Lens opening', color: 'orange', space: 'xiao', pos: COORDS.lensCenter },
      { id: 'oled-f', text: 'Small screen', color: 'teal', space: 'xiao', pos: COORDS.oledCenter },
      { id: 'hinge', text: 'Captive hinge', color: 'amber', space: 'root', pos: [36, 0, 22] },
      { id: 'bat', text: 'Optional battery bay (side entry)', color: 'amber', space: 'pod', pos: [29, -12.8, 23.85], batteryOnly: true },
    ],
  },
];

// Application state: paused by default, transparent shell ON by default, battery OFF by default, loading pose (print) for Steps 1-5
const state = {
  guide: 'all',
  stepIndex: 0,
  insertionProgress: STEPS[0].defaultProgress,
  playing: false,
  exploded: false,
  transparentShell: true,
  showBattery: false,
  showLabels: true,
  orientation: STEPS[0].orientation,
  foldAngleDeg: STEPS[0].foldAngle,
  cameraPreset: 'step',
};

// Three.js scene setup
const stageEl = $('stage');
const scene = new THREE.Scene();
scene.background = new THREE.Color('#081021');

const renderer = new THREE.WebGLRenderer({ antialias: true, powerPreference: 'high-performance' });
renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
stageEl.appendChild(renderer.domElement);

const camera = new THREE.PerspectiveCamera(38, 1, 0.5, 2500);
camera.up.set(0, 0, 1);

const controls = new OrbitControls(camera, renderer.domElement);
controls.enableDamping = true;
controls.dampingFactor = 0.08;
controls.minDistance = 45;
controls.maxDistance = 650;

scene.add(new THREE.HemisphereLight(0xdbeafe, 0x16223b, 1.9));
const keyLight = new THREE.DirectionalLight(0xffffff, 2.3);
keyLight.position.set(140, -210, 290);
scene.add(keyLight);
const fillLight = new THREE.DirectionalLight(0x93c5fd, 1.0);
fillLight.position.set(-160, 180, 140);
scene.add(fillLight);

const grid = new THREE.GridHelper(320, 16, 0x2a406b, 0x162442);
grid.rotation.x = Math.PI / 2;
grid.position.set(0, 20, -0.2);
scene.add(grid);

// Kinematic hierarchy strictly in Print Space inside root
const root = new THREE.Group();
root.name = 'root_print_space';
scene.add(root);

const podGroup = new THREE.Group();
podGroup.name = 'pod_captive_hinge';
root.add(podGroup);

const rlcdPivotGroup = new THREE.Group();
rlcdPivotGroup.name = 'rlcd_pivot';
rlcdPivotGroup.position.set(...COORDS.rlcdPivot);
root.add(rlcdPivotGroup);

const rlcdMeshOffset = new THREE.Group();
rlcdMeshOffset.name = 'rlcd_mesh_offset';
rlcdMeshOffset.position.set(-COORDS.rlcdPivot[0], -COORDS.rlcdPivot[1], -COORDS.rlcdPivot[2]);
rlcdPivotGroup.add(rlcdMeshOffset);

const xiaoGroup = new THREE.Group();
xiaoGroup.name = 'xiao_stack_group';
podGroup.add(xiaoGroup);

const batteryGroup = new THREE.Group();
batteryGroup.name = 'optional_battery_group';
podGroup.add(batteryGroup);

// Materials
const shellBodyMat = new THREE.MeshStandardMaterial({
  color: 0x3b5378,
  roughness: 0.65,
  metalness: 0.06,
  side: THREE.DoubleSide,
  transparent: true,
  opacity: 0.28,
  depthWrite: false,
});
const shellPodMat = new THREE.MeshStandardMaterial({
  color: 0x4f6d99,
  roughness: 0.6,
  metalness: 0.08,
  side: THREE.DoubleSide,
  transparent: true,
  opacity: 0.30,
  depthWrite: false,
});
const rlcdMat = new THREE.MeshStandardMaterial({
  color: 0x2de2c6,
  emissive: 0x0b4a42,
  emissiveIntensity: 0.35,
  roughness: 0.45,
  metalness: 0.08,
  side: THREE.DoubleSide,
});
const xiaoMat = new THREE.MeshStandardMaterial({
  color: 0xff8b3d,
  emissive: 0x4a1f05,
  emissiveIntensity: 0.25,
  roughness: 0.45,
  metalness: 0.1,
  side: THREE.DoubleSide,
});
const oledMat = new THREE.MeshStandardMaterial({
  color: 0x67e8f9,
  emissive: 0x22d3ee,
  emissiveIntensity: 0.85,
  roughness: 0.25,
  metalness: 0.1,
  side: THREE.DoubleSide,
});

// Load embedded STLs (hw_xiao_pod strictly excluded)
const loader = new STLLoader();
const meshesByName = {};

for (const [key, encoded] of Object.entries(assets)) {
  if (key === 'hw_xiao_pod') continue;
  const bin = atob(encoded);
  const bytes = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; i++) bytes[i] = bin.charCodeAt(i);
  const geom = loader.parse(bytes.buffer);
  geom.computeVertexNormals();

  let mat = shellBodyMat;
  let parent = root;
  if (key === 'pod') {
    mat = shellPodMat;
    parent = podGroup;
  } else if (key === 'hw_rlcd') {
    mat = rlcdMat;
    parent = rlcdMeshOffset;
  } else if (key === 'hw_xiao_stack') {
    mat = xiaoMat;
    parent = xiaoGroup;
  } else if (key === 'hw_oled_active') {
    mat = oledMat;
    parent = xiaoGroup;
  }

  const mesh = new THREE.Mesh(geom, mat);
  mesh.name = key;
  parent.add(mesh);
  meshesByName[key] = mesh;
}

// Separate optional battery schematic (58 x 42.5 x 4 mm)
const batteryMat = new THREE.MeshStandardMaterial({
  color: 0xfbbf24,
  emissive: 0x4d3404,
  emissiveIntensity: 0.35,
  roughness: 0.4,
  metalness: 0.05,
  transparent: true,
  opacity: 0.88,
});
const batteryMesh = new THREE.Mesh(
  new THREE.BoxGeometry(...COORDS.batteryDims),
  batteryMat
);
batteryMesh.position.set(...COORDS.batteryCenter);
batteryMesh.name = 'optional_battery_schematic';
batteryGroup.add(batteryMesh);

const batteryWire = new THREE.LineSegments(
  new THREE.EdgesGeometry(batteryMesh.geometry),
  new THREE.LineBasicMaterial({ color: 0xfef08a })
);
batteryWire.position.copy(batteryMesh.position);
batteryGroup.add(batteryWire);

// Static 3D markers for clips, detents, and direction arrows (no animated clip bending)
const markerGroups = {
  rlcdKeepers: new THREE.Group(),
  rlcdRetainers: new THREE.Group(),
  rlcdArrows: new THREE.Group(),
  podDetents: new THREE.Group(),
  podEntryArrow: new THREE.Group(),
  lensAxis: new THREE.Group(),
  batteryArrow: new THREE.Group(),
  hingeAxis: new THREE.Group(),
};

root.add(markerGroups.rlcdKeepers);
root.add(markerGroups.rlcdRetainers);
root.add(markerGroups.rlcdArrows);
root.add(markerGroups.hingeAxis);
podGroup.add(markerGroups.podDetents);
podGroup.add(markerGroups.podEntryArrow);
podGroup.add(markerGroups.lensAxis);
podGroup.add(markerGroups.batteryArrow);

function makeMarkerPin(pos, colorHex, radius = 3.0) {
  const g = new THREE.Group();
  g.position.set(...pos);
  const sphere = new THREE.Mesh(
    new THREE.SphereGeometry(radius, 18, 14),
    new THREE.MeshBasicMaterial({ color: colorHex, transparent: true, opacity: 0.85, depthTest: false })
  );
  sphere.renderOrder = 10;
  const ring = new THREE.Mesh(
    new THREE.RingGeometry(radius * 1.25, radius * 1.75, 24),
    new THREE.MeshBasicMaterial({ color: colorHex, side: THREE.DoubleSide, transparent: true, opacity: 0.9, depthTest: false })
  );
  ring.renderOrder = 10;
  g.add(sphere, ring);
  return g;
}

function makeDirectionArrow(dir, origin, length, colorHex) {
  const arr = new THREE.ArrowHelper(
    new THREE.Vector3(...dir).normalize(),
    new THREE.Vector3(...origin),
    length,
    colorHex,
    Math.min(9, length * 0.32),
    Math.min(4.5, length * 0.16)
  );
  arr.line.material.depthTest = false;
  arr.cone.material.depthTest = false;
  arr.renderOrder = 11;
  return arr;
}

markerGroups.rlcdKeepers.add(makeMarkerPin(COORDS.keeperLeft, 0xfb7185, 2.4));
markerGroups.rlcdKeepers.add(makeMarkerPin(COORDS.keeperRight, 0xfb7185, 2.4));

markerGroups.rlcdRetainers.add(makeMarkerPin(COORDS.bottomRetainerLeft, 0x2de2c6, 2.5));
markerGroups.rlcdRetainers.add(makeMarkerPin(COORDS.bottomRetainerRight, 0x2de2c6, 2.5));
markerGroups.rlcdRetainers.add(makeMarkerPin(COORDS.sideRetainerLeft, 0x2de2c6, 2.5));
markerGroups.rlcdRetainers.add(makeMarkerPin(COORDS.sideRetainerRight, 0x2de2c6, 2.5));
markerGroups.rlcdRetainers.add(makeDirectionArrow([0, 1, 0], [-38, 102, 20], 11, 0x2de2c6));
markerGroups.rlcdRetainers.add(makeDirectionArrow([0, 1, 0], [38, 102, 20], 11, 0x2de2c6));
markerGroups.rlcdRetainers.add(makeDirectionArrow([-1, 0, 0], [-48, 79, 20], 11, 0x2de2c6));
markerGroups.rlcdRetainers.add(makeDirectionArrow([1, 0, 0], [51, 79, 20], 11, 0x2de2c6));

markerGroups.rlcdArrows.add(makeDirectionArrow([0, 0, -1], [0, 63, 76], 24, 0x2de2c6));

markerGroups.podDetents.add(makeMarkerPin(COORDS.detentLeft, 0xfb7185, 2.3));
markerGroups.podDetents.add(makeMarkerPin(COORDS.detentRight, 0xfb7185, 2.3));
markerGroups.podDetents.add(makeDirectionArrow([0, 1, 0], [-25.01, -18.5, 6.35], 12, 0xfb7185));
markerGroups.podDetents.add(makeDirectionArrow([0, 1, 0], [25.01, -18.5, 6.35], 12, 0xfb7185));

markerGroups.podEntryArrow.add(makeDirectionArrow([0, 0, -1], [0, -28, 92], 32, 0xff8b3d));
markerGroups.lensAxis.add(makeDirectionArrow([0, -1, 0], COORDS.lensCenter, 22, 0xff8b3d));
markerGroups.batteryArrow.add(makeDirectionArrow([-1, 0, 0], [72, -12.8, 23.85], 26, 0xfbbf24));

const hingeLineGeom = new THREE.BufferGeometry().setFromPoints([
  new THREE.Vector3(-44, 0, AXIS_Z_PRINT),
  new THREE.Vector3(44, 0, AXIS_Z_PRINT),
]);
const hingeLine = new THREE.Line(
  hingeLineGeom,
  new THREE.LineDashedMaterial({ color: 0xfbbf24, dashSize: 3, gapSize: 2, depthTest: false })
);
hingeLine.computeLineDistances();
hingeLine.renderOrder = 9;
markerGroups.hingeAxis.add(hingeLine);

function printToWorld(vec3Print, space = 'root') {
  const v = new THREE.Vector3(...vec3Print);
  root.updateMatrixWorld(true);
  if (space === 'pod') return v.applyMatrix4(podGroup.matrixWorld);
  if (space === 'rlcd') return v.applyMatrix4(rlcdMeshOffset.matrixWorld);
  if (space === 'xiao') return v.applyMatrix4(xiaoGroup.matrixWorld);
  return v.applyMatrix4(root.matrixWorld);
}

function applyCameraPreset(preset) {
  state.cameraPreset = preset;
  const printMode = state.orientation === 'print';
  const step = STEPS[state.stepIndex];
  const resolved = preset === 'step' ? step.focus : preset;

  if (resolved === 'front') {
    if (printMode) {
      camera.position.set(0, -235, 88);
      controls.target.set(0, -12, 22);
    } else {
      camera.position.set(0, -245, 72);
      controls.target.set(0, 10, 62);
    }
  } else if (resolved === 'rear') {
    if (printMode) {
      camera.position.set(0, 125, 225);
      controls.target.set(0, 56, 18);
    } else {
      camera.position.set(0, 265, 72);
      controls.target.set(0, 25, 58);
    }
  } else if (resolved === 'top') {
    if (printMode) {
      camera.position.set(0, -35, 270);
      controls.target.set(0, 22, 18);
    } else {
      camera.position.set(0, -45, 290);
      controls.target.set(0, 2, 102);
    }
  } else if (resolved === 'rlcd-rear') {
    const t = printToWorld([0, 62, 22], 'root');
    const offset = printMode ? new THREE.Vector3(75, 110, 180) : new THREE.Vector3(95, 210, 45);
    controls.target.copy(t);
    camera.position.copy(t).add(offset);
  } else if (resolved === 'rlcd-keepers') {
    const t = printToWorld([0, 32, 20], 'root');
    const offset = printMode ? new THREE.Vector3(58, 72, 128) : new THREE.Vector3(75, 135, 40);
    controls.target.copy(t);
    camera.position.copy(t).add(offset);
  } else if (resolved === 'rlcd-retainers') {
    const t = printToWorld([0, 78, 20], 'root');
    const offset = printMode ? new THREE.Vector3(72, 130, 136) : new THREE.Vector3(90, 155, 28);
    controls.target.copy(t);
    camera.position.copy(t).add(offset);
  } else if (resolved === 'pod-entry') {
    const t = printToWorld([0, -28, 36], 'root');
    const offset = printMode ? new THREE.Vector3(105, -125, 125) : new THREE.Vector3(125, -135, 80);
    controls.target.copy(t);
    camera.position.copy(t).add(offset);
  } else if (resolved === 'pod-detents') {
    const t = printToWorld([0, -21, 14], 'root');
    const offset = printMode ? new THREE.Vector3(88, 62, 96) : new THREE.Vector3(98, 105, 50);
    controls.target.copy(t);
    camera.position.copy(t).add(offset);
  } else {
    const t = printToWorld([0, 15, 22], 'root');
    const offset = printMode ? new THREE.Vector3(155, -170, 175) : new THREE.Vector3(150, -185, 55);
    controls.target.copy(t);
    camera.position.copy(t).add(offset);
  }

  controls.update();
  syncQaState();
}

const qxPod = new THREE.Quaternion();
const axisVec = new THREE.Vector3(0, 0, AXIS_Z_PRINT);

function applySceneState() {
  const step = STEPS[state.stepIndex];
  const p = Math.max(0, Math.min(1, state.insertionProgress));

  if (state.orientation === 'print') {
    root.rotation.set(0, 0, 0);
    root.position.set(0, 0, 0);
  } else {
    root.rotation.set(-Math.PI / 2, 0, 0);
    root.position.set(0, 0, Y_SOLE_PRINT);
  }

  // Captive pod rotation around (0, 0, 22)
  const foldRad = ((state.foldAngleDeg - 90) * Math.PI) / 180;
  qxPod.setFromAxisAngle(new THREE.Vector3(1, 0, 0), foldRad);
  podGroup.quaternion.copy(qxPod);
  podGroup.position.copy(axisVec.clone().sub(axisVec.clone().applyQuaternion(qxPod)));

  const rlcdMesh = meshesByName.hw_rlcd;
  const xiaoMesh = meshesByName.hw_xiao_stack;
  const oledMesh = meshesByName.hw_oled_active;

  let rlcdZOffset = 0;
  let rlcdTiltDeg = 0;
  let xiaoZOffset = 0;
  let batteryXOffset = 0;

  if (step.id === 'rlcd-orient') {
    rlcdZOffset = 54 * (1 - p) + 34 * p;
    rlcdTiltDeg = 0;
    xiaoZOffset = 50;
  } else if (step.id === 'rlcd-keepers') {
    rlcdZOffset = 34 * (1 - p);
    rlcdTiltDeg = 16;
    xiaoZOffset = 50;
  } else if (step.id === 'rlcd-retainers') {
    rlcdZOffset = 0;
    rlcdTiltDeg = 16 * (1 - p);
    xiaoZOffset = 50;
  } else if (step.id === 'pod-entry') {
    rlcdZOffset = 0;
    rlcdTiltDeg = 0;
    xiaoZOffset = 62 * (1 - p) + 50 * p;
  } else if (step.id === 'pod-detents') {
    rlcdZOffset = 0;
    rlcdTiltDeg = 0;
    xiaoZOffset = 50 * (1 - p);
  } else {
    rlcdZOffset = 0;
    rlcdTiltDeg = 0;
    xiaoZOffset = 0;
    batteryXOffset = state.showBattery ? 44 * (1 - p) : 0;
  }

  if (state.exploded) {
    rlcdZOffset = 46;
    rlcdTiltDeg = 0;
    xiaoZOffset = 50;
    if (state.showBattery) batteryXOffset = 48;
  }

  rlcdPivotGroup.position.set(COORDS.rlcdPivot[0], COORDS.rlcdPivot[1], COORDS.rlcdPivot[2] + rlcdZOffset);
  rlcdPivotGroup.rotation.set((rlcdTiltDeg * Math.PI) / 180, 0, 0);

  xiaoGroup.position.set(0, 0, xiaoZOffset);
  batteryGroup.position.set(batteryXOffset, 0, 0);
  batteryGroup.visible = state.showBattery;

  const inRlcdGuide = step.guide === 'rlcd';
  if (rlcdMesh) rlcdMesh.visible = true;
  const showPodHw = !inRlcdGuide || state.exploded;
  if (xiaoMesh) xiaoMesh.visible = showPodHw;
  if (oledMesh) oledMesh.visible = showPodHw;

  const transparent = state.transparentShell;
  shellBodyMat.transparent = transparent;
  shellBodyMat.opacity = transparent ? 0.28 : 1.0;
  shellBodyMat.depthWrite = !transparent;
  shellPodMat.transparent = transparent;
  shellPodMat.opacity = transparent ? 0.30 : 1.0;
  shellPodMat.depthWrite = !transparent;
  shellBodyMat.needsUpdate = true;
  shellPodMat.needsUpdate = true;

  markerGroups.rlcdKeepers.visible = step.id === 'rlcd-keepers' || step.id === 'rlcd-retainers';
  markerGroups.rlcdRetainers.visible = step.id === 'rlcd-retainers';
  markerGroups.rlcdArrows.visible = inRlcdGuide && !state.exploded;
  markerGroups.podDetents.visible = step.id === 'pod-detents';
  markerGroups.podEntryArrow.visible = step.id === 'pod-entry' || step.id === 'pod-detents';
  markerGroups.lensAxis.visible = step.guide === 'pod';
  markerGroups.batteryArrow.visible = state.showBattery;
  markerGroups.hingeAxis.visible = step.id === 'inspect-finish' || state.foldAngleDeg !== 90;

  // Plain-language scrubber readout (no axis jargon)
  const pct = Math.round(p * 100);
  const slider = $('scrub-slider');
  slider.value = String(pct);
  slider.style.setProperty('--control-value', String(pct));
  slider.setAttribute('aria-valuetext', `${pct} percent`);

  let readoutText = `${pct}%`;
  if (step.id === 'rlcd-orient') {
    readoutText = `${pct}% · Above open rear bay`;
  } else if (step.id === 'rlcd-keepers') {
    readoutText = `${pct}% · Tucking top edge (illustrative)`;
  } else if (step.id === 'rlcd-retainers') {
    readoutText = pct >= 99 ? '100% · Seated (illustrative)' : `${pct}% · Lowering past clips`;
  } else if (step.id === 'pod-entry') {
    readoutText = `${pct}% · Aligned above pod opening`;
  } else if (step.id === 'pod-detents') {
    readoutText = pct >= 99 ? '100% · Seated in detents' : `${pct}% · Sliding downward`;
  } else if (state.showBattery) {
    readoutText = pct >= 99 ? '100% · Battery in bay' : `${pct}% · Sliding battery from side`;
  } else {
    readoutText = `${pct}% · Seated inspection`;
  }
  $('scrub-readout').textContent = readoutText;

  const foldSlider = $('fold-slider');
  const fDeg = Math.round(state.foldAngleDeg);
  foldSlider.value = String(fDeg);
  foldSlider.style.setProperty('--control-value', String(fDeg));
  foldSlider.setAttribute('aria-valuetext', `${fDeg} degrees`);
  const fMode = fDeg <= 2 ? 'Forward' : fDeg >= 178 ? 'Selfie' : fDeg === 90 ? 'Loading / Install' : 'Flipping';
  $('fold-readout').textContent = `${fDeg}° · ${fMode}`;

  $('btn-play').setAttribute('aria-pressed', String(state.playing));
  $('btn-play').textContent = state.playing ? 'Pause' : 'Play step';
  $('btn-explode').setAttribute('aria-pressed', String(state.exploded));
  $('btn-explode').textContent = state.exploded ? 'Reassemble' : 'Explode';
  $('btn-shell').setAttribute('aria-pressed', String(state.transparentShell));
  $('btn-battery').setAttribute('aria-pressed', String(state.showBattery));
  $('btn-labels').setAttribute('aria-pressed', String(state.showLabels));
  $('btn-pose').setAttribute('aria-pressed', String(state.orientation === 'use'));

  $('hud-step-pill').innerHTML = `<strong>Step ${step.number}/6</strong> · ${step.title}`;
  $('hud-validation-pill').textContent = state.playing
    ? 'Playing step · Drag to orbit'
    : state.exploded
      ? 'Exploded (pod stays captive) · Drag to orbit'
      : state.orientation === 'print'
        ? 'Lying flat (loading pose) · Drag to orbit'
        : 'Standing upright · Drag to orbit';

  syncQaState({ rlcdZOffset, rlcdTiltDeg, xiaoZOffset, batteryXOffset });
}

function syncQaState(offsets = {}) {
  const step = STEPS[state.stepIndex];
  const rlcdZ = offsets.rlcdZOffset ?? (rlcdPivotGroup.position.z - COORDS.rlcdPivot[2]);
  const rlcdTilt = offsets.rlcdTiltDeg ?? ((rlcdPivotGroup.rotation.x * 180) / Math.PI);
  const xiaoZ = offsets.xiaoZOffset ?? xiaoGroup.position.z;
  const batX = offsets.batteryXOffset ?? batteryGroup.position.x;

  window.assemblyGuideState = {
    ready: true,
    guide: state.guide,
    stepIndex: state.stepIndex,
    stepNumber: step.number,
    stepId: step.id,
    stepTitle: step.title,
    insertionProgress: Number(state.insertionProgress.toFixed(4)),
    playing: state.playing,
    exploded: state.exploded,
    transparentShell: state.transparentShell,
    showBattery: state.showBattery,
    showLabels: state.showLabels,
    orientation: state.orientation,
    foldAngleDeg: Math.round(state.foldAngleDeg),
    cameraPreset: state.cameraPreset,
    meshesLoaded: Object.keys(meshesByName),
    excludedMeshes: ['hw_xiao_pod'],
    clipHighlights: step.clips.map((c) => ({ label: c.label, note: c.coords })),
    groundedCoordsPrintMm: {
      hingeAxis: COORDS.hingeAxis,
      lensCenter: COORDS.lensCenter,
      oledCenter: COORDS.oledCenter,
      detents: [COORDS.detentLeft, COORDS.detentRight],
      rlcdKeepers: [COORDS.keeperLeft, COORDS.keeperRight],
      rlcdBottomRetainers: [COORDS.bottomRetainerLeft, COORDS.bottomRetainerRight],
      rlcdSideRetainers: [COORDS.sideRetainerLeft, COORDS.sideRetainerRight],
    },
    transforms: {
      rootPosition: root.position.toArray(),
      rootRotationEuler: [root.rotation.x, root.rotation.y, root.rotation.z],
      podPosition: podGroup.position.toArray(),
      podQuaternion: podGroup.quaternion.toArray(),
      rlcdOffsetPrint: [0, 0, Number(rlcdZ.toFixed(3))],
      rlcdTiltDeg: Number(rlcdTilt.toFixed(2)),
      xiaoOffsetPrint: [0, 0, Number(xiaoZ.toFixed(3))],
      batteryOffsetPrint: [Number(batX.toFixed(3)), 0, 0],
    },
  };
}

function renderStepUI() {
  const step = STEPS[state.stepIndex];
  const listEl = $('step-list');
  listEl.innerHTML = '';

  const filtered =
    state.guide === 'all' ? STEPS : STEPS.filter((s) => s.guide === state.guide);

  for (const s of filtered) {
    const li = document.createElement('li');
    const btn = document.createElement('button');
    btn.type = 'button';
    btn.className = `step-btn ${s.guide === 'pod' ? 'pod-step' : ''}`;
    if (s.number === step.number) btn.setAttribute('aria-current', 'step');
    btn.innerHTML = `
      <span class="step-badge">${s.number}</span>
      <span class="step-btn-meta">
        <span class="step-btn-title">${s.shortTitle}</span>
        <span class="step-btn-sub">${s.sub}</span>
      </span>`;
    btn.addEventListener('click', () => selectStep(s.number - 1));
    li.appendChild(btn);
    listEl.appendChild(li);
  }

  $('step-counter').textContent = `Step ${step.number} of ${STEPS.length}`;
  $('btn-prev-step').setAttribute('aria-disabled', String(state.stepIndex === 0));
  $('btn-next-step').setAttribute('aria-disabled', String(state.stepIndex === STEPS.length - 1));

  $('step-title').textContent = `${step.number}. ${step.title}`;
  $('step-summary').textContent = step.summary;

  const bulletsEl = $('step-bullets');
  bulletsEl.innerHTML = '';
  for (const b of step.bullets) {
    const li = document.createElement('li');
    li.textContent = b;
    bulletsEl.appendChild(li);
  }

  const clipsEl = $('step-clips');
  clipsEl.innerHTML = '';
  for (const c of step.clips) {
    const li = document.createElement('li');
    li.className = `clip-item ${c.pod ? 'is-pod' : ''}`;
    li.innerHTML = `<strong>${c.label}</strong><span class="clip-note">${c.coords}</span>`;
    clipsEl.appendChild(li);
  }

  const alertsEl = $('step-alerts');
  alertsEl.innerHTML = '';
  for (const a of step.alerts) {
    const div = document.createElement('div');
    div.className = `alert-box alert-${a.type}`;
    div.innerHTML = `<strong>${a.title}</strong>${a.text}`;
    alertsEl.appendChild(div);
  }

  $('guide-all').setAttribute('aria-pressed', String(state.guide === 'all'));
  $('guide-rlcd').setAttribute('aria-pressed', String(state.guide === 'rlcd'));
  $('guide-pod').setAttribute('aria-pressed', String(state.guide === 'pod'));
}

function announce(msg) {
  $('live-status').textContent = msg;
}

function selectStep(index, { keepFold = false } = {}) {
  const clamped = Math.max(0, Math.min(STEPS.length - 1, index));
  state.stepIndex = clamped;
  const step = STEPS[clamped];
  state.playing = false;
  state.exploded = false;
  state.insertionProgress = step.defaultProgress;
  state.orientation = step.orientation;
  if (!keepFold) state.foldAngleDeg = step.foldAngle;
  $('scrub-slider').classList.toggle('range-orange', step.guide === 'pod');
  renderStepUI();
  applySceneState();
  applyCameraPreset('step');
  announce(`Step ${step.number} of 6: ${step.title}`);
}

function setGuide(guide) {
  state.guide = guide;
  if (guide === 'rlcd' && STEPS[state.stepIndex].guide !== 'rlcd') {
    selectStep(0);
    return;
  }
  if (guide === 'pod' && STEPS[state.stepIndex].guide !== 'pod') {
    selectStep(3);
    return;
  }
  renderStepUI();
  syncQaState();
}

const calloutLayer = $('callout-layer');
const projVec = new THREE.Vector3();

function updateCallouts() {
  const step = STEPS[state.stepIndex];
  if (!state.showLabels) {
    if (calloutLayer.childElementCount) calloutLayer.innerHTML = '';
    return;
  }

  const rect = stageEl.getBoundingClientRect();
  if (rect.width < 10 || rect.height < 10) return;

  const activeCallouts = step.callouts.filter(
    (c) => !c.batteryOnly || state.showBattery
  );

  if (calloutLayer.childElementCount !== activeCallouts.length) {
    calloutLayer.innerHTML = '';
    for (const c of activeCallouts) {
      const el = document.createElement('div');
      el.className = `pin pin-${c.color}`;
      el.textContent = c.text;
      calloutLayer.appendChild(el);
    }
  }

  activeCallouts.forEach((c, idx) => {
    const el = calloutLayer.children[idx];
    if (!el) return;
    el.className = `pin pin-${c.color}`;
    el.textContent = c.text;
    const worldPos = printToWorld(c.pos, c.space);
    projVec.copy(worldPos).project(camera);
    if (projVec.z > 1 || projVec.z < -1) {
      el.style.display = 'none';
      return;
    }
    const x = (projVec.x * 0.5 + 0.5) * rect.width;
    const y = (-projVec.y * 0.5 + 0.5) * rect.height;
    if (x < 16 || x > rect.width - 16 || y < 28 || y > rect.height - 16) {
      el.style.display = 'none';
      return;
    }
    el.style.display = 'block';
    el.style.left = `${x.toFixed(1)}px`;
    el.style.top = `${y.toFixed(1)}px`;
  });
}

// Sidebar Tabs
const tabButtons = [$('tab-steps'), $('tab-trouble'), $('tab-specs')];
const tabPanels = [$('panel-steps'), $('panel-trouble'), $('panel-specs')];

function selectTab(selectedBtn) {
  tabButtons.forEach((btn, i) => {
    const isSel = btn === selectedBtn;
    btn.setAttribute('aria-selected', String(isSel));
    btn.toggleAttribute('focusgroupstart', isSel);
    if (!('focusGroup' in HTMLElement.prototype)) {
      btn.tabIndex = isSel ? 0 : -1;
    }
    const panel = tabPanels[i];
    if (isSel) {
      panel.hidden = false;
      panel.tabIndex = 0;
    } else {
      panel.hidden = 'until-found';
      panel.removeAttribute('tabindex');
    }
  });
}

tabButtons.forEach((btn, idx) => {
  btn.addEventListener('click', () => selectTab(btn));
  tabPanels[idx].addEventListener('beforematch', () => selectTab(btn));
});

if (!('focusGroup' in HTMLElement.prototype)) {
  const tablist = document.querySelector('.tablist');
  tabButtons.forEach((b, i) => (b.tabIndex = i === 0 ? 0 : -1));
  tablist.addEventListener('keydown', (e) => {
    const cur = tabButtons.indexOf(document.activeElement);
    if (cur < 0) return;
    let next = -1;
    if (e.key === 'ArrowRight') next = (cur + 1) % tabButtons.length;
    else if (e.key === 'ArrowLeft') next = (cur - 1 + tabButtons.length) % tabButtons.length;
    else if (e.key === 'Home') next = 0;
    else if (e.key === 'End') next = tabButtons.length - 1;
    if (next >= 0) {
      e.preventDefault();
      tabButtons[next].focus();
      selectTab(tabButtons[next]);
    }
  });

  const bar = document.querySelector('.toolbar');
  const items = Array.from(bar.querySelectorAll('button'));
  items.forEach((b, i) => (b.tabIndex = i ? -1 : 0));
  bar.addEventListener('focusin', ({ target }) => {
    if (items.includes(target)) items.forEach((b) => (b.tabIndex = b === target ? 0 : -1));
  });
  bar.addEventListener('keydown', (e) => {
    const cur = items.indexOf(document.activeElement);
    if (cur < 0) return;
    let next = -1;
    if (e.key === 'ArrowRight') next = (cur + 1) % items.length;
    else if (e.key === 'ArrowLeft') next = (cur - 1 + items.length) % items.length;
    else if (e.key === 'Home') next = 0;
    else if (e.key === 'End') next = items.length - 1;
    if (next >= 0) {
      e.preventDefault();
      items[next].focus();
    }
  });
}

// Event bindings
$('guide-all').addEventListener('click', () => setGuide('all'));
$('guide-rlcd').addEventListener('click', () => setGuide('rlcd'));
$('guide-pod').addEventListener('click', () => setGuide('pod'));

$('btn-prev-step').addEventListener('click', () => {
  if (state.stepIndex > 0) selectStep(state.stepIndex - 1);
});
$('btn-next-step').addEventListener('click', () => {
  if (state.stepIndex < STEPS.length - 1) selectStep(state.stepIndex + 1);
});

$('btn-play').addEventListener('click', () => {
  state.playing = !state.playing;
  if (state.playing) {
    state.exploded = false;
    if (state.insertionProgress >= 0.99) state.insertionProgress = 0;
  }
  applySceneState();
  announce(state.playing ? 'Step animation playing' : 'Step animation paused');
});

$('btn-explode').addEventListener('click', () => {
  state.exploded = !state.exploded;
  state.playing = false;
  applySceneState();
  announce(state.exploded ? 'Exploded view enabled (pod remains captive on hinge)' : 'Reassembled view');
});

$('btn-shell').addEventListener('click', () => {
  state.transparentShell = !state.transparentShell;
  applySceneState();
  announce(state.transparentShell ? 'Transparent shell enabled' : 'Solid shell enabled');
});

$('btn-battery').addEventListener('click', () => {
  state.showBattery = !state.showBattery;
  applySceneState();
  announce(state.showBattery ? 'Optional battery schematic shown' : 'Optional battery schematic hidden');
});

$('btn-labels').addEventListener('click', () => {
  state.showLabels = !state.showLabels;
  applySceneState();
});

$('btn-pose').addEventListener('click', () => {
  state.orientation = state.orientation === 'use' ? 'print' : 'use';
  applySceneState();
  applyCameraPreset(state.cameraPreset);
  announce(`Orientation: ${state.orientation === 'print' ? 'Lying flat loading pose' : 'Standing upright pose'}`);
});

$('btn-view-step').addEventListener('click', () => applyCameraPreset('step'));
$('btn-view-front').addEventListener('click', () => applyCameraPreset('front'));
$('btn-view-rear').addEventListener('click', () => applyCameraPreset('rear'));
$('btn-view-top').addEventListener('click', () => applyCameraPreset('top'));

$('scrub-slider').addEventListener('input', (e) => {
  state.playing = false;
  state.exploded = false;
  state.insertionProgress = Number(e.target.value) / 100;
  applySceneState();
});

$('fold-slider').addEventListener('input', (e) => {
  state.foldAngleDeg = Number(e.target.value);
  applySceneState();
});
$('btn-fold-0').addEventListener('click', () => {
  state.foldAngleDeg = 0;
  applySceneState();
});
$('btn-fold-90').addEventListener('click', () => {
  state.foldAngleDeg = 90;
  applySceneState();
});
$('btn-fold-180').addEventListener('click', () => {
  state.foldAngleDeg = 180;
  applySceneState();
});

document.querySelectorAll('.jump-step').forEach((btn) => {
  btn.addEventListener('click', () => {
    const idx = Number(btn.dataset.step);
    selectTab($('tab-steps'));
    selectStep(idx);
  });
});

function resize() {
  const r = stageEl.getBoundingClientRect();
  if (r.width <= 0 || r.height <= 0) return;
  renderer.setSize(r.width, r.height);
  camera.aspect = r.width / r.height;
  camera.updateProjectionMatrix();
}
new ResizeObserver(resize).observe(stageEl);

// Initialize
selectTab($('tab-steps'));
renderStepUI();
applySceneState();
resize();
applyCameraPreset('step');

let lastTime = performance.now();
function tick(now) {
  const dt = Math.min((now - lastTime) / 1000, 0.1);
  lastTime = now;

  if (state.playing) {
    state.insertionProgress += dt * 0.28;
    if (state.insertionProgress >= 1.0) {
      state.insertionProgress = 1.0;
      state.playing = false;
      announce('Step insertion reached seated position');
    }
    applySceneState();
  }

  controls.update();
  renderer.render(scene, camera);
  updateCallouts();
  requestAnimationFrame(tick);
}

requestAnimationFrame(tick);
