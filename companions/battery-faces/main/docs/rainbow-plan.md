# Battery Faces: Rainbow and adaptive monitor

- Keep Green's stable wire index for compatibility, but disable all receive/send/navigation/host access to that identity after the owner's new board-heat report. No Green update or test.
- Add Rainbow at a new stable index, MAC 02:00:00:00:00:07; preserve original flash first. Owner confirmed the same printed Waveshare model and power-pin revision.
- Draw Waveshare UI at native 240×280. Render the original 128×64 face with equal horizontal and vertical scaling on every display. Rainbow gets a spatial rainbow, also when viewed remotely.
- Tab5 arranges only fresh enabled reporters, using computed tile rectangles and uniform card scaling. Missing reporters expire after eight seconds; Green is always excluded. Registry remains an explicit allowlist; adding future hardware requires enrolling its MAC.
- Verify geometry, disabled identity rejection, navigation, adaptive layouts and stale relay handling in host tests. Build and upload active devices only. Verify current source values, radio paths, captures, and remove/rejoin reflow without battery/load tests.
- Preserve screen images and editable illustrative board art, publish Battery Faces as a new private personal GitHub repository, and record a minimal private checkpoint.
