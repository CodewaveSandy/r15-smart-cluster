#!/bin/bash
# Vehicle ESP32-S3 only. Build-only never opens a serial device.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
usage() {
  echo "Usage: bash scripts/flash-vehicle.sh --build-only | --list-ports | --port /dev/cu.YOUR_VEHICLE_PORT [--resume-backup /path/to/vehicle-8mb.bin]"
}
mode="${1:-}"
port=""
resume_backup=""
case "$mode" in
  --build-only|--list-ports) [[ $# == 1 ]] || { usage; exit 2; } ;;
  --port)
    [[ $# == 2 || $# == 4 ]] || { usage; exit 2; }
    port="$2"
    if [[ $# == 4 ]]; then
      [[ "$3" == --resume-backup && -n "$4" ]] || { usage; exit 2; }
      resume_backup="$4"
    fi
    ;;
  --help|-h) usage; exit 0 ;;
  *) usage; exit 2 ;;
esac
pio="${R15_PIO:-$HOME/.platformio/penv/bin/pio}"
[[ -x "$pio" ]] || { echo "PlatformIO missing: $pio" >&2; exit 1; }
if [[ "$mode" == --list-ports ]]; then
  "$pio" device list
  exit 0
fi
"$pio" run -d "$root/firmware/vehicle" -e sevensemi-s3
image="$root/build/vehicle/sevensemi-s3/firmware.factory.bin"
[[ -s "$image" ]] || { echo "Combined vehicle image missing." >&2; exit 1; }
shasum -a 256 "$image"
if [[ "$mode" == --build-only ]]; then
  echo "Vehicle build/package complete. No device accessed."
  exit 0
fi
[[ -c "$port" ]] || { echo "Serial port missing: $port" >&2; exit 1; }
python="${R15_VEHICLE_PYTHON:-$HOME/.platformio/penv/bin/python}"
esptool="${R15_VEHICLE_ESPTOOL:-$HOME/.platformio/packages/tool-esptoolpy/esptool.py}"
[[ -x "$python" && -f "$esptool" ]] || { echo "Installed esptool/Python missing." >&2; exit 1; }
baud="${R15_VEHICLE_BAUD:-460800}"
# Explicit chip and capacity prevent the 4 MB display from receiving this 8 MB image.
identity="$("$python" "$esptool" --chip esp32s3 --port "$port" --baud "$baud" --after no-reset-stub flash-id)"
printf '%s\n' "$identity"
[[ "$identity" == *"Detected flash size: 8MB"* && "$identity" != *"b8:f8:62:7f:2c:68"* ]] || {
  echo "Expected the 8 MB vehicle ESP32-S3; refusing to write this device." >&2; exit 1;
}
# Keep the stub running between tools: this board needs manual BOOT entry.
# The remaining calls sync with that stub without toggling reset lines.
continued=("$python" "$esptool" --chip esp32s3 --port "$port" --baud "$baud" --before no-reset --after no-reset-stub)
if [[ -n "$resume_backup" ]]; then
  backup="$resume_backup"
  [[ -f "$backup" && -f "$(dirname "$backup")/identity.txt" ]] || {
    echo "Resume needs the saved backup and adjacent identity.txt." >&2; exit 1;
  }
  # Never reuse another board's recovery image. Full contents are verified below.
  "$python" - "$identity" "$(dirname "$backup")/identity.txt" <<'PYIDENTITY'
import re, sys
from pathlib import Path
pattern = r"MAC:\s*([0-9a-fA-F:]{17})"
current = re.search(pattern, sys.argv[1])
saved = re.search(pattern, Path(sys.argv[2]).read_text())
if not current or not saved or current[1].lower() != saved[1].lower():
    sys.exit('Backup board identity mismatch; refusing write.')
PYIDENTITY
  echo "Resuming with existing backup: $backup"
else
  mkdir -p "$root/build/vehicle/backups"
  backup_dir="$(mktemp -d "$root/build/vehicle/backups/before-flash.XXXXXX")"
  backup="$backup_dir/vehicle-8mb.bin"
  printf '%s\n' "$identity" > "$backup_dir/identity.txt"
  echo "Saving full existing flash to $backup (this can take several minutes)..."
  "${continued[@]}" read-flash 0x0 0x800000 "$backup"
fi
[[ "$(wc -c < "$backup" | tr -d ' ')" == 8388608 ]] || { echo "Incomplete backup; refusing write." >&2; exit 1; }
"${continued[@]}" verify-flash 0x0 "$backup"
shasum -a 256 "$backup" > "$backup.sha256"
echo "Backup verified. Writing the vehicle input firmware; keep USB connected."
"${continued[@]}" write-flash --flash-size 8MB 0x0 "$image"
"$python" "$esptool" --chip esp32s3 --port "$port" --baud "$baud" --before no-reset --after hard-reset verify-flash 0x0 "$image"
echo "Vehicle firmware verified. Recovery backup: $backup"
echo "Left=4 Right=5 N=6 HighBeam=7 Gears1..6=GPIO8..13 Trip=14 Info=15; all active HIGH."
printf 'Start preview: bash scripts/preview.sh --vehicle-serial %q\n' "$port"
