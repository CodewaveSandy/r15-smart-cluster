# Implementation status and plan

Updated: 2026-10-22. Status: native dashboard, Appearance preferences, unified Phone UI and native nearby-discovery development adapter implemented; Navigation, hardware firmware and integrations pending.

## Current implementation

| Feature | Current state |
| --- | --- |
| Dashboard | Street centered speed; Track centered current/best lap with secondary speed; equal section heights; Midnight/Daylight |
| Instruments/assets | Chakra Petch Bold Italic numerals, Inter labels/RPM, Barlow clock; 17 MDI symbols; fractional eight-piece fuel gauge and 16% reserve cue |
| Ride pages | ODO/Trip 1/Trip 2; coolant, Track, real-time consumption, average consumption, average speed, TCS, unidentified placeholder |
| Settings | Three-category home, reversible 280 ms compact-speed transition, sequential non-touch focus and access below valid 20 km/h |
| Appearance | Theme preview/Apply/Cancel and versioned host/native NVS restart persistence; one Instrument design |
| Phone | Unified page, first-pair approval/code, saved/disconnected/connected, confirmed forget and unavailable states; native Android/Mac BLE discovery and protected saved trust in development |
| Lifecycle | R15 Light Signature; boot 2,000 ms, shutdown 2,000 ms; immediate valid urgent-warning reveal |
| Host optimizations | Persistent RAII components, changed-field updates, bounded formatting, role-specific font/icon subsets, deferred covered instrumentation, persistent readout text buffers, dirty-driven SDL presentation and zero settled redraws |
| Phone/Mac development connection | Native BLE discovery, cluster-approved pairing, protected saved trust, TLS session and bounded IPC; physical new discovery journey awaiting user test |
| Pending | Navigation/Maps feasibility, clock editor/source, real trip/lap calculations, UART codec, real radio/vehicle adapters, panel/power/hardware verification |

Host odometer, trips, laps, economy and average speed are fixed samples; TCS mutation and radio flows are simulator-only. Only confirmed theme/design persist. UI warning thresholds and the Settings speed gate are user-selected behavior, not verified Yamaha limits or proof of safe use while moving.

## Sequence

| Stage | Deliverable | Exit evidence |
| --- | --- | --- |
| E0 — Design | Architecture, style guide, responsibility map and unresolved decisions | User reviews plan; exact board identity, vehicle MCU and Maps scope tracked |
| E1 — Reproducible baseline | Define deterministic UI scenarios; vendor board example; pin toolchain/dependencies | Host test baseline established; known vendor example builds; schematic/revision and flash/PSRAM/ports match actual board |
| E2 — Shared state and host LVGL | Portable quality/state/input model, LVGL host window and tokens | Same LVGL sources build on host; state/format/navigation tests; no LVGL/IDF dependencies in shared model |
| E3 — UI implementation | Street/Track, both themes, instruments, warnings, menus and lifecycle animations | Deterministic 800 x 480 captures against specified geometry; visual review; behavior boundaries pass |
| E4 — Display firmware | Minimal board port, fonts, settings, demo source and diagnostics | Firmware fits actual flash; RGB test and demo UI run on panel; measured memory/render/latency; no artifact/leak growth |
| E5 — UART bench | Reviewed protocol specification, codec, synthetic sender, display receiver | Golden vectors; truncated/oversized/corrupt/version/reset/overflow tests; cable loss, restart and stale signal behavior |
| E6 — Vehicle MCU | Verified input adapters/calculations and bounded Wi-Fi/BLE services | Bench sources match observations; radio load cannot starve acquisition; semantic input and command tests |
| E7 — Passive bike observation | Reviewed electrical interface and measured mappings | OEM retained; listen-only verification; compare logged observations to independent references; no speculative decode |
| E8 — Replacement evaluation | Power/boot/reset/fault, independent tell-tales and mechanical/environmental assessment | Explicit reviewed evidence before replacement; software parity alone is insufficient |

E2/E3 host work can proceed before delivery. E4 compile preparation can overlap, but board bring-up is a prerequisite to claiming display readiness. E5 can begin with synthetic UART data without Yamaha decoding. Custom moving-map navigation is a separate feasibility gate; it is not hidden inside E6 BLE work. The [companion implementation plan](../mobile/docs/implementation-plan.md) tracks the Android-first Flutter app, Mac bridge and future iOS work.

## Agreed next implementation

Retain two MCUs with separate power branches: vehicle owns acquisition/calculations/inputs/radios; display owns presentation and local Settings. Verify the supplied UART0 cable mapping and USB-console sharing, then implement/fault-test the inter-MCU connection before real phone integration or maps. The 2,000 ms boot and touch Settings entry are implemented in shared UI/native bench firmware; final wiring and radio services remain pending; display Appearance NVS is implemented.

## Next implementation gates

The host baseline, dashboard, portable presentation/input, settings and applicable host optimizations are implemented. See [resource accounting](resource-budgets.md) for current measurements and [settings](settings.md) for service boundaries.

1. Board revision and vendor example are bench confirmed; finish schematic/UART conflict review and identify the vehicle MCU.
2. Build and user-flash the R15 ESP-IDF target, then validate panel behavior with pinned dependencies.
3. Measure actual flash/partition fit, internal SRAM/PSRAM, driver buffers, timing and scanout behavior before claiming display readiness.
4. Plan Navigation and clock controls after their service/time-source contracts are reviewed; preserve the approved dashboard and lifecycle visuals.
5. Review the versioned UART contract and vehicle/radio persistence ownership, then implement and fault-test real adapters. Host theme/design persistence is already implemented; display Appearance NVS is implemented; target restart/power-loss/render measurements remain pending.

No empty firmware scaffolding, unused queues or speculative board pins substitute for these gates.

## Display arrival: first bring-up session

Received and bench tested on **2026-10-22**: DIS08070H PCB **V3.0** (user photos), ESP32-S3 revision v0.2, 4 MB flash and 8 MB embedded PSRAM (esptool output). Read the [independent target map](architecture.md#independent-build-targets) before creating a firmware build.

1. Record actual product/PCB revision, module markings, board photos, supplied example/version and matching schematic. Confirm flash/PSRAM and the intended supply/programming interface from that revision's documentation; previously reported N4R8 resources remain unverified.
2. Preserve the supplied factory archive and a documented restore/recovery route before replacing factory firmware. Preserve necessary source/assets/configuration in a verified backup. Record serial port and chip identity; no guessed pins, RGB timings, reset sequence or flash offsets.
3. Reproduce the revision-matched vendor example with its documented toolchain first. Record versions and prove build/flash/reset recovery. If it uses a different LVGL/framework version, treat it as a bring-up reference; do not mix LVGL 8 APIs into our LVGL 9.2.2 code.
4. Establish the separate **display ESP-IDF/CMake target** only after that baseline. Validate the compatible toolchain/dependencies, power, RGB color/stride/rotation, backlight, PSRAM/DMA/VSYNC and console/UART availability. No vehicle MCU or phone is needed for the first panel proof.
5. Port the existing shared dashboard incrementally using clearly labeled synthetic observations. Keep display-only preferences local and all LVGL access under one owner. Inspect both layouts/themes, warnings and unavailable readings on the actual 800 x 480 panel; measure flash/partition fit, memory, render timing and startup.
6. Test the 2.00 s boot and 2.00 s shutdown behavior separately from total boot latency. Shutdown requires verified retained power; hard power removal cannot be solved by firmware alone. No bike harness connection is required for this desk bring-up.
7. Record pass/fail evidence and unresolved issues before UART/radio/map integration. The **vehicle firmware** remains a separate future target pending MCU selection. Initial bike integration remains passive/read-only with OEM function retained.

First-session success means a recoverable, revision-matched panel example and measured minimal display baseline. It does not mean complete navigation, working bike telemetry, vehicle readiness or production readiness. Until target gates pass, continue using the independent host and phone previews.


### Recorded vendor baseline and R15 target

- Vendor source: `Elecrow-RD/CrowPanel-7.0-HMI-ESP32-Display-800x480`, `example/V3.0/PlatformIO70`. User built with pioarduino 55.03.38, Arduino 3.3.8, LVGL 9.1.0 and LovyanGFX 1.2.25. R15 keeps its own LVGL 9.2.2 and uses ESP-IDF 5.5.4 without Arduino/LovyanGFX.
- Factory read: complete 4,194,304-byte image; `verify_flash` digest matched. SHA-256: `babb6b68f22ddaf221760a6321a655405cc0b74abfebab1912491a17e48a50ff`. Saved outside Git at `~/Desktop/R15-factory-backup/crowpanel-v3-factory-rom.bin`. Restore has not been exercised.
- Delivered board MAC: `b8:f8:62:7f:2c:68`; observed Mac port `/dev/cu.usbserial-110` (may change). Stub-based reads failed; ROM `--no-stub`, 115200 baud read and vendor flash succeeded. This does not establish the underlying transfer fault.
- Vendor merged image: 1,937,408 bytes written at zero, hash verified. User confirms UI, touch, reset and stable picture. External DHT20/GPIO output untested.
- R15 target: `firmware/display`, C++17, ESP-IDF/CMake orchestrated by the existing PlatformIO installation, exact LVGL component lock. Reuses `embedded/ui`/application/presentation/assets. Default observations unavailable; touch/serial-selected synthetic sweep and test warnings labelled BENCH. No real odometer, radios or vehicle UART; display Appearance uses validated NVS storage.
- Run `bash scripts/flash-display.sh --build-only` to build/validate/package without device access. `bash scripts/flash-display.sh` additionally checks this board/backup and writes the combined image via esptool 4.8.1, ROM loader at 115200. The agent prepares software; the user runs flashing.
- Build verification (2026-10-22): native display target builds successfully; final script passes `--build-only` without device access. Six partition-boundary checks pass. All eleven existing host suites pass (IPC test rerun outside sandbox after a sandbox socket-bind denial); representative 800 x 480 layouts/themes with warning/unavailable states visually inspected. R15 image has not been flashed by the agent. The user subsequently flashed it successfully and supplied a photo/video showing Light Signature followed by Street with unavailable readings. The user confirms layout/theme switching and reset/touch operation. Exact timing, frame rate and heap measurements remain pending.
- Panel measurements still required: R15 boot artwork duration/total startup, both layouts/themes, warning/unavailable states, touch/serial actions, internal heap/stack/PSRAM and VSYNC/scanout behavior. Shutdown is bench-triggered only and cannot survive unplugging USB.

### Engine-off bike power-on (2026-10-24)

The user reports successful display power-on using bike socket power cables (reported 12 V) directly into a buck converter set to 5 V, then from the converter to the display. The engine was off. The [hardware connection reference](hardware-connections.md) records this path and the details needed for a complete wiring guide. Engine-start/running, supply protection, retained shutdown power and vehicle telemetry remain unverified.

## Test strategy

The 2026-10-24 [discrete-input experiment](../firmware/vehicle/README.md) assigns GPIO4/5 to left/right with configurable N–6 and high beam, version 3 USB packets with debounced Trip/Info counters on GPIO14/15 and a Mac serial adapter. Latest reported polarity is active HIGH; the reported 2 V lower active level is below guaranteed HIGH at 3.3 V. All twelve bench inputs are enabled with weak pull-downs for one-at-a-time rewiring; `scripts/flash-vehicle.sh` provides build, port listing and guarded backup/flash. Full display UART integration, electrical validation and physical flashing/tests remain pending.

- Portable unit tests: numeric/quality validation, stale deadlines, unit conversion, formatting, calculations, action routing, schema migration.
- Codec tests: all split points, multiple frames/read, garbage and boot logs, max lengths, CRC corruption, missing fields, versions/enums/ranges, sequence wrap, duplicate/out-of-order frames, reboot/session and source age.
- Host LVGL tests: important widget state and navigation, deterministic render scenarios at 800 x 480. Use the specified behavior and meaningful boundary cases.
- Target tests: RGB color bars, rotation/stride, PSRAM, panel timings, flush completion, fonts, settings writes, UART stress, brownout/restart simulation and independent MCU boot order.
- Load/soak: worst-case RPM/fuel/turn/shift updates with menus and settings writes; vehicle radio load; log frame times, latency, heap largest block, stack margins, queue high-water marks and loss counters.
- Vehicle validation: passive only initially; track every signal's evidence and compare actual readouts. Bench and host success never count as bike verification.

Performance objectives are provisional in [architecture](architecture.md). Establish representative runs and measured budgets before optimizing or promising frame rate/startup time. Do not gate correctness on internet services or a phone connection.

## Open decisions and missing evidence

| Item | Needed before |
| --- | --- |
| V3.0 photos, chip identity, checked factory backup and flashed vendor example recorded; matching schematic review and UART/programming conflicts still needed | Board pin/timing/UART selection |
| Vehicle ESP32 model and available peripherals | CAN/input/radio task implementation |
| User's measured harness inventory | Any bike adapter; record connector view, pin numbering, conditions, polarity, range and uncertainty |
| CAN physical/protocol confirmation | Controller/transceiver choice; no guessed IDs/bitrate |
| Custom moving map selected on 2026-10-21; embedded map/bandwidth/provider feasibility and phone-service capabilities still needed | Promising final feature coverage |
| Physical control source and event semantics | Hardware navigation; touch remains optional |
| Real versus calculated odometer, retention and calibration | Persistent vehicle values |
| Clock source, retention and disconnected behavior | Hardware clock editor/restore action |
| Warning unknown/failure behavior and independent indicators | Replacement evaluation |
| Image size/partition fit and update strategy | OTA or dual-image claims |

## Current verification

Eleven host suites pass: Phone menu, phone bridge IPC, model/lifecycle, Light Signature, presentation/input, desktop input, Bluetooth, Wi-Fi, preferences, typography and LVGL rendering. The render suite captures 210 scenarios, including both layouts/themes, signal boundaries, warnings/unavailable states, settings transitions, Phone pairing/confirmation and interruption. It rejects LVGL warnings and redundant flushes for settled screens. Repeated construction/destruction, cached-versus-fresh rendering and exact dashboard restoration are checked.

The separate [Mac bridge checks](../mobile/docs/mac-preview-connection.md#verification) exercise real Dart TLS and session behavior; they do not establish physical phone/LAN or ESP32 connectivity. The user built and flashed the official V3.0 PlatformIO example with successful write-hash verification, then confirmed working touch after reset and a stable picture without flicker. This is vendor baseline evidence, not R15 panel readiness. R15 firmware, UART, retained-power and bike verification remain pending. Run the [preview and verification commands](getting-started.md) after changes; record actual hardware evidence separately.

## Hardware and release gates

- Confirm actual board revision, panel pin mapping, timing, PSRAM and UART/console conflicts using matching vendor resources.
- Record harness measurements with connector orientation, pin numbering, conditions, voltage, polarity, loading and uncertainty. Do not infer protocol from wire labels alone.
- CAN bus lines connect through suitable bus protection/transceiver circuitry. Isolation requires a designed signal/power barrier; generic optocouplers in CAN H/L are not an approved interface. Verify bus type, termination and passive controller operation.
- Validate each discrete/analog/pulse input separately. No direct vehicle voltage to MCU GPIO; board power input rating does not establish GPIO tolerance.
- Verify regulated supply under startup, cranking, key-off hold-up, rapid cycling and hard power loss. Shutdown artwork requires retained supply; review [power lifecycle](power-lifecycle.md).
- Confirm independent warning strategy and failure behavior if either MCU or UART stops. Two MCUs alone do not provide safety certification or fault independence.
- Measure internal SRAM, PSRAM, flash/image fit, task stacks, queues, render times and radio/acquisition contention under representative load.
- Review real odometer provenance, persistence atomicity/wear and reset permissions before storing vehicle-owned data.
- Test boot/shutdown animation timing and urgent-warning interruption on host and panel. Record total key-on latency separately.
- Keep OEM functionality during initial passive testing. Do not transmit to a real vehicle without an explicitly reviewed active-test phase.

Software, bench, hardware and bike verification are separate evidence levels. No regulatory compliance or production readiness is currently claimed.

## OEM requirements and evidence

The supplied OEM photograph is a functional reference, not proof of available signals or simultaneous operating states.

![OEM cluster reference](reference/oem-cluster.png)

| Function | UI requirement | Vehicle evidence |
| --- | --- | --- |
| Speed/RPM | Readable speed, continuous 0–13 RPM scale | Exact sources/calibration pending |
| Gear/neutral | Gear-first header and neutral tell-tale | User reports harness progress; mappings pending |
| Fuel | Eight slanted segments, percentage and pump icon | Calibration pending |
| Turn/high beam | Separate turn arrows and beam pictogram | Detailed measurements pending |
| ABS/engine/TCS/coolant warning | Distinct visible tell-tales with unknown-state handling | Actual semantics pending |
| QS/VVA | Badges when supported/active | Real support/rules pending |
| Phone/battery/call/message | Separate phone status; never confuse phone and bike battery | Connectivity services pending |
| Odometer | Actual verified value or unavailable | Provenance/retention pending |
| ODO / Trip 1 / Trip 2 | Vehicle-side calculation with explicit units and reset semantics | Calibration/retention pending |
| Laps | Current/latest/fastest, monotonic timing | Application implementation pending |
| Bike battery/coolant | Values and independent signal quality | Electrical sources pending |
| Economy/average speed | Explicit unavailable until supported | Inputs and OEM semantics pending |
| Clock/date | Validated editor and explicit time-source quality | Retained clock source pending |

Street/Track are display layouts, not ECU commands. Midnight/Daylight are presentation themes. Keep lower information pages and physical settings navigation. User-selected visual warning thresholds are in the [style guide](lvgl-style-guide.md); they are not verified Yamaha limits. Factory menu ordering, economy units, indicator artwork and independent physical tell-tales require verification.

User observation (2026-10-19): OEM boot approximately 3 s. Info list: coolant, Track toggle, real-time fuel L/100km, average fuel L/100km, average speed km/h, TCS toggle, unidentified final entry. The final entry and consumption/average measurement windows need identification. Host presentation follows this order; no OEM protocol/calculation equivalence is claimed.

## Moving panel demo follow-up

The first working native panel state is preserved in commit `80dac40`. The next desk-test image adds a center-tap/serial `d` speed/RPM sweep and ten-second rendered-update diagnostics. Portable checks cover ramp direction/endpoints, normalized bounds, unknown odometer, repeat behavior and steady/flashing shift thresholds. Icon tone and two-VSYNC synchronization remain unchanged pending measured panel diagnostics.

### Daylight contrast and expanded demo

User panel feedback requested a more visible Daylight RPM ramp. Shared palette now provides darker Daylight ramp colors and a distinct bed, retaining all scale stops, geometry and shift thresholds. The desk demo now sweeps fuel through reserve/critical/empty and cycles tell-tales plus an explicitly labelled all-icon lamp test. Fuel-area tap or serial `o` previews shutdown; tap after black or serial `b` reboots. Synthetic urgent warnings are suspended for this explicit preview. Native build, host visual verification and user panel results must remain separate evidence.

Verification for this follow-up: native build and `flash-display.sh --build-only` pass without device access; eleven relevant host suites pass (unchanged phone IPC excluded), including fuel/lamp/touch/lifecycle bounds and LVGL rendering. Representative 800 x 480 Street/Track in both themes, Daylight unavailable and warning frames were visually inspected. Host settled redraw count remains zero. Updated panel appearance and touch shutdown still require user verification.

### Text contrast and native redraw follow-up (2026-10-23)

Information captions/units now use black/white theme foreground, including ODO, coolant/consumption, FUEL, RPM labels and main readout units. Warning colors remain meaningful. Native display rendering uses double-buffered LVGL DIRECT mode and a single true bounce-frame completion handoff, preserving vendor timings and one CPU0 LVGL owner. New host comparisons prove identical FULL/DIRECT pixels through changing inputs, themes/layouts, warnings, unavailable data and lifecycle transitions, and verify reduced redraw area. Glyph-centering checks exclude the speed-unit object rectangle because units now intentionally share the numeric ink color. Native build/package and twelve relevant host suites pass (unchanged phone IPC excluded). Actual smoothness/scanout changes require user panel validation; no FPS gain is claimed before measurement.

### Leading-zero speed and brighter shift alert (2026-10-27)

Available speed now retains three digit positions with 16% leading-zero opacity in Street, Track and the Settings transition; significant digits retain theme foreground, and unavailable readings remain dashes. The shift-alert gradient is brighter red; RPM thresholds and flashing timing are unchanged. Host rendering checks cover 0/1/9/10/60/99/100/180/400 and unavailable readings in both layouts/themes. All fourteen host suites pass (phone IPC requires local socket permission), and the display build/package passes without device access. Representative 800 x 480 layouts/themes, warnings and unavailable readings were visually inspected. Physical-panel brightness remains pending user validation.

## Touch Settings and boot verification (2026-10-29)

Shared boot duration is now 2,000 ms; shutdown remains 2,000 ms. Header gear and release-based touch navigation reuse the existing Settings reducer and access gates on host/native display. Non-touch controls remain available. All fourteen host suites pass (local-socket IPC run with socket permission); timing and touch boundary checks pass, and representative 800 × 480 Street/Track, themes, warning/unavailable and Settings frames were visually inspected. Settled redraw count remains zero. Display build/package passes without device access; updated touch behavior and artwork timing still require panel verification. Real radios and UART wiring remain pending; Appearance now has a bounded NVS worker.

Appearance follow-up: theme choices stack in the left column with a live shared-component dashboard preview across the right two columns. Parked Settings fixtures synthesize speed only, without selecting N or flashing left. Tight speed text layers and endpoint menu reveal reduce transition drawing work; actual smoothness requires panel feedback. Preference worker checks exercise restart, malformed records, queue saturation and open/write/commit errors without hardware.

User panel feedback (2026-10-29): after flashing this follow-up, the user reports everything working correctly, covering the revised Settings transition, parked fixture, Appearance preview and Apply behavior. This is user-reported functional acceptance; exact frame timing, retained power and power-cut durability remain unmeasured.

## One-way UART input bench (2026-10-29)

Implemented vehicle UART1 TX GPIO17 → display UART1 RX GPIO38 via GPIO_D/GND, with both boards separately USB powered and no power conductor joined. Reuses the v3 discrete-input codec/USB stream, adds bounded nonblocking display RX, visible waiting/live/stale status, overflow diagnostics and Trip/Info routing. Bench/demo selection remains explicit and isolated from live observations. Both build-only packages pass without device access. All fourteen host suites pass (IPC with local socket permission), along with receive-adapter/flash-guard fault checks. Representative UART live/stale frames were visually inspected at 800 × 480 in both layout/theme combinations; actual cable/inputs/reset/staleness still require user bench acceptance. Full E5 hello/capabilities/source-age/telemetry/return-command contract, radios and map transfer remain pending. This does not establish bike or production readiness.

User bench result (2026-10-29): both updated images were user-flashed. Initial UART WAITING was traced by the user to a loose connection; reseating it produced UART / LIVE. This confirms receipt of valid packets over the reported vehicle GPIO17 → display GPIO_D IO38/GND link with separate Mac USB power. Individual input stimulus, cable-loss/stale recovery, independent resets, soak and vehicle use remain separate pending tests.
