#!/bin/bash
# Download/extract a GitHub Actions APK and update a USB-connected phone.
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: bash scripts/install-mobile-apk.sh [--branch BRANCH | --run RUN_ID]
       [--device SERIAL] [--download-only]

Defaults to the newest Mobile APK run on the current Git branch, waits for it,
downloads/extracts the artifact and installs with adb install -r (preserves data).
Requires GitHub CLI: brew install gh; gh auth login
Overrides: R15_GITHUB_REPO=OWNER/REPO, R15_ADB=/absolute/path/to/adb
EOF
}
fail() { echo "$*" >&2; exit 1; }
branch=''; run_id=''; device=''; download_only=false
while [[ $# -gt 0 ]]; do
  case "$1" in
    --branch|--run|--device)
      [[ $# -ge 2 && -n "$2" && "$2" != -* ]] || fail "Missing value for $1"
      case "$1" in
        --branch) branch="$2" ;;
        --run) run_id="$2" ;;
        --device) device="$2" ;;
      esac
      shift 2 ;;
    --download-only) download_only=true; shift ;;
    --help|-h) usage; exit 0 ;;
    *) usage >&2; fail "Unknown argument: $1" ;;
  esac
done
[[ -z "$branch" || -z "$run_id" ]] || fail 'Choose --branch or --run.'
[[ -z "$run_id" || "$run_id" =~ ^[0-9]+$ ]] || fail 'Run ID must be numeric.'
for tool in gh git shasum curl unzip; do
  command -v "$tool" >/dev/null 2>&1 || fail "Missing $tool. See --help."
done
project_root="$(cd "$(dirname "$0")/.." && pwd)"
repo="${R15_GITHUB_REPO:-CodewaveSandy/r15-smart-cluster}"
adb="${R15_ADB:-$HOME/Desktop/learning/platform-tools/adb}"
if ! "$download_only"; then
  [[ -x "$adb" ]] || fail "ADB not executable: $adb. Set R15_ADB."
fi
echo '[1/6] Checking GitHub login and locating the build...'
gh auth status --hostname github.com >/dev/null 2>&1 || fail 'Authenticate first: gh auth login --hostname github.com'
if [[ -z "$run_id" ]]; then
  if [[ -z "$branch" ]]; then
    branch="$(git -C "$project_root" symbolic-ref --quiet --short HEAD)" || fail 'Detached HEAD: specify --branch or --run.'
  fi
  run_id="$(gh run list --repo "$repo" --workflow mobile-apk.yml --branch "$branch" \
    --limit 1 --json databaseId --jq '.[0].databaseId // empty')"
  [[ -n "$run_id" ]] || fail "No Mobile APK run for $branch. Push the workflow/changes first, then retry."
fi
workflow="$(gh run view "$run_id" --repo "$repo" --json workflowName --jq .workflowName)"
[[ "$workflow" == 'Mobile APK' ]] || fail 'Selected run is not the Mobile APK workflow.'
gh run view "$run_id" --repo "$repo" --json url,headSha,headBranch \
  --template '{{.url}}{{"\n"}}Branch: {{.headBranch}}{{"\n"}}Commit: {{.headSha}}{{"\n"}}'
echo '[2/6] Waiting for the GitHub build (completed builds continue immediately)...'
gh run watch "$run_id" --repo "$repo" --exit-status --interval 10
# Check completed runs too; watch may return immediately for an already finished run.
conclusion="$(gh run view "$run_id" --repo "$repo" --json conclusion --jq .conclusion)"
[[ "$conclusion" == success ]] || fail "Build ended with: $conclusion. No APK installed."
mkdir -p "$project_root/build/mobile-actions"
destination="$(mktemp -d "$project_root/build/mobile-actions/run-$run_id.XXXXXX")"
echo '[3/6] Looking up the APK artifact and its download size...'
artifact="$(gh api "repos/$repo/actions/runs/$run_id/artifacts?per_page=100" \
  --jq '.artifacts[] | select(.name == "r15-companion-debug" and .expired == false) | [.id, .size_in_bytes] | @tsv')"
[[ -n "$artifact" && "$artifact" != *$'\n'* ]] || fail 'Expected one unexpired APK artifact. Rerun the build if it has expired.'
IFS=$'\t' read -r artifact_id artifact_bytes <<< "$artifact"
[[ "$artifact_id" =~ ^[0-9]+$ && "$artifact_bytes" =~ ^[0-9]+$ ]] || fail 'Invalid artifact metadata.'
awk -v bytes="$artifact_bytes" 'BEGIN {printf "Downloading APK ZIP: %.1f MiB\n", bytes / 1048576}'
echo 'Live columns: % received | bytes received | average speed | elapsed time | ETA | current speed'
# Keep the token off the command line and disk. curl strips Authorization on
# cross-host redirects to GitHub artifact storage; never use --location-trusted.
gh auth token --hostname github.com | {
  IFS= read -r token
  printf 'header = "Authorization: Bearer %s"\n' "$token"
} | curl --disable --config - --location --fail --show-error \
  --proto '=https' --proto-redir '=https' --connect-timeout 30 \
  --speed-limit 1024 --speed-time 90 --retry 3 \
  --output "$destination/artifact.zip" \
  "https://api.github.com/repos/$repo/actions/artifacts/$artifact_id/zip" \
  || fail "Download failed/interrupted. Retry the script. Partial ZIP: $destination/artifact.zip"
echo '[4/6] Download complete. Extracting the ZIP...'
unzip -q "$destination/artifact.zip" -d "$destination"
echo 'Extraction complete. Verifying APK SHA-256 checksum...'
[[ -f "$destination/app-debug.apk" && -f "$destination/SHA256SUMS" ]] || fail 'Artifact is missing APK/checksum.'
expected="$(awk '$2 == "app-debug.apk" && length($1) == 64 {print $1}' "$destination/SHA256SUMS")"
actual="$(shasum -a 256 "$destination/app-debug.apk" | awk '{print $1}')"
[[ "$expected" == "$actual" ]] || fail 'APK checksum mismatch. No APK installed.'
echo "Verified APK: $destination/app-debug.apk"
if "$download_only"; then exit 0; fi

# Select only authorized USB devices, excluding emulators and network transports.
echo '[5/6] Finding an authorized USB phone...'
phones=()
while IFS= read -r serial; do
  [[ -n "$serial" ]] && phones+=("$serial")
done < <("$adb" devices -l | awk '$2 == "device" && / usb:/ && $1 !~ /^emulator-/ {print $1}')
[[ ${#phones[@]} -gt 0 ]] || fail 'No authorized USB phone found. Unlock/approve USB debugging; check adb devices -l.'
if [[ -n "$device" ]]; then
  found=false
  for serial in "${phones[@]}"; do [[ "$serial" != "$device" ]] || found=true; done
  "$found" || fail 'Requested USB phone is not authorized/connected. Check adb devices -l.'
else
  [[ ${#phones[@]} -eq 1 ]] || fail 'Connect/authorize exactly one USB phone, or use --device SERIAL. Check adb devices -l.'
  device="${phones[0]}"
fi
echo '[6/6] Transferring APK over USB and installing (large APKs can take a minute)...'
if ! "$adb" -s "$device" install -r "$destination/app-debug.apk"; then
  fail 'Install failed. A signature mismatch needs the original debug signing key; this script never uninstalls or erases app data. See mobile/docs/development.md.'
fi
echo 'Installed. Open R15 Companion on your phone.'
