#!/usr/bin/env python3
"""Apply a half-pixel coverage expansion to fresh lv_font_conv 4-bpp glyphs.

Run once immediately after conversion, never on an already processed asset.
Glyph bounds, advances, kerning and bitmap allocation remain unchanged.
"""
import argparse
from pathlib import Path
import re


def embolden(source):
    if 'Chakra weight: half-pixel' in source:
        raise ValueError('Regenerate the original font before applying weight again')
    if not re.search(r'\.bpp\s*=\s*4\b', source):
        raise ValueError('Expected an uncompressed 4-bpp LVGL font')
    match = re.search(r'(glyph_bitmap\[\]\s*=\s*\{)(.*?)(\};)', source, re.S)
    if not match:
        raise ValueError('Missing glyph bitmap')
    plain = re.sub(r'/\*.*?\*/', '', match[2], flags=re.S)
    data = bytearray(int(n, 16) for n in re.findall(r'0x[0-9a-fA-F]+', plain))
    glyphs = re.findall(r'\.bitmap_index = (\d+), \.adv_w = \d+, \.box_w = (\d+), \.box_h = (\d+)', source)
    if not glyphs:
        raise ValueError('Missing glyph descriptors')
    for offset, width, height in glyphs:
        offset, width, height = int(offset), int(width), int(height)
        count = width * height
        if offset + (count + 1)//2 > len(data):
            raise ValueError('Glyph exceeds bitmap allocation')
        pixels = [(data[offset+i//2] >> (0 if i%2 else 4)) & 15 for i in range(count)]
        for i, coverage in enumerate(pixels):
            y, x = divmod(i, width)
            # Half-pixel cardinal expansion, clipped to existing glyph bounds.
            neighbors = [coverage]
            if x: neighbors.append(pixels[i-1])
            if x+1 < width: neighbors.append(pixels[i+1])
            if y: neighbors.append(pixels[i-width])
            if y+1 < height: neighbors.append(pixels[i+width])
            expanded = (coverage + max(neighbors) + 1)//2
            index, shift = offset+i//2, 0 if i%2 else 4
            data[index] = (data[index] & ~(15 << shift)) | (expanded << shift)
    rows = ['    '+', '.join(f'0x{n:02x}' for n in data[i:i+16])+',' for i in range(0, len(data), 16)]
    bitmap = '\n    /* Chakra weight: half-pixel coverage expansion; original metrics retained. */\n'+'\n'.join(rows)+'\n'
    return source[:match.start(2)] + bitmap + source[match.end(2):]


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('font', type=Path)
    args = parser.parse_args()
    args.font.write_text(embolden(args.font.read_text()))
