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
for spec in 'small 12 Regular' 'medium 23 SemiBold' 'large 46 Bold' 'speed 128 Bold' 'track 79 Bold' 'clock 30 SemiBold'; do
  read -r name size weight <<< "$spec"
  if [[ "$name" == small || "$name" == medium || "$name" == clock ]]; then
    "$converter" --font "assets/fonts/bricolage/BricolageGrotesque-$weight.ttf" --range 32-126,176,183,215,8212 --size "$size" --bpp 4 --format lvgl --no-compress --lv-font-name "bricolage_$name" -o "assets/fonts/lvgl/bricolage_$name.c"
  else
    "$converter" --font "assets/fonts/bricolage/BricolageGrotesque-$weight.ttf" --symbols "$(if [[ "$name" == large ]]; then printf '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ :.-'; else printf '0123456789N:.-'; fi)" --range 8212 --size "$size" --bpp 4 --format lvgl --no-compress --lv-font-name "bricolage_$name" -o "assets/fonts/lvgl/bricolage_$name.c"
  fi
done
