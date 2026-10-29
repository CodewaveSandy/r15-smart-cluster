# Command reference

Run these commands in Terminal. Start from the repository root unless a section says otherwise:

```sh
cd /Users/codewavesandy/Desktop/learning/projects/r15-smart-cluster
```

Replace `YOUR_...` placeholders with actual values before running. Build outputs, captures and backups go under ignored `build/`. Build commands can download missing dependencies. Flash commands write hardware; build-only commands do not access a device.

## Flash the display with the latest UI

Connect the Elecrow display to the Mac by USB, close serial monitors and run:

```sh
bash scripts/flash-display.sh
```

This builds the current source, packages/checks the display image, checks the saved factory backup and connected board identity, then flashes the delivered V3.0 display and verifies the write hash. Default port: `/dev/cu.usbserial-110`. Keep USB connected until verification finishes. This installs the current **desk-test UI**; display Appearance saving uses NVS; real vehicle UART and phone services are not integrated yet.

| Command | What it does |
| --- | --- |
| `bash scripts/flash-display.sh --build-only` | Builds and validates/packages the latest display firmware without accessing hardware. |
| `ls /dev/cu.*` | Lists Mac serial-device paths. Compare before/after plugging in the display to identify its port. |
| `R15_DISPLAY_PORT=/dev/cu.YOUR_DISPLAY_PORT bash scripts/flash-display.sh` | Builds and flashes the display using a different port. |
| `R15_FACTORY_BACKUP="/absolute/path/crowpanel-v3-factory-rom.bin" bash scripts/flash-display.sh` | Uses a moved factory backup; the script still checks its expected size/hash before flashing. |

After flashing: top-left tap changes layout, top-right outside the gear changes theme, center starts/stops the synthetic demo, lower-left cycles trips, lower-middle cycles info, and the fuel area previews shutdown. Tap after black to boot. The gear opens Settings when its existing speed/quality/warning gates allow it. Serial `p` selects a stationary synthetic fixture for bench Settings testing; `s` opens, `q` goes Back, `[`/`]` moves focus and Enter selects. Boot and shutdown artwork each last 2 seconds. Full serial controls and recovery instructions: [display firmware guide](../firmware/display/README.md). Factory restoration writes an older image and is documented there separately.

## Host dashboard preview and captures

| Command | What it does |
| --- | --- |
| `bash scripts/preview.sh` | Configures/builds and launches the Mac LVGL dashboard with simulated data. Esc quits outside Settings. |
| `bash scripts/preview.sh --vehicle-serial /dev/cu.YOUR_VEHICLE_PORT` | Runs the Mac UI with vehicle USB gear/lamp/button observations at 115200; other vehicle readings stay unavailable. Does not flash either board. |
| `SDL_VIDEODRIVER=dummy bash scripts/preview.sh --render build/lvgl/previews` | Builds and renders deterministic 800 × 480 scenarios without opening a window; saves BMPs and metrics. |
| `SDL_VIDEODRIVER=dummy bash scripts/preview.sh --animate build/lvgl/animation-preview` | Also captures boot/shutdown motion frames. |
| `python3 scripts/export-animation-previews.py build/lvgl/animation-preview` | Converts a complete animation capture set into GIFs/contact sheets in that directory; requires Pillow. |
| `R15_SETTINGS_PATH="$PWD/build/preview-preferences.bin" bash scripts/preview.sh` | Runs with isolated preference storage instead of your normal saved Appearance settings. |

Keyboard controls: [preview guide](getting-started.md). Render and animate runs do not load/save your normal preferences.

## Vehicle firmware

These commands target the separate vehicle ESP32, not the display. See the [vehicle guide](../firmware/vehicle/README.md) for the configured inputs and wiring limitations.

| Command | What it does |
| --- | --- |
| `bash scripts/flash-vehicle.sh --help` | Prints supported vehicle-script arguments. |
| `bash scripts/flash-vehicle.sh --build-only` | Builds/packages the vehicle discrete-input firmware without device access. |
| `bash scripts/flash-vehicle.sh --list-ports` | Lists serial ports using PlatformIO. |
| `bash scripts/flash-vehicle.sh --port /dev/cu.YOUR_VEHICLE_PORT` | Builds, checks chip/capacity, backs up and verifies current 8 MB flash, then writes/verifies vehicle firmware. Rejects the known display. |
| `R15_VEHICLE_BAUD=115200 bash scripts/flash-vehicle.sh --port /dev/cu.YOUR_VEHICLE_PORT` | Uses slower backup/flashing transfers if the default 460800 fails. USB preview remains 115200 independently. |
| `bash scripts/flash-vehicle.sh --port /dev/cu.YOUR_VEHICLE_PORT --resume-backup /absolute/path/vehicle-8mb.bin` | Resumes using a previously completed pre-write backup with adjacent saved identity; verifies it against the connected board before writing. Only use while flash still matches that backup. |

After flashing, run the `--vehicle-serial` preview above using the same port. Stop the preview before flashing again. Manual BOOT/reset recovery is described in the vehicle guide.

## Phone app and Mac connection

Requires the pinned Flutter SDK and an authorized physical Android phone connected for installation/debugging. These launchers run from the repository root.

| Command | What it does |
| --- | --- |
| `bash scripts/preview-mobile.sh --help` | Shows mobile launcher usage. |
| `bash scripts/preview-mobile.sh` | Checks the Flutter version, resolves locked dependencies and launches the companion on the single connected Android phone. |
| `bash scripts/preview-mobile.sh YOUR_ANDROID_DEVICE_ID` | Selects one phone when multiple are connected. |
| `flutter devices` | Lists device IDs and available Flutter targets. |
| `bash scripts/preview.sh --phone-bridge` | Starts the Mac BLE/TLS development bridge and dashboard; keep this Terminal running. |
| `bash scripts/preview-mobile.sh --mac-preview` | Run in a second Terminal: installs/launches the companion configured for the running Mac bridge. Phone/Mac use the same LAN; USB installs/debugs only. |
| `bash scripts/preview-mobile.sh --mac-preview YOUR_ANDROID_DEVICE_ID` | Same bridge-connected launch with explicit phone selection. |
| `R15_PREVIEW_HOST=YOUR_MAC_LAN_IPV4 bash scripts/preview.sh --phone-bridge` | Overrides automatic Mac LAN address selection. |

In Flutter's running Terminal: `r` hot reloads, `R` restarts, `q` quits. Connection setup and permissions: [Mac connection guide](../mobile/docs/mac-preview-connection.md).

## Build, test and diagnostics

Run from repository root:

| Command | What it does |
| --- | --- |
| `cmake -S . -B build/lvgl -G Ninja -DCMAKE_BUILD_TYPE=Debug` | Configures the host build, including the hash-pinned LVGL dependency and tests. |
| `cmake --build build/lvgl --parallel 4` | Builds host UI and test executables without launching a window. Configure first. |
| `ctest --test-dir build/lvgl --output-on-failure` | Runs all 14 registered host suites, including rendering and local-socket IPC. Build first. |
| `ctest --test-dir build/lvgl -N` | Lists registered test names without running them. |
| `ctest --test-dir build/lvgl -R '^gear-usb-link$' --output-on-failure` | Runs just the USB input/codec suite. Replace the expression with another listed test name to select it. |
| `python3 tests/embedded/preference-store-check.py` | Compiles/runs the actual firmware preference adapter with fake NVS/RTOS APIs to check restart, invalid data, bounded queues and storage failures; no hardware. |
| `python3 tests/embedded/display-image-check.py` | Tests partition/image packager bounds without hardware access. |
| `python3 tests/embedded/vehicle-flash-check.py` | Tests flash-script guards, backup ordering and recovery using fake tools; never flashes a device. |
| `python3 tests/host/phone-bridge-check.py` | Checks phone sessions and real local TLS/IPC; requires local socket access and OpenSSL. |
| `python3 tests/host/phone-preview-smoke.py` | Runs the Mac LVGL host, local bridge and Flutter TLS test together; requires a built Mac host, Flutter dependencies and local sockets. |
| `python3 scripts/audit-resources.py --build-dir build/lvgl --output build/lvgl/previews/resource-audit.json` | Reports compiled host asset/resource sizes; build first. These are not target SRAM/FPS measurements. |

For mobile checks, first change directory:

```sh
cd /Users/codewavesandy/Desktop/learning/projects/r15-smart-cluster/mobile
```

| Command | What it does |
| --- | --- |
| `flutter --version` | Shows SDK version; compare with `mobile/.flutter-version`. |
| `flutter doctor -v` | Diagnoses Flutter, Android and Xcode toolchain availability. |
| `flutter pub get --enforce-lockfile` | Restores dependencies using the committed lockfile. |
| `dart format --output=none --set-exit-if-changed lib test` | Checks Dart formatting without rewriting source. |
| `flutter analyze --no-pub` | Runs static analysis after dependencies are restored. |
| `flutter test --no-pub` | Runs Flutter/Dart tests. |
| `flutter run --no-pub -d YOUR_ANDROID_DEVICE_ID` | Direct app launch; use the root launcher for version checking and bridge configuration. |
| `flutter build apk --debug --no-pub` | Builds a debug APK without installing it. |
| `flutter build ios --simulator --no-pub` | Builds for iOS simulator; needs full Xcode/native setup, still a separate verification gate. |

## Asset generation and internal helpers

Return to repository root before using these. Generation commands rewrite tracked runtime assets; use them when deliberately changing the corresponding artwork/fonts.

| Command | What it does |
| --- | --- |
| `bash scripts/generate-fonts.sh` | Regenerates bundled LVGL font subsets and applies the numeral coverage adjustment. Installs locked `lv_font_conv` tooling if missing. |
| `bash scripts/generate-icons.sh` | Regenerates selected LVGL MDI icon subsets using the locked converter. |
| `python3 scripts/generate-animation-assets.py` | Rebuilds the A8 R15 wordmark and associated asset metadata from the approved source; requires Pillow. |
| `python3 scripts/embolden-font.py /absolute/path/generated-font.c` | Internal font-generation helper: rewrites a generated font's coverage. Normally invoked by `generate-fonts.sh`; do not apply repeatedly by hand. |
| `python3 tools/display/prepare_image.py build/display/crowpanel-v3` | Internal packaging helper: validates IDF outputs and creates the combined display image/manifest. Normally invoked by the display build/flash script. Requires the matching esptool environment; the script uses `build/display-tools/bin/python`. |
| `python3 tools/phone_bridge/bridge.py --help` | Lists low-level bridge arguments. Use `preview.sh --phone-bridge` for normal launch, native helper setup and cleanup. `--test-no-approval` is an isolated test fixture only. |

## Setup and path overrides

| Command/variable | Purpose |
| --- | --- |
| `xcode-select --install` | Installs Apple command-line developer tools if missing; not full Xcode. |
| `brew install cmake ninja sdl2` | Installs host-preview build prerequisites if missing. |
| `R15_PIO=/absolute/path/to/pio bash scripts/flash-display.sh --build-only` | Overrides PlatformIO location; also supported by the vehicle script. Default: `~/.platformio/penv/bin/pio`. |
| `R15_VEHICLE_PYTHON` / `R15_VEHICLE_ESPTOOL` | Vehicle-script overrides for Python/esptool locations; normally leave unset. |

For target recovery, toolchain details and wiring, use the linked authoritative guides rather than substituting flash offsets, board pins or images. Update this reference whenever adding/changing a public script or command option.

In native desk-test firmware, tapping the gear (or serial `s`) from the idle no-vehicle state explicitly selects a parked synthetic fixture and opens Settings. The header switches to `BENCH / SYNTHETIC DATA`; this is not a received zero-speed observation. Moving demo/test warnings are never replaced to grant access. Serial `b` returns to boot with unavailable observations. Real/host data-quality gates remain unchanged.
