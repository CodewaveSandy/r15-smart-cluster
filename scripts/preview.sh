#!/bin/bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
bridge_pid=""
cleanup() {
  if [[ -n "$bridge_pid" ]]; then
    kill "$bridge_pid" 2>/dev/null || true
    wait "$bridge_pid" 2>/dev/null || true
  fi
}
trap cleanup EXIT
if [[ "${1:-}" == --phone-bridge ]]; then
  shift
  for tool in python3 openssl; do
    command -v "$tool" >/dev/null || { echo "Missing $tool" >&2; exit 1; }
  done
  bridge_host="${R15_PREVIEW_HOST:-$(python3 - <<'PYIP'
import socket
with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as connection:
    connection.connect(('192.0.2.1', 9))  # Route lookup only; no packet is sent.
    print(connection.getsockname()[0])
PYIP
)}"
  bridge_config="$root/build/phone-bridge/bootstrap.json"
  native_source="$root/tools/phone_bridge/native/Nearby.swift"
  native_app="$root/build/phone-bridge/R15 Nearby.app"
  native_helper="$native_app/Contents/MacOS/r15-nearby"
  mkdir -p "$native_app/Contents/MacOS"
  cp "$root/tools/phone_bridge/native/Info.plist" "$native_app/Contents/Info.plist"
  mkdir -p "$root/build/phone-bridge"
  if [[ ! -x "$native_helper" || "$native_source" -nt "$native_helper" || "$root/tools/phone_bridge/native/Info.plist" -nt "$native_helper" ]]; then
    swiftc "$native_source" -o "$native_helper" -framework CoreBluetooth -framework Security -Xlinker -sectcreate -Xlinker __TEXT -Xlinker __info_plist -Xlinker "$root/tools/phone_bridge/native/Info.plist"
  fi
  python3 "$root/tools/phone_bridge/bridge.py" --host "$bridge_host" --config "$bridge_config" --native-helper "$native_helper" &
  bridge_pid=$!
  for ((attempt=0;attempt<600;attempt++)); do
    kill -0 "$bridge_pid" 2>/dev/null || { echo "Phone bridge failed to start." >&2; exit 1; }
    if [[ -f "$bridge_config" ]] && python3 - "$bridge_config" "$bridge_pid" <<'PYREADY'
import json, sys
from pathlib import Path
try:
    sys.exit(0 if json.loads(Path(sys.argv[1]).read_text()).get('R15_PREVIEW_PID') == sys.argv[2] else 1)
except (ValueError, OSError):
    sys.exit(1)
PYREADY
    then break; fi
    sleep 0.1
  done
  if [[ $attempt -eq 600 ]]; then echo "Bridge startup timed out." >&2; exit 1; fi
  export R15_PHONE_BRIDGE_SOCKET="$(python3 - "$bridge_config" <<'PYSOCKET'
import json, sys
from pathlib import Path
print(json.loads(Path(sys.argv[1]).read_text())['R15_PREVIEW_SOCKET'])
PYSOCKET
)"
  echo "Phone bridge listening on $bridge_host:8765 (TLS). Use the same Wi-Fi network."
fi
cmake -S "$root" -B "$root/build/lvgl" -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build "$root/build/lvgl" --parallel 4
if [[ "$(uname -s)" == Darwin ]]; then
  "$root/build/lvgl/r15-preview.app/Contents/MacOS/r15-preview" "$@"
else
  "$root/build/lvgl/r15-preview" "$@"
fi
