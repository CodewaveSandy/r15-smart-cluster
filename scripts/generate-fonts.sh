#!/bin/bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"
converter="$root/build/font-tools/node_modules/.bin/lv_font_conv"
if [[ ! -x "$converter" ]]; then
  npm ci --prefix "$root/tools/fonts" --no-audit --no-fund
  converter="$root/tools/fonts/node_modules/.bin/lv_font_conv"
fi
mkdir -p assets/fonts/lvgl
# Each font is rasterized at its final logical size. No runtime font scaling.
while read -r name size source subset; do
  args=(--font "assets/fonts/$source" --size "$size" --bpp 4 --format lvgl --no-compress --lv-font-name "$name" -o "assets/fonts/lvgl/$name.c")
  if [[ "$subset" == text ]]; then
    args+=(--range 32-126,176,183,215,8212)
  elif [[ "$subset" == headline ]]; then
    args+=(--symbols '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ :.-' --range 8212)
  else
    args+=(--symbols '0123456789N:.-' --range 8212)
  fi
  "$converter" "${args[@]}"
done <<'FONTS'
inter_caption_12 12 inter/Inter-Medium.ttf text
inter_label_14 14 inter/Inter-Medium.ttf text
inter_value_16 16 inter/Inter-Medium.ttf text
inter_unit_18 18 inter/Inter-Medium.ttf text
inter_heading_23 23 inter/Inter-SemiBold.ttf text
barlow_gear_46 46 barlow-semi-condensed/BarlowSemiCondensed-SemiBold.ttf headline
barlow_speed_128 128 barlow-semi-condensed/BarlowSemiCondensed-SemiBold.ttf numeric
barlow_track_79 79 barlow-semi-condensed/BarlowSemiCondensed-SemiBold.ttf numeric
barlow_clock_30 30 barlow-semi-condensed/BarlowSemiCondensed-SemiBold.ttf numeric
FONTS
