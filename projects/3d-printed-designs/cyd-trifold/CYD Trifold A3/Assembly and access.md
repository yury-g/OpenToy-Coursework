# CYD trifold A3 — assembly and connector access

This prototype uses the selected original A cone/divot bearing geometry, with
four captive cartridges supporting the top and bottom of both folds. All parts
print flat. Each cartridge prints with its two bearing halves already captured;
there are no separate pins. The enclosure still needs assembly around the boards.

## Print and assemble

Use `Compact A3 - U1 plate.3mf` for the arranged geometry. The plate contains ten
objects: three rear frames, three face frames and four two-body A cartridges.
Keep each cartridge's two bodies together and preserve the supplied orientations.
The 3MF contains geometry, not a validated slicer profile. Physical fit of this
larger adaptation remains untested. Original A was reported to print flat.

Use the interactive preview's exploded view to identify the parts. The left
fold uses two cartridges in their original orientation; the right fold uses two
cartridges flipped over in the assembled enclosure. Every cartridge is printed
in its original flat orientation regardless of how it mounts afterward.

Seat the rear cartridges in their rear-frame cradles, position the front
cartridges on their supports, and place each board in its four corner seats.
All boards face the same direction with their USB ends toward the same case end.
Fit the corresponding face frames; their pockets and stops retain the cartridge
feet. Check board and glass seating before engaging the clips. Do not force a
clip or hinge that binds; inspect the printed clearance first.

## Connector and button access

The user confirmed BOTH USB-C and micro-USB on each ESP32-2432S028. Each case has
an open rear, broad short-end connector approach and long-edge service openings.
These provide space around USB, the power connector, microSD, extended IO, DHT,
speaker, BOOT and RESET. The rear view in the dashboard shows the open frames.
Board electronics in the preview are illustrative envelopes, not a measured
model of this particular dual-USB revision.

At 54 mm board pitch, there is only 4 mm between bare PCB edges. For a plug or
microSD card on an inner edge, fold the neighboring panel away to get access.
The design does not promise simultaneous bulky side plugs with all screens flat.
Unplug external cables before folding; check any protruding card or added wiring.

Exact connector locations, plug housings, clip force and print release still
need checking on the actual boards. Numerical prototype checks are recorded in
`CAD verification.json` and `plate-verification.json`.

Reference interface identities: manufacturer specification, page 5:
https://mischianti.org/wp-content/uploads/2025/04/ESP32-2432S028-Specifications-EN.pdf
The referenced photograph is a micro-USB revision; its layout is not a precise
mechanical drawing of the user's dual-USB boards.

## Final printability review

The snap socket roofs are now 0.8 mm thick, replacing the earlier 0.1 mm ledges.
The face-down snap hooks retain a short approximately 0.6 mm steep cantilever.
This is not a floating part, but it is a local overhang to inspect in the slicer
and physical PLA trial. The design does not claim every surface is within 45°.
See `printability-review.json` for the final mesh-based review.
