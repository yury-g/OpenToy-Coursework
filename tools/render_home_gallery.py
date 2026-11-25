"""Refresh the project cards in README.md from docs/home-gallery.tsv."""
import argparse
import csv
from html import escape
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
START = "<!-- project-gallery:start -->"
END = "<!-- project-gallery:end -->"


def row(cells):
    return "| " + " | ".join(cells) + " |"


def gallery():
    with (ROOT / "docs/home-gallery.tsv").open(encoding="utf-8", newline="") as stream:
        cards = list(csv.DictReader(stream, delimiter="\t"))
    blocks = []
    for offset in range(0, len(cards), 3):
        group = cards[offset:offset + 3]
        titles, images, captions = [], [], []
        for card in group:
            title, target, image, alt, caption = (
                card[key] for key in ("title", "guide", "image", "alt", "caption")
            )
            if not (ROOT / image).is_file():
                raise ValueError(f"Missing image for {title}: {image}")
            if not target.startswith("https://") and not (ROOT / target.split("#")[0]).exists():
                raise ValueError(f"Missing guide for {title}: {target}")
            titles.append(f"[{title}]({target})")
            images.append(f'[<img src="{escape(image)}" width="300" alt="{escape(alt)}">]({target})')
            captions.append(caption)
        blocks.append("\n".join((row(titles), row(["---"] * len(group)), row(images), row(captions))))
    return "\n\n".join(blocks)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="report a stale README without changing it")
    args = parser.parse_args()
    readme = ROOT / "README.md"
    original = readme.read_text(encoding="utf-8")
    before, rest = original.split(START)
    _, after = rest.split(END)
    updated = f"{before}{START}\n\n{gallery()}\n\n{END}{after}"
    if args.check and updated != original:
        raise SystemExit("Project cards need refreshing. Run python3 tools/render_home_gallery.py")
    if not args.check and updated != original:
        readme.write_text(updated, encoding="utf-8")
