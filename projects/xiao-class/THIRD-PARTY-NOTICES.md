# Third-party notices

Project firmware, tests and Mac client/app source: MIT (LICENSE).

- Arduino-ESP32 3.3.10: LGPL-2.1-or-later. Original license preserved under licenses/; exact source: https://github.com/espressif/arduino-esp32/tree/3.3.10 . The supplied source/build instructions support rebuilding against this version; Arduino's core and BLE dependency were not modified.
- Arduino-ESP32 bundled BLE library: original LICENSE and NOTICE preserved separately. Original libb64 license is also preserved.
- ESP-IDF 5.5.4: Apache-2.0 and component-specific licenses, including vendor binary radio/controller components. Original root license preserved; exact source and component notices: https://github.com/espressif/esp-idf/tree/v5.5.4 . The ESP32 radio stack is not entirely open-source, although this implementation and its client are.
- Mbed TLS (via pinned SDK): Apache-2.0 or GPL-2.0-or-later dual licensing; used under Apache-2.0. Source: https://github.com/Mbed-TLS/mbedtls . SDK's exact dependency definitions remain in the pinned ESP-IDF sources.
- Bleak 3.0.2: MIT; pyserial 3.5: BSD; PyObjC 12.2.2: MIT. Installed-package license files preserved in licenses/python-packages/. Pinned Python requirements are recorded in the project and repository root requirements files.
- esptool 5.3.0: GPL-2.0-or-later. Used as a separate build/upload tool. Original bundled license and source reference are preserved. https://github.com/espressif/esptool/tree/v5.3.0 .
- Zig 0.16.0: MIT plus bundled LLVM/libc licenses. Used only for host protocol tests; not linked into firmware or distributed in this package. Source/compiler license preserved.
- Bluefruit Connect: existing third-party open-source iPad client, not modified, bundled or tested on the iPad here. https://github.com/adafruit/Bluefruit_LE_Connect_v2 .

Private backups and copied pre-existing class source are preservation artifacts, not relicensed by the project's MIT license. They are excluded from the shareable source archive. Do not redistribute the private device key or original flash image. No binaries are included in the public repository. If distributing binaries publicly, retain all dependency/component notices and provide corresponding LGPL relinking material/source required for that distribution; this local project is not a public release.

Mac launcher packaging uses py2app0.28.10 (MIT), modulegraph0.19.7 (MIT), macholib1.16.4 (MIT), altgraph0.17.5 (MIT), packaging26.3 (Apache/BSD) and setuptools84.0.0 (MIT). Their installed license files are preserved under licenses/python-packages. PyObjC provides the native Cocoa interface. The local app is an alias launcher; it references the pinned runtime and source rather than embedding a portable environment.

OLED dependencies are pinned to Adafruit SH110X2.1.14, Adafruit GFX1.12.6 and Adafruit BusIO1.17.4. Their unmodified source and license notices are preserved under licenses/Adafruit_*. Upstream: https://github.com/adafruit/Adafruit_SH110x, https://github.com/adafruit/Adafruit-GFX-Library, https://github.com/adafruit/Adafruit_BusIO. The checked-frame sender follows the SH1106 page/column-offset behavior documented in the preserved driver. Original class sketches have no license assumed and are excluded from the shareable archive.
