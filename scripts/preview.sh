#!/bin/bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
cmake -S "$root" -B "$root/build/lvgl" -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build "$root/build/lvgl" --parallel 4
if [[ "$(uname -s)" == Darwin ]]; then
  exec "$root/build/lvgl/r15-preview.app/Contents/MacOS/r15-preview" "$@"
else
  exec "$root/build/lvgl/r15-preview" "$@"
fi
