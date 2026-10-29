#!/usr/bin/env python3
"""Export preview GIFs/contact sheets from the host's --animate BMP output."""
import argparse
from pathlib import Path
from PIL import Image, ImageDraw

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("directory", type=Path)
args = parser.parse_args()
for kind, times in [("boot", [250, 500, 1000, 1450, 1850, 2000]),
                    ("shutdown", [100, 300, 800, 1250, 1750, 2000])]:
    paths = sorted(args.directory.glob(kind + "-motion-*.bmp"))
    expected=51
    if len(paths) != expected:
        parser.error(f"Expected {expected} {kind} motion captures; use a fresh --animate output directory")
    frames = []
    for path in paths:
        with Image.open(path) as image:
            frames.append(image.convert("RGB"))
    frames[0].save(args.directory / (kind + ".gif"), save_all=True,
                   append_images=frames[1:], duration=40, loop=0)
    sheet = Image.new("RGB", (1200, 520))
    for i, elapsed in enumerate(times):
        with Image.open(args.directory / f"{kind}-stage-{elapsed:04d}.bmp") as image:
            image.thumbnail((400, 240))
            sheet.paste(image, ((i % 3) * 400, (i // 3) * 260))
        ImageDraw.Draw(sheet).text(((i % 3) * 400 + 8, (i // 3) * 260 + 240),
                                   f"{elapsed / 1000:.3f}s", fill="white")
    sheet.save(args.directory / (kind + "-review.png"))
    print(args.directory / (kind + ".gif"))
