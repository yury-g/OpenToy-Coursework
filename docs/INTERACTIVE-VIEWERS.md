# Interactive viewers

The READMEs are OpenToy's home. GitHub Pages runs the interactive HTML that the README itself cannot run.

| Project | Open viewer |
| --- | --- |
| ditherCam | [Flip viewer](https://yury-g.github.io/OpenToy-Coursework/dithercam/) · [Assembly guide](https://yury-g.github.io/OpenToy-Coursework/dithercam/assembly/) |
| CYD tri-fold | [A6 folding viewer](https://yury-g.github.io/OpenToy-Coursework/cyd-trifold/) · [Hinges](https://yury-g.github.io/OpenToy-Coursework/cyd-trifold/hinges.html) · [Four rounded notebooks](https://yury-g.github.io/OpenToy-Coursework/cyd-trifold/notebooks.html) |
| Animatronic Eyes 2 | [Doodle face controls](https://yury-g.github.io/OpenToy-Coursework/face-v2/) |
| Nearby Buddy | [Enclosure viewer](https://yury-g.github.io/OpenToy-Coursework/nearby-buddy/) |

These are browser simulations and CAD views. Physical test results remain in each project guide. The original HTML files still work offline.

## Updating a viewer

Edit the original project files in this repository. Merge the reviewed change into `main`; the **Publish interactive viewers** GitHub Action exports and deploys it automatically. There is no second editable website copy. The Pages root returns visitors to the GitHub README.

`tools/build_interactive_viewers.py` selects the viewers and records the source commit in [source.json](https://yury-g.github.io/OpenToy-Coursework/source.json). Add new interactive pages there and add their source paths to `.github/workflows/publish-viewers.yml`. Static images, animations, guides and source files continue to live in the README galleries.

To check an export locally, use an empty output directory:

```sh
python3 tools/build_interactive_viewers.py --output /absolute/path/to/empty/viewers
python3 -m http.server 8768 --directory /absolute/path/to/empty/viewers
```

Open a viewer path, such as `/dithercam/assembly/`. Check its rendering and controls. The exporter checks local links. The original standalone viewers preserve their interfaces and embedded assets; larger CAD views can take a moment to load.

The earlier standalone ditherCam assembly site is retained for existing links. The proposed separate courseware homepage was cancelled; ongoing viewer publishing uses GitHub Pages.

## Browser checks — November 25, 2026

All seven viewer types rendered in Chrome without JavaScript errors. Checked ditherCam Selfie, assembly Next, tri-fold Folded stack, the doodle face Angry preset, and both links between the ditherCam views. The export contains 13 HTML viewer pages including the earlier tri-fold revisions; their local links passed. These checks do not establish new hardware or physical fit results.
