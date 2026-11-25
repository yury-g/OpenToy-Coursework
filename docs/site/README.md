# OpenToy website

The website uses the same project catalog and viewer files as the repository. Update this repository first, then export and publish the site from that commit. The hosting checkout is a generated deployment copy.

- `index.html`, `style.css`, `site.js`: home page and project filters.
- `viewer.html`: navigation around the existing interactive viewers.
- `projects.json`: categories and viewer links keyed by the gallery image path. New gallery entries appear automatically; add metadata here to choose their filter or interactive viewer.
- `../home-gallery.tsv`: shared project names, images, captions and guide links.
- `../../tools/build_story_site.py`: static export. It copies the existing ditherCam, CYD, Nearby Buddy and Face v2 viewers and their selected assets. It records the source commit in `source.json`.

## Export

From the repository root, with Python 3 and Git available:

```sh
python3 tools/build_story_site.py --output /absolute/path/to/empty/dist
python3 -m http.server 8768 --directory /absolute/path/to/empty/dist
```

The output directory must be empty. Preview the home page at desktop and phone widths, check the filters, then open the seven viewer entries and try their controls. The original CAD viewers retain their own interfaces and browser requirements. Their meshes can take a moment to load on a phone.

## Publish with Sites

Reuse the existing Site, `appgprj_6b0722b06c748191ac8fbe36289d1714`, and its hosting checkout. Preserve its `.openai/hosting.json` and public audience. Replace only the generated `dist/` from the reviewed source revision. Follow the Sites source-helper workflow to push, package, save and deploy that exact output. Keep hosting credentials in session memory, outside Git.

The website is a manually published snapshot. Pushing GitHub does not automatically deploy it. Check `source.json` to see which revision supplied the live assets. Make changes to the repository's viewer originals and site templates, then export again; do not maintain independent edits in the hosting copy.

## Verification — November 25, 2026

The first export contains 17 project cards, seven interactive entries and eight hardware links. Local file links resolved. At 390px width the home page had no horizontal overflow. Chrome rendered a canvas in every viewer without JavaScript errors; the Face v2 Angry preset and tri-fold Folded stack control responded. These are browser checks, not new hardware or physical fit tests.
