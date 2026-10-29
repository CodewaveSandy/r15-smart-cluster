#!/bin/bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
python="${R15_SERIAL_PYTHON:-$HOME/.platformio/penv/bin/python}"
[[ -x "$python" ]] || { echo "Serial Python missing: $python" >&2; exit 1; }
exec "$python" "$root/tools/clock/sync_clock.py" "$@"
