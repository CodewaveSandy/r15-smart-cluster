# Development, wiring and commands

**2026-11-05 removal:** The custom map/navigation implementation and its menu/tab have been removed. Pairing, metadata/time, telemetry and the UART/vehicle-stack fixes remain. Google Maps notification turn guidance is planned, not implemented. See the [removal record](navigation-removal.md).

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

## Notification turn guidance firmware — 2026-11-05

Firmware source integration is complete for the new optional no-map dashboard guidance path. The [firmware handoff](turn-by-turn-firmware.md), [normative v1 bytes](turn-guidance-protocol.md) and [static vectors](../shared/protocol/turn-guidance-v1-vectors.txt) are the current authority. Android is still pending; Google Maps notification source feasibility and all physical acceptance remain open. User explicitly requested firmware-first work after removing the custom map runtime.

Both pinned build-only scripts succeed without accessing hardware, with no compiler warnings in the final affected builds. The normal host dashboard/Settings/Phone renderer succeeds. The guidance developer renderer writes53 states ×4 layouts/themes=212 labelled simulation images with zero LVGL warnings. Manual visual inspection covered Street/Track, Midnight/Daylight, fresh left/right/keep/U-turn/roundabout, unknown distance, long text, muted32-second instruction,120-second update-pending, expired clock, warning precedence and late-after-Stop ordinary trip/info. The actual shared bridge/store/codec/demux/assembler are used, not a fake application-data source. Each simulated layout run records127 accepted display commands,5 deliberate display rejections,1 identical duplicate and1 transport expiry before the explicit reboot; bridge records129 admissions and2 deliberate shape rejections (above-max distance/non-ASCII). After simulated reboot, the old context is rejected. Stop-before-Open receipt is included. These are manual preview/runtime diagnostics, not an automated suite, real radio notification observation or measured target latency.

Static target accounting from PlatformIO (excludes runtime allocations/stack headroom):

| Target | Static RAM | App flash used | Main task configured |
| --- | --- | --- | --- |
| Vehicle sevensemi-s3 |22,356 /327,680 bytes|502,745 /8,388,608 bytes|8192 bytes, compile-guarded |
| Display crowpanel-v3 |24,752 /327,680 bytes|693,479 /4,194,304 bytes|16384 bytes, compile-guarded |

Actual generated Kconfig headers were inspected for those stack settings. Disassembly shows app_main local frames vehicle0x430 (1072 bytes), display0xfe0 (4064 bytes); those are compiler frames, **not peak call-stack usage or physical margins**. Host ABI Store584 bytes, Bridge512 bytes, assembler248 bytes; board-ABI static guards include the actual writer/receiver/reassembly/message/presentation allocation budget under4096 bytes. No old map arenas or new route/cache/GPS dependency is present. Firmware diagnostics retain heap/free/min/largest, main/TX margins, source/UI/return gaps, frame CRC/timeouts and numeric guidance counters. Bench measurement is still required.

Source backup: `build/source-backups/before-turn-guidance-20261105-180144/source.tar.gz` and `SHA256.json`,317 source/doc/asset members individually verified. Outputs and previews stay under ignored build/. A clean rebuild corrected stale PlatformIO LVGL component objects after enabling the local line widget; another checkout with an old library must clean that generated display environment as documented in the handoff. A freshly constructed preview also caught and fixed the17-entry icon-definition table's leftover18-slot allocation from map removal; compile-time completeness/order checks now prevent null default slots.

No automated tests/suites were created, restored or run. No Android APK/Gradle/native build, SDK installation, serial port access, flashing, phone permission change or physical board/phone acceptance occurred. Before app integration/claims, manually flash matched images, prove stable pairing/metadata/input operation over repeated diagnostics, then implement Android against the frozen v1 channel using GitHub Actions. Measure negotiated MTU≥227, actual source changes, timing/ACK bounds, Clear/revoke/reconnect/reset, stack/heap/source gaps and60 min locked-screen coexistence. Google Maps' fields/cadence/languages remain unproven;30/120-second source-age treatment is a firmware policy awaiting that observation.

Final packaged image SHA-256 (no flash):

- Vehicle: `222c3168d089d5dc206ac22bce66c002f9f9de53ec00ae45806fdae603f1e01a` (568,688 factory-image bytes).
- Display: `3d83c634df506b212b8fba304e08557d7996e482a4b3891597043394185384f4` (759,424 factory-image bytes).
