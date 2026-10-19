#!/usr/bin/env python3
"""Audit bundled bitmap bytes and compiled host sections; never infer firmware fit."""
import argparse
import json
from pathlib import Path
import platform
import re
import shutil
import subprocess


def allocated_sections(path, size_tool):
    if platform.system() == "Darwin":
        output = subprocess.check_output([size_tool, "-m", str(path)], text=True)
        total = 0
        current_segment = None
        for line in output.splitlines():
            segment = re.match(r"Segment (\w+):", line)
            if segment:
                current_segment = segment.group(1)
            qualified = re.search(r"Section \(([^,]+), [^)]+\): (\d+)", line)
            simple = re.search(r"Section \w+: (\d+)", line)
            if qualified:
                if qualified.group(1) in {"__TEXT", "__DATA", "__DATA_CONST"}:
                    total += int(qualified.group(2))
            elif simple and current_segment in {"__TEXT", "__DATA", "__DATA_CONST"}:
                total += int(simple.group(1))
        if total == 0:
            raise RuntimeError(f"No host allocated sections recognized in {path}")
        return total
    output = subprocess.check_output([size_tool, "-A", str(path)], text=True)
    return sum(int(size) for name, size in re.findall(r"^(\.\S+)\s+(\d+)\s+", output, re.M)
               if name.startswith((".text", ".rodata", ".data", ".bss", ".sdata", ".sbss", ".eh_frame")))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=Path("build/lvgl"))
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    build = args.build_dir.resolve()
    size_tool = shutil.which("size")
    if not size_tool:
        parser.error("A host size tool is required (Xcode Command Line Tools or binutils).")
    rows = []
    for category in ("fonts", "icons", "animations"):
        for source in sorted((root / "assets" / category / "lvgl").glob("*.c")):
            match = re.search(r"(?:glyph_bitmap|image_data)\[\]\s*=\s*\{(.*?)\};", source.read_text(), re.S)
            if not match:
                raise RuntimeError(f"Cannot find glyph bitmap in {source}")
            bitmap = re.sub(r"/\*.*?\*/|//[^\n]*", "", match.group(1), flags=re.S)
            objects = list(build.glob(f"**/assets/{category}/lvgl/{source.name}.o"))
            if len(objects) != 1:
                raise RuntimeError(f"Expected one compiled object for {source.name}; build the preview first")
            rows.append({"asset": str(source.relative_to(root)),
                         "bitmap_bytes": len(re.findall(r"0x[0-9a-fA-F]+|\b\d+\b", bitmap)),
                         "host_allocated_section_bytes": allocated_sections(objects[0], size_tool)})
    report = {"scope": "Host object sections exclude debug sections; host ABI/linker results are not ESP32 flash usage.",
              "platform": platform.system(), "assets": rows,
              "total_bitmap_bytes": sum(row["bitmap_bytes"] for row in rows),
              "font_icon_bitmap_bytes": sum(row["bitmap_bytes"] for row in rows if "/animations/" not in row["asset"]),
              "animation_bitmap_bytes": sum(row["bitmap_bytes"] for row in rows if "/animations/" in row["asset"]),
              "total_host_asset_section_bytes": sum(row["host_allocated_section_bytes"] for row in rows)}
    executable = build / ("r15-preview.app/Contents/MacOS/r15-preview" if platform.system() == "Darwin" else "r15-preview")
    if executable.exists():
        report["host_executable_allocated_section_bytes"] = allocated_sections(executable, size_tool)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(f"Bitmap bytes: {report['total_bitmap_bytes']}; host asset sections: {report['total_host_asset_section_bytes']}")
    print(f"Report: {args.output}")


if __name__ == "__main__":
    main()
