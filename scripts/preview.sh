#!/bin/bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
qt_prefix="$(brew --prefix qt)"
cmake -S "$project_dir" -B "$project_dir/build" -G Ninja \
    -DCMAKE_PREFIX_PATH="$qt_prefix" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$project_dir/build"
open "$project_dir/build/r15-cluster.app"
