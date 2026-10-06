#!/usr/bin/env bash

set -euo pipefail

# ------------------------------------------------------------
# R15 Smart Cluster - Source Exporter
#
# Run from anywhere inside the repository:
#
#   ./scripts/export-source.sh
#
# Generates:
#
#   project-source.txt
#
# Upload that file to ChatGPT/Codex for full project context.
# ------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

OUTPUT="$PROJECT_ROOT/project-source.txt"

# Files/extensions that are useful for understanding the project.
INCLUDE_EXTENSIONS=(
    "cpp"
    "cc"
    "cxx"
    "c"
    "h"
    "hpp"
    "qml"
    "js"
    "json"
    "cmake"
    "md"
    "txt"
    "sh"
    "py"
    "yaml"
    "yml"
    "toml"
)

# Directories we never want to export.
EXCLUDED_DIRS=(
    ".git"
    "build"
    "cmake-build-debug"
    "cmake-build-release"
    ".idea"
    ".vscode"
    ".qodo"
    "node_modules"
)

cd "$PROJECT_ROOT"

echo "Generating project source export..."
echo "Project: $PROJECT_ROOT"
echo

# Start fresh.
: > "$OUTPUT"

{
    echo "============================================================"
    echo "R15 SMART CLUSTER - PROJECT SOURCE EXPORT"
    echo "============================================================"
    echo
    echo "Generated: $(date)"
    echo "Project root: $(basename "$PROJECT_ROOT")"
    echo
    echo "This file is generated automatically."
    echo "Do not edit it manually."
    echo

    echo "============================================================"
    echo "PROJECT TREE"
    echo "============================================================"
    echo

    # Portable tree-like listing without requiring `tree`.
    find . \
        \( \
            -path "./.git" -o \
            -path "./.git/*" -o \
            -path "./build" -o \
            -path "./build/*" -o \
            -path "./cmake-build-debug" -o \
            -path "./cmake-build-debug/*" -o \
            -path "./cmake-build-release" -o \
            -path "./cmake-build-release/*" -o \
            -path "./.idea" -o \
            -path "./.idea/*" -o \
            -path "./.vscode" -o \
            -path "./.vscode/*" -o \
            -path "./.qodo" -o \
            -path "./.qodo/*" -o \
            -path "./node_modules" -o \
            -path "./node_modules/*" -o \
            -name "project-source.txt" \
        \) -prune \
        -o -print | sort

    echo
    echo
    echo "============================================================"
    echo "SOURCE FILES"
    echo "============================================================"
    echo

} >> "$OUTPUT"

# ------------------------------------------------------------
# Determine whether a file should be included.
# ------------------------------------------------------------

should_include() {
    local file="$1"
    local filename
    local extension

    filename="$(basename "$file")"

    # Important extensionless project files.
    case "$filename" in
        CMakeLists.txt|Makefile|Dockerfile|AGENTS.md|SKILL.md)
            return 0
            ;;
    esac

    # Skip the generated export itself.
    if [[ "$filename" == "project-source.txt" ]]; then
        return 1
    fi

    # Extension.
    if [[ "$filename" == *.* ]]; then
        extension="${filename##*.}"
    else
        extension=""
    fi

    for allowed in "${INCLUDE_EXTENSIONS[@]}"; do
        if [[ "$extension" == "$allowed" ]]; then
            return 0
        fi
    done

    return 1
}

# ------------------------------------------------------------
# Export files.
# ------------------------------------------------------------

FILE_COUNT=0

while IFS= read -r -d '' file; do

    if ! should_include "$file"; then
        continue
    fi

    # Extra protection: skip binary files.
    if ! grep -Iq . "$file" 2>/dev/null && [[ -s "$file" ]]; then
        echo "Skipping binary: $file"
        continue
    fi

    {
        echo
        echo
        echo "############################################################"
        echo "# FILE: ${file#./}"
        echo "############################################################"
        echo

        cat "$file"

        echo
        echo
        echo "# END FILE: ${file#./}"
        echo "############################################################"

    } >> "$OUTPUT"

    ((FILE_COUNT += 1))

done < <(
    find . -type f \
        ! -path "./.git/*" \
        ! -path "./build/*" \
        ! -path "./cmake-build-debug/*" \
        ! -path "./cmake-build-release/*" \
        ! -path "./.idea/*" \
        ! -path "./.vscode/*" \
        ! -path "./.qodo/*" \
        ! -path "./node_modules/*" \
        ! -name "project-source.txt" \
        -print0 | sort -z
)

# ------------------------------------------------------------
# Summary
# ------------------------------------------------------------

SIZE="$(du -h "$OUTPUT" | cut -f1)"

echo
echo "============================================================"
echo "Done!"
echo "============================================================"
echo
echo "Files exported : $FILE_COUNT"
echo "Output          : $OUTPUT"
echo "Size            : $SIZE"
echo
echo "Upload project-source.txt to ChatGPT/Codex."