# Source provenance

Animatronic Eyes is Yury Gitman's continuation of OpenToy Face v1 and Face v2 within OpenToy, named November 15, 2026 after the two Feather/QT gamepad ports were installed and owner-tested.

- Original project: [yury-g/OpenToy](https://github.com/yury-g/OpenToy), MIT, copyright 2026 Yury Gitman.
- Preserved release tags: `v1.0.0-eyes` and `v2.0.0-face`.
- Starting checkout: OpenToy commit `f9402ed`, followed by the dedicated `feature/face-feather-puppetry` port.
- `shared/opentoy` preserves the original parameter structures, parsers, eye pixels and doodle spring/curve renderer without modification. `shared/Telemetry.h` preserves the checked Gravity Water USB queue. [Source paths and SHA-256](upstream-files.json) are checked by the host test runner.
- Tests retain the upstream eye pixel, doodle pixel/animation and telemetry checks; only their include paths changed. The new control tests compare all six v2 preset hashes against the original fixtures, including the happy/surprised mouth openings.

This project folder contains its required shared sources. A separate repository was briefly created by mistake; the owner clarified that only the OpenToy project listing was intended, and the extra repository was archived. Adafruit and Espressif libraries remain external dependencies, under their respective licenses; no third-party library binaries are bundled. The root LICENSE retains the upstream MIT terms.
