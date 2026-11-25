# Nesso target

This is Arduino Nesso N1, **ESP32-C6**, not an ESP32-S3 Feather. Use the C6 ROM identity, actual 16 MB flash layout and active OTA selection from the connected device; never identify by USB pathname. Keep two matching fresh full reads with a hardware digest before app replacement, outside Git. Preserve bootloader, partition/OTA data and all other flash using a bounded app-only write. The existing bench selects app0 at 0x10000 with capacity 0x300000; verify again instead of assuming it for another unit.

Use Arduino-ESP32 3.3.10, M5Unified 0.2.18, M5GFX 0.2.25. Keep Nesso and Feather toolchains separate. Reuse the FIFO display workaround (`dma_channel=0`) from Gravity Water. Touch and LCD use separate buses; touch must not end another display transaction. Read FT5x06 contact data with explicit I2C success checks, then use the display's rotation transform. Failed or stale touch must release the eye.

The canonical v1 renderer remains unchanged. TouchReaction varies its existing mask heights between exact angry/neutral/happy endpoints. Keep host checks separate from physical poke/rotation observations. No extra repository: this stays inside OpenToy's project list.
