#!/bin/bash
# Launch the companion on a connected physical Android phone.
set -euo pipefail

usage() {
  echo "Usage: bash scripts/preview-mobile.sh [android-device-id]"
  echo "Connect and unlock your phone, enable USB debugging, and approve this computer."
}
if [[ "${1:-}" == --help || "${1:-}" == -h ]]; then
  usage
  exit 0
fi
if [[ $# -gt 1 || "${1:-}" == -* ]]; then
  usage >&2
  exit 2
fi
for tool in flutter python3; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "Missing $tool on PATH. See mobile/docs/development.md for setup." >&2
    exit 1
  fi
done

project_root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$project_root/mobile"
preview_temp="$(mktemp -d "${TMPDIR:-/tmp}/r15-mobile-preview.XXXXXX")"
trap 'rm -rf "$preview_temp"' EXIT

flutter --version --machine > "$preview_temp/version.json"
python3 - "$preview_temp/version.json" .flutter-version <<'PY'
import json
import sys
from pathlib import Path

actual = json.loads(Path(sys.argv[1]).read_text()).get('frameworkVersion')
expected = Path(sys.argv[2]).read_text().strip()
if actual != expected:
    sys.exit(f'Flutter {expected} is required; found {actual}. Select the pinned SDK on PATH.')
PY

printf 'Looking for an authorized Android phone…\n'
flutter devices --machine > "$preview_temp/devices.json"
phone_id="$(python3 - "$preview_temp/devices.json" "${1:-}" <<'PY'
import json
import sys
from pathlib import Path

phones = [d for d in json.loads(Path(sys.argv[1]).read_text())
          if str(d.get('targetPlatform', '')).startswith('android')
          and d.get('isSupported', False) and not d.get('emulator', False)]
requested = sys.argv[2]
if requested:
    matches = [d for d in phones if d['id'] == requested]
    if not matches:
        sys.exit('Requested Android phone is not connected/authorized. Run flutter devices.')
    print(matches[0]['id'])
elif len(phones) == 1:
    print(phones[0]['id'])
elif not phones:
    sys.exit('No authorized Android phone found. Connect a data-capable USB cable, '
             'unlock the phone, enable USB debugging, and approve the prompt. '
             'Run flutter doctor -v if Android tooling is missing.')
else:
    print('Multiple Android phones found. Run this script with one device ID:', file=sys.stderr)
    for device in phones:
        print(f"  {device['name']}  {device['id']}", file=sys.stderr)
    sys.exit(1)
PY
)"

printf 'Preparing dependencies…\n'
flutter pub get --enforce-lockfile
printf 'Launching R15 Companion. Press r to hot reload, R to restart, q to quit.\n'
flutter run --debug --no-pub -d "$phone_id"
