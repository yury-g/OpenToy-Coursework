# Source provenance

The P4 board adapter uses the hardware pin assignments and MIPI timing documented
in Elecrow DHE04209D V1.0 examples Lesson05-Touchscreen and
Lesson07-Turn_on_the_screen at commit
[`342a40719c98601fbd282f72e7fc2cd413133dd8`](https://github.com/Elecrow-RD/CrowPanel-Advanced-9inch-ESP32-P4-HMI-AI-Display-1024x600-IPS-Touch-Screen/tree/342a40719c98601fbd282f72e7fc2cd413133dd8/example/V1.0/idf-code).
The application and adapter are new OpenToy code; the vendor's demo application
is not bundled. Refer to the vendor source for its original comments and notices.

The build imports unchanged OpenToy Companion, reaction, idle, personality and
renderer headers from `projects/animatronic-eyes`; their MIT notices and
upstream provenance remain in that project. No shared target files are modified.

ESP-IDF and Espressif display/touch/port components retain their Apache-2.0
licenses; LVGL retains its MIT license. The component manager downloads their
sources and license files into the private build directory. Dependency versions
are pinned in `main/idf_component.yml`; keep its resolved lockfile with the build.
