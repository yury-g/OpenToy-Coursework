# Recovery

Keep the toy powered during an update. On communication failure, the Mac does not report success. Reconnect, request status and identify the running build. Retry a new authenticated transfer from byte zero; there is no resume across disconnect.

The updater writes the inactive application slot. Partial transfers never select it. A fully transferred image must pass authenticated size/hash checks, structural bounds and SDK validation before staging. REBOOT explicitly selects the staged image. An unconfirmed failing boot rolls back to the previous valid application; that rollback was exercised on both bench boards.

## If Bluetooth is unavailable

Reconnect USB. Verify the expected chip/factory MAC before writing and re-upload the known-good USB seed. The ROM downloader is independent of application firmware. If a bad application prevents entering it automatically, hold BOOT, press/release RESET, then release BOOT and inspect the resulting port. Physical BOOT/RESET is a genuine fallback; it was not needed on these bench boards.

A stalled application was recovered electronically through the USB ROM reset path during malformed-image investigation. No routine whole-flash erase was used.

## Restore the preserved original state

The private `backup/original-flash-8MB.bin` may include old settings or credentials. For an intentional full-state restoration:

```sh
.venv/bin/python projects/xiao-class/scripts/project.py restore-original
```

Use the corresponding project for the bare board. The helper checks image size, SHA256 and the expected chip/MAC before restoring at address0. A full original-state restoration writes the complete saved image, including original partition metadata. This command was provided and inspected, but a full original restoration was not executed in the bench tests. It is a different operation from ordinary application updates.

## Known partition layout

The pre-layer smoke-test source is preserved in the owner's local `OpenToy-smoke-test-source.zip`; its last working device-specific image/manifest remains in the private build folder. Stop controller input before reverting a known-good image. The v1 Mac app pauses layer commands if the running firmware lacks `eyes=1`; the Face v2 app requires `doodle=1`; restore the matching older app source when testing the earlier controller map. Routine application reversion can use the same Bluetooth uploader and inactive slot, without restoring the entire original flash.

Default8MB layout used by both tested boards: NVS0x9000/0x5000, otadata0xe000/0x2000, app0 at0x10000/0x330000, app1 at0x340000/0x330000, SPIFFS0x670000/0x180000, coredump0x7f0000/0x10000. Do not change this layout through a BLE application update. Preserve a known-good source, local key/profile, backup hash, manifest and USB data cable before class.

## Return from Face v2 to v1

Stop controller input and disconnect the app before using the uploader. The owner's private `projects/xiao-class/backup/before-face-v2/ToyBLE.ino.bin` and adjacent `manifest.json` preserve the exact working `EyeLayers` application. Upload with the same BLE helper and retained key/profile. Switch the Mac app source to the preserved `v1.0.0-eyes` revision for its matching Xbox map, then restart the app. The public release contains reproducible source rather than the owner's key-bearing binary. This return path was preserved and inspected; a deliberate return from the accepted v2 build has not been exercised.
