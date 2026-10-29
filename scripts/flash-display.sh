#!/bin/bash
# Build/package by default, then flash only when the caller invokes this script.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
port="${R15_DISPLAY_PORT:-/dev/cu.usbserial-110}"
mode="${1:-flash}"
if [[ "$mode" != flash && "$mode" != --build-only ]]; then
  echo "Usage: bash scripts/flash-display.sh [--build-only]" >&2; exit 2
fi
pio="${R15_PIO:-$HOME/.platformio/penv/bin/pio}"
[[ -x "$pio" ]] || { echo "PlatformIO executable missing: $pio" >&2; exit 1; }
"$pio" run -d "$root/firmware/display" -e crowpanel-v3
build="$root/build/display/crowpanel-v3"
python="$root/build/display-tools/bin/python"
if [[ ! -x "$python" ]]; then
  python3 -m venv "$root/build/display-tools"
fi
if ! "$python" -c 'import esptool; assert esptool.__version__ == "4.8.1"' 2>/dev/null; then
  "$python" -m pip install --disable-pip-version-check esptool==4.8.1
fi
"$python" "$root/tools/display/prepare_image.py" "$build"
shasum -a 256 "$build/r15-display.factory.bin"
if [[ "$mode" == --build-only ]]; then
  echo "Build and package checked. No device accessed."
  exit 0
fi
[[ -c "$port" ]] || { echo "Serial port missing: $port (set R15_DISPLAY_PORT if changed)." >&2; exit 1; }
# Protect the recovery image established during this board's first bring-up.
backup="${R15_FACTORY_BACKUP:-$HOME/Desktop/R15-factory-backup/crowpanel-v3-factory-rom.bin}"
"$python" - "$backup" <<'PY'
import hashlib, sys
from pathlib import Path
p=Path(sys.argv[1])
expected='babb6b68f22ddaf221760a6321a655405cc0b74abfebab1912491a17e48a50ff'
if not p.is_file() or p.stat().st_size != 0x400000 or hashlib.sha256(p.read_bytes()).hexdigest()!=expected:
    sys.exit('Verified factory backup missing or changed. Set R15_FACTORY_BACKUP to its saved location.')
PY
echo "Checking the delivered display on $port (no erase)..."
identity="$("$python" -m esptool --chip esp32s3 --port "$port" --baud 115200 --no-stub flash_id)"
printf '%s\n' "$identity"
[[ "$identity" == *"MAC: b8:f8:62:7f:2c:68"* && "$identity" == *"Detected flash size: 4MB"* ]] || {
  echo "Unexpected board identity or flash capacity; stopping before write." >&2; exit 1;
}
echo "Flashing R15 desk-test image. Keep USB connected until hash verification finishes."
"$python" -m esptool --chip esp32s3 --port "$port" --baud 115200 --no-stub \
  write_flash --flash_size 4MB 0x0 "$build/r15-display.factory.bin"
echo "R15 flash finished. Check the 2-second artwork, dashboard, touch and reset."
echo "Touch top-left: layout; top-right: theme; lower-left: trip; lower-middle: info."
echo "Tap the center to start/stop the labelled speed/RPM sweep and shift-warning test."
echo "The demo also cycles fuel and tell-tales. Tap the fuel area for shutdown; after black, tap anywhere to boot."
