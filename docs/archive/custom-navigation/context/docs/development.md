# Development, wiring and commands

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

This builds the current source, packages/checks the display image, checks the saved factory backup and connected board identity, then flashes the delivered V3.0 display and verifies the write hash. Default port: `/dev/cu.usbserial-110`. Keep USB connected until verification finishes. This installs the current **desk-test UI**; display Appearance saving uses NVS; one-way discrete-input UART uses vehicle GPIO17 → display GPIO_D IO38; authenticated BLE pairing/reconnect and phone name/battery/time metadata are implemented; full telemetry/navigation/Wi-Fi services remain pending.

| Command | What it does |
| --- | --- |
| `bash scripts/sync-clock.sh /dev/cu.usbserial-10` | Keeps display time/date synced to the Mac and its current local timezone over USB; leave running, Ctrl+C stops. Replace the port if changed. Stop before flashing or another serial console. |
| `bash scripts/sync-clock.sh --help` | Shows clock-sync helper usage. |
| `bash scripts/flash-display.sh --build-only` | Builds and validates/packages the latest display firmware without accessing hardware. |
| `screen /dev/cu.usbserial-110 115200` | Opens the display USB serial console for `v` live UART, `p` parked Settings, and diagnostics. Change the port if needed; quit with Ctrl+A, then backslash, then `y` before flashing. |
| `ls /dev/cu.*` | Lists Mac serial-device paths. Compare before/after plugging in the display to identify its port. |
| `R15_DISPLAY_PORT=/dev/cu.YOUR_DISPLAY_PORT bash scripts/flash-display.sh` | Builds and flashes the display using a different port. |
| `R15_FACTORY_BACKUP="/absolute/path/crowpanel-v3-factory-rom.bin" bash scripts/flash-display.sh` | Uses a moved factory backup; the script still checks its expected size/hash before flashing. |

After flashing: top-left tap changes layout, top-right outside the gear changes theme, center starts/stops the synthetic demo, lower-left cycles trips, lower-middle cycles info, and the fuel area previews shutdown. Tap after black to boot. The gear opens prototype Settings without speed/quality/warning restrictions. Serial `p` selects a stationary synthetic fixture for bench Settings testing; `s` opens, `q` goes Back, `[`/`]` moves focus and Enter selects. Boot and shutdown artwork each last 2 seconds. Full serial controls and recovery instructions: [display firmware guide](../firmware/display/README.md). Factory restoration writes an older image and is documented there separately.

## Host dashboard preview and captures

| Command | What it does |
| --- | --- |
| `bash scripts/preview.sh` | Configures/builds and launches the Mac LVGL dashboard with simulated data. Esc quits outside Settings. |
| `bash scripts/preview.sh --vehicle-serial /dev/cu.YOUR_VEHICLE_PORT` | Runs the Mac UI with vehicle USB gear/lamp/button observations at 115200; other vehicle readings stay unavailable. Does not flash either board. |
| `SDL_VIDEODRIVER=dummy bash scripts/preview.sh --render build/lvgl/previews` | Builds and renders deterministic 800 × 480 scenarios without opening a window; saves BMPs and metrics. |
| `SDL_VIDEODRIVER=dummy bash scripts/preview.sh --animate build/lvgl/animation-preview` | Also captures boot/shutdown motion frames. |
| `python3 scripts/export-animation-previews.py build/lvgl/animation-preview` | Converts a complete animation capture set into GIFs/contact sheets in that directory; requires Pillow. |
| `R15_SETTINGS_PATH="$PWD/build/preview-preferences.bin" bash scripts/preview.sh` | Runs with isolated preference storage instead of your normal saved Appearance settings. |

Keyboard controls: [preview guide](development.md#host-setup-and-controls). Render and animate runs do not load/save your normal preferences.

## Vehicle firmware

These commands target the separate vehicle ESP32, not the display. See the [vehicle guide](../firmware/vehicle/README.md) for the configured inputs and wiring limitations.

| Command | What it does |
| --- | --- |
| `bash scripts/flash-vehicle.sh --help` | Prints supported vehicle-script arguments. |
| `bash scripts/flash-vehicle.sh --build-only` | Builds/packages the vehicle discrete-input firmware without device access. |
| `bash scripts/flash-vehicle.sh --list-ports` | Lists serial ports using PlatformIO. |
| `bash scripts/flash-vehicle.sh --port /dev/cu.YOUR_VEHICLE_PORT` | Builds, checks chip/capacity, backs up and verifies current 8 MB flash, then writes/verifies vehicle firmware. Rejects the known display. |
| `R15_VEHICLE_BAUD=115200 bash scripts/flash-vehicle.sh --port /dev/cu.YOUR_VEHICLE_PORT` | Explicitly selects the115200 backup/flashing default; faster rates require a reliable link. USB preview remains 115200 independently. |
| `bash scripts/flash-vehicle.sh --port /dev/cu.YOUR_VEHICLE_PORT --resume-backup /absolute/path/vehicle-8mb.bin` | Resumes using a previously completed pre-write backup with adjacent saved identity; verifies it against the connected board before writing. Only use while flash still matches that backup. |

After flashing, run the `--vehicle-serial` preview above using the same port. Stop the preview before flashing again. Manual BOOT/reset recovery is described in the vehicle guide.

## LVGL map render preview

```sh
bash scripts/preview.sh --map-preview
# Headless, deterministic 800 × 480 captures plus runtime diagnostics:
SDL_VIDEODRIVER=dummy bash scripts/preview.sh --map-preview --render build/lvgl/map-previews
```

This is a separate native LVGL 9.2.2/SDL executable (`r15-map-preview`), not a screenshot mockup and not board firmware. It uses the existing Header, compact PrimaryReadout, TellTaleRow and LifecycleOverlay components above a vector MapPanel. The normal dashboard/settings launcher and firmware entry points are unchanged. It does not load/save preferences, open serial/radios, call a provider, or request location. Synthetic local-metre geometry and route progress live only in `simulator/lvgl/MapDemo.h`; the screen always labels the map/sample route and never populates bike speed/gear or wall-clock time from the animation.

| Control | Preview behavior |
| --- | --- |
| D / Tab | Midnight↔Daylight / Street↔Track instrument configuration |
| Space / R | Pause/resume the sample position / restart the route |
| V | Follow↔whole-route overview; both are north-up |
| + or = / − | Three fixed follow zoom levels |
| X / U | Stale position (hide marker/pause guidance) / unavailable map |
| W | Simultaneous sample engine/coolant warnings in the preserved tell-tale row |
| O / Esc | Existing two-second ignition lifecycle / quit the preview |

Follow pans a fixed local-plane viewport with the current sample pose; overview fits the entire route. Cached roads and the route remain visible when position is stale, but the marker/ETA guidance is withdrawn; unavailable map hides the scene. Data validity, clipping, road/route geometry, theme styles and fixed capacities are separate from the host animation. No provider, global geographic projection, route search, real maneuver generation, BLE/UART map contract, persistent tile cache, heading-up camera or physical-board performance is established by this preview.

Verification on 2026-11-02: host builds passed with warning flags enabled. Deterministic captures cover both themes/instrument configurations, follow/overview, moved pose, close zoom, arrival, stale/unavailable, simultaneous warnings and boot/shutdown with warning interruption. LVGL diagnostics reported zero warnings; repeated identical paused frames produced zero flushes in all four theme/layout combinations. The synthetic fixture has 20 roads, seven route points and originally used a 2,956-byte bounded scene value; the real-area importer now uses a 204,828-byte-capacity scene (the host and renderer retain their own copies). These sizes exclude LVGL objects/draw tasks/framebuffers/fonts; they are not ESP32 heap/FPS evidence. Native GUI controls for restart/pause, overview, theme/layout and warnings/stale position were manually inspected. No automated test files/suites were created or run. Physical display build/flash, resource measurements, map delivery, provider licensing and B3 acceptance remain pending.

### Real Kolkata OpenStreetMap roads

```sh
# First use only: one small Overpass download if the raw snapshot is absent.
python3 scripts/import-osm-map.py --download
# Reopening the preview is offline; it never contacts a map server:
bash scripts/preview.sh --kolkata-map
SDL_VIDEODRIVER=dummy bash scripts/preview.sh --kolkata-map --render build/lvgl/kolkata-map-previews
```

The selected central-Kolkata bounding box is south/west/north/east `22.546,88.337,22.574,88.365` (roughly 3.1 × 2.9 km), with a local rendering origin at `22.56,88.351`. This is one selected area, not all of Kolkata. Actual OSM road names/geometry include Chowringhee/Jawaharlal Nehru Road, Park Street, Esplanade roads, Mayo Road and surrounding lanes. There is **no calculated route, maneuver/ETA, GPS marker or simulated movement** in this mode. Arrows pan in 100m steps, R recenters, V toggles area overview, +/− zoom, D changes theme; Tab/U/W/O/Esc retain layout/unavailable/sample-warning/lifecycle/quit controls. The synthetic `--map-preview` remains separate.

`import-osm-map.py` uses Python standard-library tools only. It caches the source at ignored `build/maps/kolkata/osm-roads.json`, projects a small-area spherical approximation into local metres, clips to the bounding box, simplifies with a 2m tolerance and splits polylines into at most eight points. All supported fetched road/path classes are retained; service roads and paths are visually omitted at overview scale and drawn thinner close up. Named arterial roads are prioritized; duplicate/overlapping labels are suppressed with a fixed eight-label limit. No road class is a claim of legal motorcycle access. An import exceeding 2,048 chunks fails explicitly instead of silently losing geometry. Road names prefer `name:en`; unsupported characters are normalized/omitted and names longer than 23 ASCII characters are shortened, with counts recorded in provenance. Full Bengali labels require a separate font/text plan.

The host text adapter loads `kolkata.r15map` outside rendering, checks format/version/file size/counts/coordinates/names and atomically supplies a validated scene. The generated `.provenance.json` records query, geographic bounds/origin, OSM timestamp, simplification, counts and source SHA-256; the `.LICENSE.txt` records ODbL/copyright. Source/derived map data remain ODbL data, not firmware assets or public domain. Keep provenance/licence with any redistributed map extract. The screen displays © OpenStreetMap contributors and the copyright/licence URL; a tiny locally generated Inter copyright glyph avoids adding a full Unicode font. The import command reuses an existing source and does not poll Overpass during interaction. Use an explicitly different `--source`/`--output` for a fresh snapshot. The default download is a bounded Overpass POST with a project User-Agent, a timeout and an 8 MiB source ceiling.

Verified 2026-11-02 snapshot: OSM timestamp `2026-11-02T15:08:51Z`, source SHA-256 `38e7324cfa61598decd972af89135a4008736e9a7ae4fc07ed270805804c04b5`; 1,384 input ways -> 1,435 chunks / 4,494 points, with 35 normalized/shortened names. The host scene capacity is 204,828 bytes, and the host/renderer retain separate copies. Host C++ builds and 800 × 480 rendering in both themes/instrument configurations passed; LVGL warnings and unchanged-frame flushes were zero. Map/overview/pan/zoom, labels, unavailable scenes, warning visibility and lifecycle captures were manually inspected. No automated suites, physical display flash, GPS, live routing or phone/UART map transfer occurred. MCU memory/CPU/scanout suitability remains unverified.

## Phone app and Mac connection

For a Mac without the local mobile toolchain, every push builds a mobile debug APK in GitHub Actions. Run `bash scripts/install-mobile-apk.sh` to wait/download/install it using the existing USB ADB; see [setup and signing](../mobile/docs/development.md#build-on-github-install-from-the-mac). Firmware builds remain local and separate.

The local Flutter launchers below require the pinned Flutter SDK and an authorized physical Android phone connected for installation/debugging. These launchers run from the repository root.

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

In Flutter's running Terminal: `r` hot reloads, `R` restarts, `q` quits. Connection setup and permissions: [Mac connection guide](../mobile/docs/development.md#mac-preview-adapter).

## Build and diagnostics

Run from repository root:

| Command | What it does |
| --- | --- |
| `cmake -S . -B build/lvgl -G Ninja -DCMAKE_BUILD_TYPE=Debug` | Configures the host build, including the hash-pinned LVGL dependency. |
| `cmake --build build/lvgl --parallel 4` | Builds the host UI executable without launching a window. Configure first. |
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
| `dart format --output=none --set-exit-if-changed lib` | Checks Dart formatting without rewriting source. |
| `flutter analyze --no-pub` | Runs static analysis after dependencies are restored. |
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
| `~/.platformio/penv/bin/python tools/clock/sync_clock.py --help` | Internal clock helper CLI; normally use `bash scripts/sync-clock.sh PORT`. |
| `python3 tools/phone_bridge/bridge.py --help` | Lists low-level bridge arguments. Use `preview.sh --phone-bridge` for normal launch, native helper setup and cleanup. `--test-no-approval` is an isolated test fixture only. |

## Setup and path overrides

| Command/variable | Purpose |
| --- | --- |
| `xcode-select --install` | Installs Apple command-line developer tools if missing; not full Xcode. |
| `brew install cmake ninja sdl2` | Installs host-preview build prerequisites if missing. |
| `R15_PIO=/absolute/path/to/pio bash scripts/flash-display.sh --build-only` | Overrides PlatformIO location; also supported by the vehicle script. Default: `~/.platformio/penv/bin/pio`. |
| `R15_VEHICLE_PYTHON` / `R15_VEHICLE_ESPTOOL` | Vehicle-script overrides for Python/esptool locations; normally leave unset. |

For target recovery, toolchain details and wiring, use the linked authoritative guides rather than substituting flash offsets, board pins or images. Update this reference whenever adding/changing a public script or command option.

Tapping the gear or serial `s` opens Settings without substituting demo observations; live UART remains live. Serial `p` is an optional explicitly labelled bench fixture.

For the connected two-board test, follow the [UART flashing and acceptance sequence](../firmware/display/README.md#flash-and-verify). Display serial `v` selects live UART; `p` explicitly selects parked Settings demo; Settings is accessible directly in live mode; the gear color indicates connection and the header date remains the local date.

## Vehicle BLE pairing bench

Follow [the wiring and Android test procedure](development.md#vehicle-ble-uart-bench) before flashing both targets. Use the real companion for current metadata acceptance; nRF Connect is an optional separate pairing diagnostic and must release the sole BLE slot before companion checks. Build without touching hardware:

```bash
bash scripts/flash-vehicle.sh --build-only
bash scripts/flash-display.sh --build-only
```

Both commands build/package firmware without opening a device. Automated test files and their runner commands were removed on 2026-10-31; use manual verification, static analysis and affected builds until the user resumes test development.


## Host setup and controls

Install the prerequisites listed above, then run `bash scripts/preview.sh` from the repository root. The preview is an 800 × 480 development surface; its vehicle observations are simulated unless a specific live adapter is selected.

| Key | Action |
| --- | --- |
| Left / Right | Speed -/+ 1 km/h |
| Down / Up | RPM -/+ 100 |
| [ / ] | Fuel -/+ 1 percentage point |
| 0–6 | Neutral or gear |
| Tab | Street / Track |
| D | Midnight / Daylight |
| U | Metric / imperial speed and distance |
| Enter | Street only: ODO / Trip 1 / Trip 2; Track keeps BEST LAP fixed |
| I | Info list: coolant / Track mode / real-time fuel / average fuel / average speed / TCS / unidentified |
| T | Toggle selected TCS (simulator only) or Track mode |
| L / R | Exclusive left / right simulated indicator |
| Z | Hazards |
| H | High beam |
| A / E / C | ABS / engine / coolant warning |
| X | Toggle unavailable observations |
| O | Simulated key-off / key-on |
| Space | Reset demo values |
| F1 | Print controls in Terminal |
| P / S | Stationary neutral fixture / open Settings |
| F3 / F4 | Fail next Bluetooth request / toggle service availability |
| F5 / F6 | Fail next Wi-Fi request / toggle service availability |
| F2 | Illuminate all icons steadily; press again to restore normal status |
| Backspace / Esc | Back in Settings; Esc quits outside Settings |

Fuel: 10–20% yellow, below 10% red, blinking below 20%. RPM: steady shift alert from 10,500; flashing from 11,400. These are UI demonstration thresholds, not verified vehicle limits. Clock/date uses the Mac's local wall clock; deterministic renders use fixed time.

O simulates ignition; the Mac window stays open after shutdown so O can restart it. It does not cut power. A valid engine/coolant warning exposes instrumentation during boot/shutdown artwork. Real ignition, retained power and final critical-warning classification require hardware design/testing.


## Manual preview workflow

Use the render/animate commands above with isolated ignored output folders; they are development previews with runtime diagnostics, not authorization to restore automated test suites. Inspect native 800 × 480 states in both layouts/themes, warnings/unavailable data, Settings focus/back, Phone pairing/reconnect and lifecycle interruptions. Boot and shutdown artwork each lasts exactly 2.00 seconds; total panel startup and retained power require physical measurements. Settings remains unrestricted by speed/quality/UART/warnings in this prototype.

Tune shared theme/metrics in `embedded/ui/Tokens.h`, geometry in `Geometry.h`, font roles in `Typography.h`, components in `embedded/ui/components`, formatting in `embedded/presentation`, semantic input/lifecycle in `embedded/application` and host scenarios in `simulator/lvgl`. Follow [UI style](ui.md#ui-settings-and-lifecycle) and [lifecycle](ui.md#power-lifecycle); do not redesign approved screens. Fonts/icons/assets ship locally with their manifests/licenses; use the generation commands above rather than substituting fonts. Captures, exports, resource reports and isolated preferences stay ignored under `build/`. Actual target memory/render evidence belongs to [architecture](architecture.md#resource-accounting-and-target-verification).

## Hardware wiring and observed checks

This is the physical connection reference, to be completed as hardware connections are confirmed. Unknown terminals, pin numbers and component specifications remain explicit gaps. The [architecture](architecture.md#system-architecture-status-and-roadmap) defines MCU responsibilities; [power lifecycle](ui.md#power-lifecycle) defines ignition and retained-power requirements.

### Display power: engine-off bike test

**Reported by the user on 2026-10-24:** the display successfully powered on from the bike socket with the **engine off**.

```text
Bike socket power cables (reported 12 V)
                  |
                  v
          Buck converter input
          Output set to 5 V
                  |
                  v
          Display power input
```

| Step | User-reported setup / result |
| --- | --- |
| 1 | Bike socket 12 V power cables connected directly to the buck converter input |
| 2 | Buck converter output set to 5 V |
| 3 | Buck converter output connected to the display power input |
| 4 | Display powered on successfully; engine remained off |

This is the observed power path, not a connector pinout or an approved final power circuit. Positive/return terminals, wire colors and the display input connector have not yet been recorded. Evidence is the user's report; actual voltage measurements, instrument used, current, duration and firmware version were not supplied. Engine-start, cranking, engine-running, road use, telemetry and key-off shutdown were not validated by this test.

### Details to complete the wiring guide

#### Configurable discrete input test

Latest user report on 2026-10-24 supersedes earlier gear/polarity assignments: OUT1 is **left indicator → GPIO4**, OUT2 is **right indicator → GPIO5**; active approximately **2–3.3 V**, inactive **0.05–0.2 V**. Active HIGH decoding is implemented. The lower active reading is below ESP32-S3's guaranteed HIGH threshold at a 3.3 V supply and needs electrical resolution; probe/reference/instrument and individual channel peaks remain unrecorded. The all-enabled bench allocation is N GPIO6, high beam GPIO7 and gears 1–6 GPIO8–13, using weak pull-downs so opto outputs can move between inputs without reflashing. See the authoritative [vehicle input guide](../firmware/vehicle/README.md) for configuration, voltage limits and separate conditioning requirements. Board schematic/module validation remains pending.

| Connection / component | Still to record or verify |
| --- | --- |
| Bike socket | Identity/location, connector orientation and pin numbering, positive/return identification and key-on/key-off voltage behavior |
| Buck converter | Model/photo, input range, continuous output current, adjustment method and protection specifications |
| Display supply | Exact power connector/terminal labels, polarity and revision-matched input requirements |
| Measurements | Input/output voltage under display load, current, instrument, test conditions and duration |
| Wiring / protection | Actual fuse arrangement, reverse-polarity/transient protection, wire/connector ratings, insulation and mechanical securing |
| Ignition / shutdown | Separate ignition observation, retained supply and measured shutdown margin; none confirmed by this test |
| Vehicle MCU / UART / signals | MCU identity, power/ground topology, reviewed logic levels and terminal mappings; connections pending |

Before extending testing to engine start/running, review the actual converter and supply protection and define measurements for those conditions. The reported 12 V source and 5 V setting do not establish cranking or running behavior. Initial signal integration remains passive/read-only with OEM functionality retained.

A supply that disappears at key-off cannot provide the required two-second shutdown animation without retained power. This power-on result does not establish retention; see the [shutdown power contract](ui.md#shutdown-power-contract).

### Agreed two-MCU bench connection

Both boards are powered separately from Mac USB. User identifies display connectors UART0 (4-pin), GPIO_D (4-pin), I2C (4-pin), SPK (2-pin), and a supplied 4-pin socket-to-loose-wire cable. Connector fit does not identify signal order. Verify V3.0 schematic, orientation, logic levels and USB/UART0 bridge sharing before assigning TX/RX/GND. Leave the cable power conductor disconnected and insulated. Final installation uses separate supply branches and dedicated power control as defined in [power lifecycle](ui.md#agreed-power-topology); the vehicle ESP coordinates rather than supplying display load current.

### Direct UART bench wiring (2026-10-29)

The user supplied a rear photo of the 7SEMI ESP32-S3 DevKit showing GPIO17 and GND and reports completing the following wiring. Both boards remain separately powered by Mac USB. This records the reported connection, not an independently measured electrical test.

| Vehicle header | Display GPIO_D cable |
| --- | --- |
| GPIO17 (dedicated UART1 TX) | IO38 (dedicated UART1 RX) |
| GND | GND |
| No connection | 3V3 and NC; individually insulate loose leads |

Use GPIO_D, not UART0. The official V3.0 schematic connects UART0 RX/TX in parallel with the CH340C USB bridge through its series resistors. GPIO38 is exposed on J7 GPIO_D and is unused by our RGB/touch adapter; assigning UART1 RX there preserves the USB console. Source: [Elecrow V3.0 schematic](https://raw.githubusercontent.com/Elecrow-RD/CrowPanel-7.0-HMI-ESP32-Display-800x480/master/Eagle_SCH%26PCB/V3.0/CrowPanel%20ESP32%20Display-7.0-inch-V3.0-20240420.sch). Vehicle GPIO17 is a software assignment to the photographed accessible header, not the vehicle board's printed TX pin. InputConfig rejects overlap with configured acquisition inputs.

Power both boards off before changing wiring. Disconnect both signal and ground cable leads before flashing/powering just one board to avoid signal-line back-powering; reconnect with both boards off, then power both. No bike harness or power lead is required for the first link test. Keep this short 3.3 V logic UART connection on the desk; a vehicle installation still requires reviewed physical-layer, power and environmental design.

User bench result (2026-10-29): both updated images were user-flashed. Initial UART WAITING was traced by the user to a loose connection; reseating it produced UART / LIVE. This confirms receipt of valid packets over the reported vehicle GPIO17 → display GPIO_D IO38/GND link with separate Mac USB power. Individual input stimulus, cable-loss/stale recovery, independent resets, soak and vehicle use remain separate pending tests.

User bench acceptance (2026-10-29): the user reports the display follow-up working and confirms the described jumper-input tests working over the physical UART link. This covers the tested neutral/gear, left/right/high-beam and Trip/Info behavior, plus functional acceptance of the current display changes. This is user-reported desk evidence, not instrumented timing, electrical-margin, power-loss/RTC, soak or bike verification. Mac USB clock synchronization remains the selected time source; no RTC hardware change is planned in this phase.

## Vehicle BLE UART bench

Implemented in firmware; first numeric-comparison pairing and saved-bond reconnect after powering both boards off/on are user bench-confirmed (2026-10-30). Vehicle owns BLE and bond storage; display owns the approval UI. The real Android companion now supports this connection and authenticated phone name/battery/time metadata; core behavior is user-confirmed. Wi-Fi, vehicle-to-phone telemetry, navigation, media and maps remain pending. Mac preview remains a separate development adapter.

### Wiring and flashing

Power off and unplug both boards before changing leads. Keep the working vehicle GPIO17 → display GPIO_D IO38 wire and shared GND. Add **display UART0 TX GPIO43 → vehicle GPIO18** using the second connector cable. Identify TX from the PCB labels/schematic, not cable color or viewing orientation. Insulate its RX GPIO44, 3V3 and unused GND leads. Both boards retain separate USB power; never join their power rails.

UART0 RX44 shares the display USB bridge transmitter: do not connect a vehicle transmitter there. TX43 is the existing console output and also feeds the USB bridge receiver. Firmware sends framed control messages through that same output without reassigning the pin. The GPIO_D receive path stays unchanged.

Stop the clock helper and serial monitors. Disconnect inter-board leads before separately flashing each board. Ports can change; list them first with `bash scripts/flash-vehicle.sh --list-ports`.

```bash
R15_VEHICLE_BAUD=115200 bash scripts/flash-vehicle.sh --port /dev/cu.usbserial-0001
R15_DISPLAY_PORT=/dev/cu.usbserial-10 bash scripts/flash-display.sh
```

For the vehicle's manual download mode, hold BOOT, tap RESET, and release BOOT once the flashing stub starts. Reconnect leads with both boards powered off, then power both by USB. The gear icon should show a live vehicle connection; Settings → Phone should offer Pair phone. Optional clock sync:

```bash
bash scripts/sync-clock.sh /dev/cu.usbserial-10
```

### Android acceptance

Use [Nordic nRF Connect for Android](https://github.com/nordicsemi/Android-nRF-Connect) for this bench test. Android's ordinary Bluetooth settings may not list this BLE peripheral.

1. Select Pair phone on the cluster. This opens a 60-second enrollment window.
2. Scan for **R15 Cluster**, connect and request Bond in nRF Connect.
3. Compare the actual six-digit code on Android and the cluster. Confirm only when both match. The cluster defaults to Reject; select Allow within 30 seconds and approve on Android.
4. Expect confirmed connected state only after authentication and bond completion. Current Phone settings show the delivered name and battery percentage (or unavailable fallbacks), replacing the old authenticated-bond status sentence. A connected GATT link alone is insufficient.
5. Disconnect retains the bond. Reconnect from the phone and test after power cycling both boards. Forget explicitly removes the vehicle's saved bond; remove the corresponding Android bond too before pairing again.

Also test Reject, both timeouts, a missing return wire, UART loss during approval, and GPIO indicators/navigation while BLE is busy. Existing telemetry must continue and stale connection state must remain visible. Do not claim radio, panel performance or bike verification from host tests.

### Implemented boundaries

NimBLE uses authenticated LE Secure Connections with a 128-bit key, numeric comparison, one connection and one saved bond. Legacy pairing, debug keys and automatic bond replacement are disabled. Fresh display heartbeat and an explicit enrollment action are required for a new bond. Pending approval is cancelled on display loss. An established authenticated connection may remain connected when display communication is lost. The optional enrollment phone-name GATT service accepts a bounded display label; no vehicle-control or navigation/media commands are exposed. See [the phone-name contract](../mobile/docs/protocol.md#enrollment-name-c011).

Bonds persist in vehicle NVS across resets/power cycles. Storage errors make the service unavailable; firmware never erases NVS to recover. Development NVS is not encrypted or protected against physical readout. Reflashing a combined factory image may overwrite NVS; persistence across firmware updates is not promised.

The BLE host owns all radio/storage calls. A bounded four-command queue expires entries after one second. Host health expires after one second independently of UART telemetry. UI never calls BLE APIs or mutates confirmed connection state optimistically.

### Narrow UART sideband v1

The existing 36-byte R15G v3 telemetry remains unchanged. A fixed 48-byte R15P v1 stream carries phone status vehicle→display and phone controls display→vehicle at 115200 8N1. Shared codec: `shared/protocol/PhoneLink.h`; normative protocol vectors: `shared/protocol/phone-link-v1-vectors.txt`.

| Bytes | Encoding |
| --- | --- |
| 0–3 | ASCII R15P |
| 4, 5 | Version 1; kind Status=1 / Control=2 |
| 6 | Flags: paired, connected, requested, approved, enrollment, command failed (bits 0–5) |
| 7 | Result None=0 / Queued=1 / Rejected=2 |
| 8–11, 12–15 | Vehicle boot session, display boot session |
| 16–19, 20–23 | Sender sequence, request ID |
| 24–27, 28–31 | Pairing challenge nonce, numeric code 0–999999 |
| 32 | Heartbeat=0 / Pair=1 / Approve=2 / Reject=3 / Disconnect=4 / Forget=5 |
| 33 | Status quality Available=0 / Unavailable=1 / Stale=4; control=0 |
| 34–35 | Reserved zero |
| 36–39 | Source monotonic uptime milliseconds modulo 2^32 |
| 40–43 | Status: current per-connection identifier; Control: zero |
| 44–47 | CRC-32/ISO-HDLC over bytes 0–43 |

All integers are little endian; no native structs are transmitted. CRC uses reflected polynomial 0xEDB88320, initial/final XOR 0xFFFFFFFF. Receivers use bounded sliding buffers, strict field validation and session/sequence/uptime ordering. CRC detects corruption; it does not authenticate physical UART attackers.

Status and heartbeat cadence is 200 ms; freshness is 1,000 ms. Controls require an established matching session and fresh heartbeat. Requests retry every 200 ms with a 1,500 ms local deadline; vehicle deduplicates IDs. Approve/Reject must match the current random pairing nonce. Queued acknowledges queue admission only; subsequent status confirms the result. Reboots invalidate prior-session commands. RX overflow invalidates freshness without clearing accepted-request deduplication.

Display console output includes binary control frames followed by LF alongside diagnostic text. LF conversion is disabled to preserve bytes. Vehicle accepts only validated Control frames; clock helper tolerates binary console lines. This is a bench sideband, not the final full telemetry/map protocol.

### Diagnosing unavailable Phone status

The normal gear color proves only the R15G telemetry path. Phone distinguishes missing status packets, a mismatched vehicle session, an unconfirmed return UART and a vehicle BLE service fault. “Return UART not confirmed” means the vehicle has not echoed this display boot session; check TX43 → GPIO18 and the existing common ground. One shared GND is sufficient. Repeated Android status 19 disconnects before enrollment are consistent with the vehicle rejecting an unapproved connection; they do not establish a radio failure.

BLE health age is measured when the snapshot is observed, accounting for the host task publishing after the acquisition loop timestamp was captured. A newer concurrent publication must not be reported unavailable. Retain the one-second stale-health boundary; earlier regression evidence is historical because automated suites are currently removed. Subsequent user bench evidence confirms first pairing and a saved-bond reconnect; prolonged/concurrent load testing remains pending.

If the return channel is confirmed but BLE becomes unavailable during connection, capture the bounded vehicle fault diagnostic over its USB console. Stop other serial monitors first. With the current native USB vehicle port:

```bash
/Users/codewavesandy/.platformio/penv/bin/python tools/vehicle/ble_log.py /dev/cu.usbmodem1101
```

The reader sends no commands and filters binary input snapshots out. Start it before selecting Pair phone and connecting Android. `@BLE FAULT` reports only an operation, numeric result/object type or health age, never keys or numeric-comparison codes. Faults are latched/rate-limited; if already emitted, start the reader before powering the vehicle and reconnect if USB enumeration changes. This diagnostic identifies a failure; it does not establish its cause without the reported result.

Advertising restarts only when synchronization, disconnect, a failed connection or advertising completion explicitly requests it. In pinned ESP-IDF 5.5.4 NimBLE, the sole connection slot can be reserved before CONNECT reaches the application (remote-feature exchange precedes that callback). Periodic polling must not restart advertising during this interval: `ble_gap_adv_start` returns `BLE_HS_ENOMEM` (6) when no connection slot remains. Preserve this explicit restart ownership rather than inferring that inactive advertising means a free slot. This is not evidence of general heap exhaustion. Subsequent first pairing and saved-bond reconnect are user bench-confirmed.

### Current physical evidence

Core matching-code pairing and encrypted saved-bond reconnect after both-board power cycling are user-confirmed. The real companion also retains its live link, reconnects after explicit app relaunch and supplies accepted phone name/battery/UTC metadata. See [the concise mobile evidence](../mobile/docs/development.md#current-evidence-and-remaining-checks). No fixed reconnect-latency promise follows from a single Android log. Extended Forget/fault/expiry/load and locked-screen checks remain separate gates under [background sync](background-sync.md).

### Forget while connected

Forget phone opens a confirmation with Cancel as the initial focus. Confirm sends one semantic Forget request. The display then shows “Disconnecting and forgetting phone...” and suppresses repeat operation rows while waiting; a queue acknowledgement alone does not count as completion. Back remains available. The vehicle requests disconnection once, waits for the actual disconnect event, stops advertising before NimBLE unpair, verifies the saved-bond list and resumes advertising. For an already disconnected saved phone it stops advertising and removes the bond directly. Only a confirmed unpaired status restores Pair phone / Back. The enrollment window stays closed until Pair phone is selected again.

Vehicle completion has a three-second deadline, and the display's completion wait expires after 3.5 seconds. A refused request, storage error, lost link or timeout remains explicit rather than assuming the phone was forgotten. Android may retain its own bond; remove that on Android before a fresh pairing if necessary. Physical Forget and repeated-input fault acceptance remain required; do not treat old removed host tests as current verification.

The pairing-code UI now uses a large speed-font modal and a prominent optional phone name. Name support requires updating both firmwares and implementing the [companion write contract](../mobile/docs/protocol.md#enrollment-name-c011). Unbonded security initiation waits at most three seconds for optional name metadata; saved-bond reconnect remains immediate. Core real-app name/code behavior is user-confirmed; latest first-pair ordering and the extended fault matrix remain separate acceptance.

The reader also displays `@BLE SECURITY` (peer procedure already active / actual start failure) and `@BLE LINK disconnected reason=...`. A missing `@BLE FAULT` line does not prove that the phone stayed connected: previous firmware had silent security-start termination paths. The peer-first security regression is described in [the name integration contract](../mobile/docs/protocol.md#enrollment-name-c011). Diagnostics never print the comparison code or keys.


### Background-sync and phone telemetry handoff

Vehicle firmware supplies new authenticated c013/c014/c015 capability, full discrete-input telemetry and health values; [Protocol](../mobile/docs/protocol.md#background-sync-capabilities-and-telemetry-c013c015) defines every byte. [Mobile background sync](../mobile/docs/background-sync.md#m2-integrate-the-supplied-telemetry-firmware) is the app developer's implementation handoff. No UART or display runtime change is required for this addition; retain a compatible existing display image.

Build the vehicle using `bash scripts/flash-vehicle.sh --build-only`; use the existing guarded user-operated flash workflow and current port. Build-only does not access boards. A factory combined flash can replace NVS; do not promise bond preservation across reflashing. Reconnect/re-enroll explicitly as required rather than disabling security.

For a full continuity observation window, the read-only USB collector accepts 1..7200 seconds:

```sh
/Users/codewavesandy/.platformio/penv/bin/python tools/vehicle/ble_log.py /dev/cu.YOUR_VEHICLE_PORT --seconds 3900
```

It sends no data/reset request and prints only bounded `@BLE` diagnostic lines; firmware emits one `@BLE SYNC metadata-expired` line per expiry. Detailed counters/maximum ages are available through authenticated c015 reads by the updated app, not through a new USB command. Ordinary telemetry events are not logged individually, avoiding a new console flood/sampling task. Phone USB/debugging must be disconnected during B1; board USB power/logging remains acceptable for a desk check. Capture numeric app callback gaps and c015 deltas alongside display UART/heap/stack/render diagnostics; absence of serial faults alone is not acceptance.

## Phase 1 map firmware integration

2026-11-02/03 implementation session: firmware source implementation and affected board builds/packaging are complete; **not flashed, radio/physical/real-GPS accepted or bike verified**. No serial board ports were connected during this session. The implementation handoff is [phase 1 maps](map-phase-1.md), with exact implemented v1 bytes in [phone protocol](../mobile/docs/protocol.md#phase-1-map-extension--implementation-specification). Android map/GPS work is still separate.

Implemented:

- Portable R15B package validation/projection, R15X commands, R15K capability and R15S display status; incremental CRC/SHA-256 and bounded escaped R15Q UART framing/demultiplexing. [Normative vectors](../shared/protocol/map-phase-1-vectors.txt) are non-executable data, not an automated suite.
- Display reserves two binary slots and two decoded scenes from PSRAM only (540,728 bytes at current ceilings); failure disables maps while instruments continue. Its application/LVGL owner stages, hashes/parses in bounded512-byte/16-road steps, then publishes an immutable scene. No scene-sized task-stack copies, flash/NVS map writes, partial-map rendering or synthetic board GPS source.
- Navigation tile opens the map page under the existing Settings lifecycle. Header/compact instruments/tell-tales and warnings remain; full dashboards remain outside the map page. Touch, serial focus/select/Back and Trip-next/Info-select on the open map page operate Back/Follow/Area/Zoom. Cached maps remain on disconnect/Stop; stale/uncertain/poor-accuracy/outside fixes hide the marker. A valid stationary fix uses a neutral dot; qualified course uses an arrow. Follow reprojects under each active region's origin.
- Vehicle adds authenticated c016/c017/c018 with authenticated/encrypted status CCCD, current boot/connection/display IDs, downstream resource/credit checks, one pending control/latest location and display-originated ACKs. c013–c015 bytes stay unchanged. All UART1 writes are whole bounded records through a priority worker; GPIO sampling never waits on a map write. Bulk DATA is limited to4096 encoded bytes/sec; critical frames precede every next fragment. Driver RX buffers are2048 bytes, bounded reads remain256 vehicle/512 display bytes per application turn. UART0 console/return records use a separate bounded writer, preserving existing pins/baud and record integrity alongside logs.
- Numeric diagnostics include internal heap/min/largest block, task stacks, source gap and TX queue-full attempts (backpressure attempts, not proof of dropped/delivered map data). Display reports PSRAM and whether its map reservation succeeded. Runtime resource/timing/coexistence measurements remain a physical gate.

Generate the binary Kolkata region from the already cached source:

```sh
python3 scripts/import-osm-map.py
```

Current output: `build/maps/kolkata/kolkata.r15b`, **30,066 bytes**, SHA-256 `3443bb2dab6efe9a91840b0c8f375cfd970398c86c6fac0d23c93327fdd91685`; 1,435 road chunks /4,494 points. The `.manifest.json`, `.geojson`, provenance and licence accompany it. The C++ SHA output in the firmware developer preview matches this Python-generated manifest. This is the same central-Kolkata source timestamp/hash described above, not a new network request or citywide coverage.

Build and inspect the actual portable firmware store/tunnel with the native Dashboard map page:

```sh
cmake -S . -B build/lvgl
cmake --build build/lvgl
SDL_VIDEODRIVER=dummy build/lvgl/simulator/lvgl/r15-map-firmware-preview.app/Contents/MacOS/r15-map-firmware-preview --package build/maps/kolkata/kolkata.r15b --render build/lvgl/map-firmware-previews
```

This host-only developer preview explicitly labels BENCH DEMO and SIMULATED fixes; it has no radio/board injection endpoint. Captures cover17 states in both layouts/themes (68 images): empty, loading0/half, checking, active/no fix, simulated direction arrow/neutral dot, low accuracy, outside coverage, replacement-half, cancel, hash rejection retaining old map, stale, warning, Stop, disconnect and15s timeout retaining old map. Console diagnostics show accepted byte offsets, state/error/location changes and matching SHA; latest run has **0 LVGL warnings**. Manually inspected representative day/night, Street/Track, loading/checking, marker, stale/failure and warning images. The normal dashboard/Settings/Phone developer preview also completed successfully. These are development previews/runtime diagnostics, not resumed automated test suites.

Both `bash scripts/flash-vehicle.sh --build-only` and `bash scripts/flash-display.sh --build-only` build/package successfully with pinned ESP-IDF5.5.4; display image partition bounds/application fit are checked. No chip or partition changes and no flashes were performed. One existing `PairingIdentity.h` misleading-indentation compiler warning remains; new map sources compile without warnings. Build hashes below identify this session's packages; rebuilds may change embedded build metadata.

For an actual paired-radio transfer, see [map tool commands and peer/MTU constraints](../tools/maps/README.md). The optional desktop sender has syntax/CLI checks only; it has **not** run against a board. It does not synthesize LOCATION, auto-approve pairing, remove the phone bond or claim locked-screen Android coexistence. Firmware physical acceptance still requires identified boards, PSRAM/heap/stack and serial rate/gap measurements, authenticated reconnect/subscription, first/replacement/malformed/stalled transfers and loss/reset recovery. Follow the complete [acceptance matrix](map-phase-1.md#8-phase-1-acceptance-and-evidence), then implement Android against the frozen v1 contract. No automated test suites were added or run.

| Package | SHA-256 |
| --- | --- |
| Vehicle `build/vehicle/sevensemi-s3/firmware.factory.bin` | `d1279752b776f2c081b83dc7e88a4a41492b5ad2e9e6f770a4deaeec5ca3cd92` |
| Display `build/display/crowpanel-v3/r15-display.factory.bin` | `4ecc2bb0f8c528aa3b66ec3bee28ff84ace5706e40bb2fff9f074a44411a386b` |

Build-time static usage is vehicle23,880 bytes RAM /486,941 bytes app flash; display26,104 bytes RAM /701,375 bytes app flash. These exclude runtime stacks, driver/LVGL allocation, framebuffers and reserved map PSRAM; they are not free-heap/performance measurements.

## Phase 1 user feedback and phase 2 planning

2026-11-03 user report: both boards/APK installed; real map appears, but approximately30KB sharing takes1–2minutes, current marker is hidden, low-accuracy/outside-coverage messages appear, free display drag/pan is absent and a green vehicle-board LED is distracting. No exact MTU, measured stage timings, reported accuracy/anchor diagnostics, flashed/APK hashes or LED circuit identity accompanied this report. Record it as partial user bench evidence, not a completed navigation/continuity/performance acceptance.

The [phase 2 plan](navigation-phase-2.md) and separate [firmware](navigation-phase-2-firmware.md)/[mobile](../mobile/docs/navigation-phase-2.md) work packages document the requested new journey. Location precision permission, actual accuracy, map coverage and transfer readiness are independent. Current central-Kolkata coverage cannot serve arbitrary current locations. The plan includes throughput measurement/pipeline targets, dynamic source/cache, search/motorcycle routes, full phone corridor vs bounded display cache, independent cameras and verified LED-off handling. No firmware/app code, local app build, flash or tests were performed in this planning turn.



## Phase 2 firmware integration

2026-11-03/04 (final checks2026-11-04): **firmware scope implemented and board-build/manual-host-render verified.** This supersedes the earlier foundations-only status. The user explicitly requested firmware-first completion, so the independently defined semantic-cell/route contract is frozen now; Android still must prove its canonical raw-cache/renderer adapter and encode to it. See [firmware status](navigation-phase-2-firmware.md), [exact c019–c01b contract](../mobile/docs/protocol.md#phase-2-navigation-extension--implemented-firmware-contract) and [static normative examples](../shared/protocol/navigation-v2-vectors.txt).

No board ports were opened or flashed. No local Android/Gradle/native app compilation, SDK/Studio installation, automated suite or small-screen/2× phone inspection was performed. Git changes remain uncommitted/unpushed. Firmware build scripts/backup/partition guards,115200 baud and existing physical pins are unchanged.

### Implemented path and ownership

- Portable `NavigationProtocol` specifies source descriptor/full object identity, Web-Mercator extent4096 cells, route overview/detail windows, strict revision/source/context rules, quality-bearing LOCATION/PROGRESS and status/interest/window requests. No raw MVT/GIS SDK, radio, LVGL or RTOS dependency enters the shared model/store.
- Vehicle `NavigationBridge` and authenticated c019/c01a/c01b preserve one trusted bond/GATT owner. Up to four commands remain pending until display ACK; ATT success never commits data. Byte credit derives from the bounded command slots/value limit. Latest location/progress are separate coalesced lanes, each≥500ms, ahead of bulk DATA. Identical retries, conflicting sequence bytes, missing-prefix recovery, urgent cancellation/Stop/End, boot/connection/display invalidation and stale capabilities are explicit. Original c011–c018 bytes remain unchanged. Bonded GATT service-cache subscribers are notified after authenticated connection; no bond erase or approval bypass.
- The existing single UART TX worker fragments complete messages independently of50Hz acquisition. Critical records precede each new lower-priority record. DATA retains4,096 encoded B/s cap,≤133-byte fragment, safe escaping/demultiplexing and bounded queues. No giant driver backlog, new baud, Wi-Fi ingress or extra radio owner.
- Display preflights twelve32KiB complete raw slots + one32KiB staging slot, shared v1 binaries and twin render snapshots. Combined map buffers are **966,712 bytes**; each scene204,828 bytes. The guarded allocation is PSRAM-only; inline owner metadata/internal RAM is separately reflected in the build/runtime diagnostics. Source/route content is never stored in NVS. Resource failure disables v2 while instruments continue.
- SHA/body-CRC validation is incremental≤512 bytes/poll, geometry validation≤16 records/poll. Render staging copies≤16 roads/32 route points per poll and swaps only when complete. Never display a half-received cell/window. Geometry-ceiling pressure reports partial detail. Regional-edge objects can be fully received yet geographically partial; that explicit flag never counts as full tile coverage and cannot downgrade an already complete tile.
- At most six raw cells form the selected visible working set; target LOD wins, with bounded cached parent/finer fallback. Obsolete zoom levels retire without pinning an entire city. Two halo targets and old/new route context fit within twelve slots. Active snapshots own copied immutable geometry, so raw-cell retirement cannot dangle LVGL pointers. Active/candidate route objects and in-progress build references are protected; unpinned remote content is LRU-evicted.
- Camera requests coalesce up to four missing visible keys before halo, repeat unfulfilled needs at bounded1s and never change phone routing/camera. Full-SHA HAVE avoids a whole resend; partial staging never resumes across sessions. ROUTE_NEED requests detail near/outside the current range; route activation requires complete matching overview/window/current-area context. Old revisions/conflicting route identities cannot activate. New route candidates suppress old confident guidance; the old complete scene remains until publication.
- MapPage displays independent source/coverage/transfer/camera/route states, geographic neutral/qualified-course markers, hollow approximate points with actual uncertainty, and gray previously eligible last-known points only up to10s. Live/guidance requires conservative<2s monotonic age and current anchors/context; future/mock/replay/uncertain observations never become live later. Coverage never substitutes for GPS quality, and a valid position outside the former Kolkata area still renders with missing-map status. Phone estimates stay labelled; no GPS/route field populates instrument speed/gear/odometer.

### Camera, touch and physical navigation

Drag within x0..799/y142..441 enters Browse. Incoming fixes move the geographic marker without moving that camera. Follow explicitly recenters; Area fits the complete route overview or current cached roads. V1 has three zoom levels; v2 has six at0.12/0.24/0.48/0.96/1.92/3.84 projected px/m. Scale and accuracy radii include latitude. World east wraps; Mercator north is bounded. A dot can leave the viewport while browsing. Missing detail stays empty/cached, never fabricated.

GT911 uses the proven single-contact registers with30ms sampling. Press/move/release/cancel and6px drag threshold replace release-only taps. Read/clear faults, invalid coordinates, second contact or500ms loss of reports cancel; all contacts must release before a new gesture after fault/multiple contacts. Hidden pages/lifecycle cancel the reducer. A drag consumes its release even over the footer; it cannot toggle the bench demo. Pinch is not claimed.

Footer Pan opens West/East/North/South/Done. Trip cycles focus; Info selects. Next/Previous/Select/Back and serial `[`/`]`/Enter/`q` also work. Back first exits Pan. Existing header/Settings/tell-tales/warnings and2.00s boot/shutdown ownership remain. No held-button combo, touchscreen-only requirement or new wiring.

V1 APK remains compatible and keeps its strict50m/bbox/location contract. New v2 map/source/route behavior requires the upcoming Android encoder/session work; installing this firmware alone cannot make the current v1 APK download surrounding tiles or search routes.

### 7Semi onboard RGB off

The user confirmed7Semi ESP32-S3 DevKit/N8R8. The official [7Semi N8R8 product page](https://7semi.com/7semi-esp32-s3-dev-boardc-1-n8r8-wifi-bluetooth-dual-usb-c-rgb-led/) describes an addressable RGB; its embedded [DevKit pinout](https://cdn11.bigcommerce.com/s-e7ighpotvb/images/stencil/original/image-manager/dc-10123-2-.jpg?t=1788758445) explicitly labels **GPIO48 — RGB LED**. Reference image SHA256 `2897ebd735c3605af4913cdf7051d9fb5e1d5c8a6cfad179f017a48f96b7c646`, inspected2026-11-03. The family diagram establishes the LED mapping, not the exact delivered module memory/revision/cold-power behavior; do not infer those from the photographed module.

The `sevensemi-s3` profile now sets `R15_VEHICLE_LED_KIND=3`, `R15_VEHICLE_LED_GPIO=48`. `BoardLed.h` uses a WS2812-compatible all-zero frame via hardware RMT (10MHz,0.3µs high/0.9µs low zero bits,600µs low latch), releases the channel and holds GPIO low. Zero RGB bytes do not depend on color ordering. This is the selected compatible driver profile, not a claimed measured LED part number. It runs first in app_main, before BLE/input work, and never blinks/colors status. Static guards reject collisions with inputs/UART/USB/reserved memory/strapping pins. GPIO/simple-active-level and hardwired-power profiles remain explicit alternatives for a future verified board, not guesses used here.

Bench acceptance must observe startup, soft reset, full power cycle, pairing, transfer and faults and confirm the RGB stays off after application startup. ROM/bootloader flashes and a hardwired power LED are not a firmware-only guarantee. No physical LED modification was made.

### Build evidence

Both existing guarded build-only scripts pass with pinned ESP-IDF5.5.4, C++17 and affected-source warning flags; latest logs have **no warnings/errors**. Display image/application/partition-fit validation passes. No device accessed:

```sh
bash scripts/flash-vehicle.sh --build-only
bash scripts/flash-display.sh --build-only
```

| Target | Static RAM report | App flash report | Package SHA256 |
| --- | --- | --- | --- |
| Vehicle | 26,732 /327,680 bytes | 520,477 /8,388,608 bytes | `b8b82d85868590bd4dfb3a80d154d226ebf276478ee1d71610eaf01503b84180` |
| Display | 30,104 /327,680 bytes | 755,575 /4,194,304 bytes | `e2c7bf23ace9852d6037f40043cddcac8b1823e58115c8c54f3950042e4363df` |

These are linker/build values, not free runtime heaps/stack margins/FPS/scanout or physical BLE performance. Hashes identify these packages; embedded metadata can change on rebuild.

### Functional tools and manual preview evidence

Prepare bounded developer objects from the existing regional asset; no network/phone location:

```sh
python3 tools/maps/navigation.py prepare --package mobile/assets/maps/kolkata.r15b --output build/maps/navigation-v2 --demo-route
cmake --build build/lvgl --target r15-map-firmware-preview r15-map-preview r15-preview --parallel 4
SDL_VIDEODRIVER=dummy build/lvgl/simulator/lvgl/r15-map-firmware-preview.app/Contents/MacOS/r15-map-firmware-preview --navigation-bundle build/maps/navigation-v2 --render build/lvgl/navigation-v2-previews
```

The preparer validates/encodes clipped Mercator geometry, bounds object/count/area work, refuses dense overflow without truncation, retains source/licence and marks boundary tiles geographically partial. Source input is unchanged R15B30,066 bytes/SHA `3443bb2dab6efe9a91840b0c8f375cfd970398c86c6fac0d23c93327fdd91685`, derived from the previously documented OSM snapshot. Current developer bundle: **16 cell objects, 35103 encoded bytes, 12 partial edge cells**, plus two128-byte illustrative route objects; descriptor SHA `e4e2b562d264dfcb60dbd0dc70c8d8de3aa3ee9537852302081b6969e203700d`. Different framing/conversion overhead means this is equivalent regional source geometry, not exactly the same30,066 encoded bytes as v1. Manifest, complete object SHA/size, index and licence live under ignored `build/maps/navigation-v2/`. No new provider/account/key or data upload. `--demo-route` is an explicit host illustration on one real road line, **not** calculated routing or physical GPS; native preview labels it HOST SIMULATION/BENCH DEMO.

The actual `NavigationStore`, `NavigationBridge`, full four-command admission/ACK and bidirectional escaped tunnel are exercised by the developer renderer. It captures **38 states ×4 themes/layouts=152 images**, with **zero LVGL warnings**. Coverage includes current point without roads, partial packet hidden, first complete cell, cached working set, point outside the former city with missing roads, return to cache, independent Browse/new fix, approximate/last-known/expired/future-invalid points, route activation, quality-gated instructions, overview, next-window request through the bridge, staged reroute, NOT_READY for missing minimum context, atomic new revision and rejection of older revision, hash failure/cancel/timeout retaining complete cells, warnings, Stop then End clearing the route/offline pan/disconnect, new-session no-source/guidance, full-SHA reuse and shutdown/off/boot. Representative day/night Street/Track, missing/approximate/stale, route/revision/warning and lifecycle images were manually inspected. This is a development renderer/runtime diagnostic tool, **not** a restored automated suite, fake device source or throughput measurement. Legacy v1 store/renderer captures remain available separately with `--package`.

Optional actual radio map-only sender, after normal user-controlled OS/cluster pairing of the selected Mac peer:

```sh
build/map-tools/bin/python tools/maps/navigation.py send --device YOUR_ALREADY_PAIRED_OS_ID --bundle build/maps/navigation-v2 --mtu 96
```

Use the existing pinned bleak1.1.1 environment from [map tools](../tools/maps/README.md). MTU247 is opt-in only after verifying the actual negotiated bound; CoreBluetooth does not expose it reliably here. The tool validates bundle hashes, honors bounded DATA windows, waits for display offsets/verification, reports bytes/time, and never auto-approves pairing, replaces bonds, sends synthetic LOCATION, activates demo routes or operates bike controls. It has syntax/CLI/preparation validation, **not physical BLE acceptance**. Do not replace the phone bond just to run it unless explicitly choosing that normal pairing workflow.

### Diagnostics and required physical acceptance

Vehicle `@MAP_BLE` reports observed MTU, admissions/backpressure, admitted payload bytes including retries, maximum actual admission→display ACK and v2 window high-water. `@MAP` reports successful encoded bytes/bulk/fragments, write failures, TX queue-full attempts, max critical enqueue→worker dequeue wait at FreeRTOS tick resolution, source gap, heaps/stacks. `@MAP_RX`/display RX report framing/CRC/timeouts. Display `NAV2` reports commands/rejects/duplicates/accepted DATA/verified objects/evictions/hash failures; ordinary `MAP` retains v1 receive/verify/accuracy/age/anchor/coverage diagnostics. UI/loop/flush maxima and PSRAM/internal heaps/stacks remain visible. Counters/maxima are bounded lifetime numbers and can wrap; use interval deltas. No source coordinate, route/destination, payload, bond secret or phone identity logs. The return UART shares the console physically; binary protocol records are not plaintext diagnostic output.

Before hardware acceptance: compare no-map and sustained transfer source/critical/UI gaps, actual MTU96/≥247 useful vs encoded rate and same-source geometry timings; first useful≤8KiB≤5s and30KB20s/45s are targets, **unmeasured**. Measure both heaps/largest/min/stacks/queues/PSRAM/render/scanout. Check old APK compatibility, security/bond/service discovery, disconnect/reboot/source expiry, incomplete/corrupt/duplicate/gap/cancel/retry, cache pressure and rapid zoom/pan, route/window/revision replacement, physical Pan/Back/touch and both themes/layouts/warnings/lifecycle. Confirm LED-off on the real board. Real GPS, Android locked-screen/notification Stop behavior and source/cache/provider offline proof follow the Android v2 Actions APK; no local app build or automated test suite is authorized.

### Return-UART connection flapping — 2026-11-04

The user reports the display alternating between live phone state and missing phone-status/return-UART messages with stable wiring. Source inspection found the display's console-mode `RecordWriter` wrote binary heartbeat and map reply records with `fwrite` but no flush. The pinned IDF5.5.4 `components/newlib/src/reent_syscalls.c::_fstat_r_console` explicitly enables line-buffered stdout/stderr. A complete binary record need not contain LF; its tail could wait for a later binary LF, buffer fill or log. This can exceed the existing1000 ms return-heartbeat deadline. The relevant actual UI message is `Return UART not confirmed - check TX43 to GPIO18`; vehicle RGB GPIO48 is unrelated.

The shared writer now holds the stdout stream lock across `fwrite` and an immediate `fflush` for each console record. A flush failure counts as a TX write error. The UART1 driver path remains unchanged. No pin/baud/security/CRC/freshness-threshold change or UI debounce hides an actual lost link. This fixes an identified delivery defect consistent with the report; the reported physical symptom is not yet proven resolved.

Verification: both guarded `--build-only` firmware builds/package checks passed on the pinned toolchain; no boards were opened/flashed, no Android build and no automated suites were run. Display factory SHA256 `f3ef86a69f5f5ebde42898058a0237079a7f1dbca15be06b7c34ae9067b9852a`; vehicle compatibility-build SHA256 `06d19f1b3904c449a4aebdb38118faf8ab230633ad97223c764c6a88ca66607c`. The display needs reflashing for this fix; vehicle reflashing is not required for its unchanged UART1 path.

Physical follow-up: with the same wiring, flash the display using its guarded script, then observe Phone state for several minutes first without map sharing and then during a sustained v2 transfer. Check that real connection/name/battery remain steady and `MAP write_errors`, critical wait, RX framing/CRC errors and overruns do not grow unexpectedly. Disconnect a UART wire deliberately only during the normal powered desk bench procedure to confirm the original unavailable-state deadlines still work, then restore it. If flapping remains, capture both boards' existing10-second numeric diagnostics over~60 s; separate return confirmation, R15G freshness, missing R15P status, RX resets and BLE reconnects rather than assuming a physical cable fault or lengthening deadlines.

### Both-board follow-up after the symptom persisted

The user reports the stdout-flush change alone did not resolve flapping and clarified that pairing cannot yet be completed; no phone map transfer is involved. Two45 s numeric serial observations found the forward R15G stream at approximately49 accepted packets/s, display RX overruns0 and fresh UART state. Display-reported TX byte counts continued increasing with write_errors0, while the vehicle's decoded return-fragment count stalled around100, with tunnel CRC/framing/timeouts0. This localizes the next investigation to return reception/status confirmation, but does not prove a physical wire fault or distinguish driver flushes from parser/heartbeat loss. These are runtime observations of the then-installed images, not acceptance of the new changes. The serial observer requested deasserted DTR/RTS and sent no data; serial-open/close reset effects were not independently measured.

Both UART receivers previously discarded all buffered data above1536 bytes even without a driver overflow, and processed only one256-byte vehicle/512-byte display read per loop. Both now use a4096-byte driver ring and drain up to1536 bytes in bounded nonblocking chunks per application turn. High backlog alone no longer flushes valid records. Actual UART FIFO/full/frame/parity fault events still invalidate parser/session state; the original1000 ms freshness/security checks remain. No pins, baud, CRC, pairing approval or UI timeout masking changed.

Vehicle `@MAP_RX` now additionally records driver resets, RX bytes/high-water backlog, accepted controls, display-session-change count, current control age, maximum control gap and actual return freshness. Display diagnostics add RX bytes/high-water backlog and `PHONE_LINK` numeric quality/unavailable reason, status count/age/max gap. These distinguish the failing lane without phone identity or packet logging.

Both guarded firmware builds/package checks passed without compiler warnings; no automated suites or Android build. New factory hashes: display `8c30ba9f6291b2d0bd75d6de1842b964715eecdfe8e23801eee5b45cb7becaab`, vehicle `aa65f0899bf7181652757700fcb5d2c9e8307afbb619ad24eb41f60f184c7de8`. **Both boards need the new images for this follow-up**, superseding the earlier display-only instruction. The user chose to perform flashing manually. No flashing was performed by the agent. Stable pre-pairing status, successful numeric approval and post-pairing/loaded behavior remain unverified; collect the new counters after both updates before claiming the problem resolved.

### Confirmed cause of the pre-pairing regression: vehicle main-stack overflow

This supersedes the earlier flush/backlog hypotheses for the user's repeated pre-pairing flapping. The follow-up capture on the diagnostic images showed vehicle counters returning to startup values every~10.2 s while display counters remained cumulative. A filtered vehicle panic capture then **directly confirmed** `stack overflow in task main has been detected.` at10.3 s,20.4 s and30.6 s, followed by `Rebooting...` and `rst:0xc (RTC_SW_CPU_RST)`. The captured running ELF prefix was `8ed1b8246`. Initial serial opening also produced `USB_UART_CHIP_RESET`; the subsequent periodic software resets occurred while the port remained open. No flash or serial command was sent during the observation.

Immediately before the crash the vehicle reported driver_resets0, return_fresh1 and low RX high-water values (~240–299 bytes), and the display reported uninterrupted real R15G receipts, no RX overruns and current phone statuses. Thus the logs do not support attributing this reproduction to wiring, RX capacity or map transfer. The visible waiting/session/return-confirmation messages are expected during repeated vehicle reboot/identity re-establishment; no UI debounce should hide them.

The Phase2 changes increased the work/diagnostic call depth while the vehicle retained IDF's default `CONFIG_ESP_MAIN_TASK_STACK_SIZE=3584`. The main task's compiled entry frame alone was1120 bytes (`entry a1,0x460`), before nested driver/formatting/diagnostic calls. The ten-second reporting path overflowed that budget. Source defaults now request8192 bytes and `main.cpp` statically rejects an actual generated setting below8192, preventing an old cached3584-byte SDK configuration from silently rebuilding the faulty image. The existing local generated SDK config was backed up byte-for-byte and verified before updating it; fresh checkouts inherit the tracked defaults. Existing other workspaces with cached SDK configs must explicitly update that setting or regenerate their config, preserving any local hardware settings first.

The extra4608 bytes are RTOS main-task stack allocated from runtime RAM, not an additional raw-map buffer. The display's main stack is already16384 bytes; no display stack/pin/security/protocol change is required. Both-board diagnostics and the earlier source changes remain available, but they must not be represented as having fixed the confirmed crash. Only the vehicle needs reflashing for this stack correction. Afterward capture several consecutive ten-second reports: counters/uptime must remain monotonic, no panic/software-reset loop, healthy main/TX stack margins and return heartbeat gaps below the unchanged deadline. Then validate numeric pairing and ordinary telemetry before map sharing. Automated suites remain prohibited; no Android build is involved. Physical post-correction acceptance remains pending until the user installs this image.

The corrected vehicle guarded build/package check passed (66.87 s) with generated `CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192` verified in `build/vehicle/sevensemi-s3/config/sdkconfig.h`. Factory image SHA256 `24e996401e91a7b5e67e780f16ed95cbc7ca7596db9788011cc41003e55025cb`. No post-fix flash was performed; install and sustained crash-free/stack-margin/pairing observations remain pending. This image supersedes `aa65f089...` for the vehicle; the installed display can remain unchanged for this correction.
