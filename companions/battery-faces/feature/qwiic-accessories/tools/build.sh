#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
[ -f firmware/FeatherFace/FaceLinkConfig.h ] || { echo 'Create private FaceLinkConfig.h from its example first'; exit 1; }
python3 tools/setup_m5gfx.py
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s3_reversetft --build-path outputs/build firmware/FeatherFace
arduino-cli compile --fqbn esp32:esp32:arduino_nesso_n1 --build-path outputs/build-orange firmware/FeatherFace
arduino-cli compile --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB --build-property compiler.cpp.extra_flags=-DFACE_WAVESHARE_169 --build-path outputs/build-yellow firmware/FeatherFace
mkdir -p outputs/build-rainbow
cp outputs/build-yellow/FeatherFace.ino.bin outputs/build-rainbow/FeatherFace.ino.bin
arduino-cli compile --fqbn esp32:esp32:esp32 --build-property compiler.cpp.extra_flags=-DFACE_CYD=1 --build-path outputs/build-purple firmware/FeatherFace
arduino-cli compile --fqbn esp32:esp32:m5stack_tab5:ChipVariant=prev3,PSRAM=enabled,USBMode=hwcdc,CDCOnBoot=cdc --library vendor/M5GFX --build-path outputs/build-monitor firmware/FaceMonitor

arduino-cli compile --fqbn esp32:esp32:esp32s3:FlashSize=8M,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,PartitionScheme=default_8MB --build-property compiler.cpp.extra_flags=-DFACE_STICK=1 --build-path outputs/build-pink firmware/FeatherFace

arduino-cli compile --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB --build-property compiler.cpp.extra_flags=-DFACE_SILVER=1 --build-path outputs/build-silver firmware/FeatherFace
