# Gravity Water implementation record

Goal: a responsive miniature bottle on Nesso, reusable across future OpenToy
adapters, with a fixed slim battery/face bar and preserved Face recovery.

The owner initially requested a lightweight 2D liquid test and later clarified
that the visual goal is individual particles, aesthetically like UnityCoder's
Water Particles. The final implementation follows that explicit correction:
400 blue beads, 2D fixed-point particle motion, spatial neighbor collisions,
Nesso-specific sensor/display adapter, and continuous gravity.

- [x] Preserve the Face branch and create feature/gravity-water from 7196c33.
- [x] Test portable gravity and particle count, boundaries, cardinal flow,
  reversals, near-flat noise, invalid input and renderer bounds.
- [x] Implement 400 shaded beads with momentum and neighbor collision response.
- [x] Add the Nesso adapter, 13px battery/face bar and independent fresh BMI270
  sampling. Correct sensor axes using the owner's physical upright observation.
- [x] Verify factory identity, original backup agreement, live partitions and
  selected app; preserve and compare Face; write only app0 and verify it.
- [x] Measure actual sensor, frame and simulation rates on hardware. Preserve
  raw evidence privately and distinguish software timing from visual feel.
- [x] Owner accepted upright location, left/right gravity direction and aesthetic.
- [x] Link the standalone chapter from the root README, run all host checks,
  inspect the change independently, and save on the separate feature branch.

Acceptance evidence and remaining physical limits live in
projects/gravity-water/BENCH.md. Other device ports are deliberately deferred.
