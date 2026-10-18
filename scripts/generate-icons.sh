#!/bin/bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"
converter="$root/build/font-tools/node_modules/.bin/lv_font_conv"
if [[ ! -x "$converter" ]]; then
  npm ci --prefix "$root/tools/fonts" --no-audit --no-fund
  converter="$root/tools/fonts/node_modules/.bin/lv_font_conv"
fi
for size in 32 30; do
ranges="$(python3 - "$size" <<'PY'
from pathlib import Path
import hashlib,json,sys
folder=Path('assets/icons/material-design')
manifest=json.loads((folder/'manifest.json').read_text())
assert hashlib.sha256((folder/'materialdesignicons-webfont.ttf').read_bytes()).hexdigest()==manifest['fontSha256']
print(','.join('0x'+g['codepoint'] for g in manifest['glyphs'] if g['size']==int(sys.argv[1])))
PY
)"
mkdir -p assets/icons/lvgl
"$converter" --font assets/icons/material-design/materialdesignicons-webfont.ttf --range "$ranges" --size "$size" --bpp 4 --format lvgl --no-compress --lv-font-name "cluster_icons_$size" -o "assets/icons/lvgl/cluster_icons_$size.c"
done
