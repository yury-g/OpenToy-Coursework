# Source and license provenance

`reference/Board.c`, `Board.h` and `Smooth.h` are unmodified read-only snapshots
of OpenToy's CrowPanel work at the Eyes 0.2.0 development checkpoint, dated
2026-11-17. [Exact source paths and SHA-256 hashes](../docs/renderer-provenance.json)
make the snapshot reproducible without changing another device owner's work.
Runtime results from that work do not transfer to Copy-Face.

The board adapter uses Elecrow DHE04209D V1.0 Lesson05/07 hardware evidence at
vendor commit `342a40719c98601fbd282f72e7fc2cd413133dd8`; no vendor camera code
is bundled. See [camera evidence](../docs/CAMERA-RESEARCH.md).

The build imports unchanged OpenToy Companion, controls, idle, reaction,
personality and protocol headers from `projects/animatronic-eyes`. Their MIT
notices and upstream provenance remain in that project. The native eye renderer
retains its MIT SPDX notice and large-pupil geometry.

ESP-IDF and Espressif components retain Apache-2.0 licenses; LVGL retains MIT.
The component manager keeps their source/licenses in the private build directory.
The complete display/camera dependency resolution is in `locks/camera-idf5.4.2.lock`.
The unmodified SC2336 revision-1 tuning JSON is bundled under Apache-2.0;
[provenance and pinned sources](../locks/camera-notes.md) and the
[license](../locks/esp-cam-sensor-LICENSE.txt) accompany it.
Pillow 12.0.0 is used only for host preview export, under its own license.
