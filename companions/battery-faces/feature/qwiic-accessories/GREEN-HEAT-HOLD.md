# Retired Green hardware: keep disconnected

The damaged original Green Feather MAC **02:00:00:00:00:03** remains excluded after the October19 ESP-chip/board heat report. Keep its USB and LiPo disconnected. No probing, flashing, charging or runtime tests are authorized for that board.

October20: the owner supplied a different, new Adafruit Feather ESP32-S3 Reverse TFT, MAC **02:00:00:00:00:02**, to become Green Face. Registry index1 and the green color now belong to this replacement. A LiPo is attached, as owner-confirmed. This does not clear the hold on the original hardware: packet filtering still explicitly rejects its MAC, and installers match only the new MAC. Old images remain historical.
