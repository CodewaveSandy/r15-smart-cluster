# System architecture, status and roadmap

Updated: 2026-10-31. Shared host/native display UI, vehicle discrete inputs, return UART, authenticated BLE pairing/reconnect and phone metadata/clock synchronization are implemented. Core pairing/reconnect and phone metadata behavior are user bench-confirmed. Background execution, full telemetry, Wi-Fi and navigation/maps remain pending; extended hardware/load acceptance remains separate. [Implementation status and gates](architecture.md#current-status-and-roadmap) distinguish host evidence from hardware readiness.

The project is a personal Yamaha R15 V4 instrument cluster using an Elecrow 7-inch 800 x 480 HMI. The reported display module is ESP32-S3-WROOM-1-N4R8 with 4 MB flash and 8 MB PSRAM. Delivered PCB V3.0 is confirmed from user photos (2026-10-22). Esptool reports ESP32-S3 revision v0.2, 4 MB flash and embedded 8 MB PSRAM. Vehicle MCU and mechanical/environmental suitability remain unconfirmed. Commercialization, additional bikes and accessories are outside the immediate prototype scope.

## System boundaries

```text
Bike harness -> reviewed signal conditioning / CAN interface
                         |
                         v
                Vehicle ESP32
        acquisition -> validation -> calculations
        physical controls / Wi-Fi / BLE services
                         |
             versioned normalized UART messages
                         |
                         v
                Display ESP32-S3
        parser -> state store -> presenter -> LVGL
                         |
                   800 x 480 LCD
```

Protected automotive supply -> reviewed regulation -> board power inputs.
Power, grounding, reset, brownout and ignition sequencing remain system dependencies even though their circuit design is outside the UI implementation.

| Responsibility | Owner | Boundary |
| --- | --- | --- |
| Harness sampling, signal polarity, calibration, CAN decoding | Vehicle MCU | Only measured/documented mappings; no raw frames in UI |
| Speed/RPM normalization, fuel conversion, trips and ride calculations | Vehicle MCU | Explicit units, quality and provenance |
| Session lap timing | Vehicle MCU proposed | Monotonic; UI formats elapsed snapshots; link loss stops presentation extrapolation |
| Wi-Fi/BLE and eventual phone services | Vehicle MCU | Bounded work; never starve acquisition; feature capability explicit |
| UART serialization/parser | Shared portable protocol code | No LVGL or device drivers |
| Display state, screen focus and formatting | Display application | Coherent snapshots; no vehicle calculations |
| Screens, widgets, theme and animation | LVGL presentation | One LVGL owner; no transport or storage calls |
| Theme, units, local display preferences | Display settings service | Versioned, validated, coalesced NVS writes outside rendering |
| Vehicle-owned odometer | Unresolved vehicle contract | Never seed from demo or overwrite bike data |
| Time source and retained clock | Display Clock service; authenticated phone or USB bench source | Monotonic ticking after validated UTC/offset sync; no RTC backup or cold-power retention claim |

The user identified the vehicle test board as a 7SEMI ESP32-S3 DevKit and reported N8R8; module/flash identity still needs device verification. The vehicle MCU is not presumed safety certified; allocating radios and acquisition to it requires load/failure testing. Independent critical indications and safe behavior if either MCU fails remain unresolved.

## Stack and compatibility

- ESP-IDF with FreeRTOS, CMake, and C++17 application/domain code.
- Host preview uses LVGL 9.2.2 with an SHA-256-pinned source archive. Board firmware compatibility, esp_lcd RGB driver and board port remain pending.
- An SDL desktop LVGL simulator builds the same screens/presenters with host adapters.
- C interfaces at vendor boundaries; ordinary structs/scoped enums and explicit ownership internally. No host GUI framework dependency in firmware or portable code.
- Pin exact ESP-IDF, LVGL, component and font-converter versions together only after vendor-example compatibility is established. Do not combine LVGL 8 examples with LVGL 9 APIs.
- Prefer hand-maintained focused widgets. No GUI generator or new framework is required.

ESP-IDF is recommended for explicit task, memory, driver and diagnostic control. First reproduce the vendor's exact working example for the actual revision, even if it uses a different toolchain; then validate the proposed stack with a minimal panel test before implementing screens.

## Independent build targets

Decision recorded 2026-10-21: the product has **three independently built deployables**, plus one development-only host executable. These are build targets, not three unrelated build technologies or repositories.

| Target | Destination and responsibility | Build tooling | Current evidence |
| --- | --- | --- | --- |
| Display firmware | Display ESP32: LVGL, instruments, map rendering and display-owned preferences | Separate ESP-IDF/CMake project/configuration and firmware image; C++17 | Vendor V3.0 baseline flashed by user; R15 ESP-IDF target under `firmware/display`, initial user panel observations recorded, current updates pending panel measurements |
| Vehicle firmware | Other ESP32: acquisition, validation, calculations, physical inputs, Wi-Fi/BLE and bounded phone-data forwarding | Separate ESP-IDF/CMake project/configuration and firmware image; C++17 | Discrete-input UART, return controls, BLE pairing/reconnect and metadata implemented; full telemetry/Wi-Fi pending |
| Companion app | Android phone first; GPS, routing/map preparation, permitted notifications/media; future iOS | Flutter/Dart, Android Gradle for APK/AAB; future iOS Xcode tooling | Android pairing/reconnect, Home and phone metadata implemented; background/telemetry/navigation pending |
| Host cluster preview | Mac development simulation using shared display sources; no ESP32 flashing | Existing CMake/SDL2/LVGL target | Implemented; `bash scripts/preview.sh` |

Mobile launcher: `bash scripts/preview-mobile.sh`. It runs only the phone app; it does not start or pair with the Mac preview. The opt-in [Wi-Fi Mac phone-service bridge](../mobile/docs/development.md#mac-preview-adapter) substitutes for the vehicle service during development and is not a fourth product firmware.

Keep these targets in this repository with separate entry points, board configurations, dependency locks and output directories. Build/flash each ESP32 with its own image and verified chip/port; never reuse a display image for the vehicle MCU or assume both chips have identical configuration. A new app release need not reflash firmware when negotiated protocol versions/capabilities remain compatible. Protocol changes require compatibility tests across affected targets.

Share portable C++ domain/protocol logic between firmware and host where appropriate. Never copy the dashboard into a separate competing firmware implementation. Dart and C++ share a normative contract/test vectors rather than assuming native structs or the same language runtime. Real board project directories and flash commands are added only after board identities and toolchains are verified; existing root CMake is the host build, not an ESP-IDF firmware build.

## Display concurrency and lifetime

Proposed full-protocol execution model: a bounded UART receive/parser task plus one UI task that owns all LVGL calls, state presentation and navigation. ISR callbacks only signal driver completion or queue bounded events using supported APIs. Settings storage runs through a separate service/task if writes can block. Avoid extra tasks until measured need exists; assign priorities/core affinity from profiling, not habit.

The parser publishes a coherent latest-state mailbox; the UI consumes the newest valid state rather than replaying an old telemetry backlog. Physical navigation uses a separate bounded ordered queue. Queue overflow is observable; critical state is recoverable from full snapshots. Define command idempotence and release/cancel behavior before implementing held buttons.

Create widgets on screen entry or startup under one explicit owner; retain frequently used dashboard objects, update only changed fields, and unregister callbacks/timers on destruction. No widget pointers may cross into the acquisition or parser task. Bound all queues, text buffers, packet buffers, logs and diagnostic histories. No per-update allocation by application code in the telemetry/render hot path.

## Normalized state and failure behavior

Use portable value types with explicit units and fixed-width wire encodings. Each signal carries quality: available, unsupported, unavailable, invalid or stale. Connection/session status is independent of individual signal quality. False warning, neutral gear and zero speed are valid observations only when quality permits.

Track source age and local monotonic receipt age separately. A fresh UART heartbeat must not make an old CAN observation fresh. Restart/session identity invalidates old observations and prevents sequence comparisons across boots. Reject malformed values without partially publishing a snapshot. Repeated invalid input cannot refresh freshness. Critical invalidity must become visible by an explicit signal-status update or its bounded expiry.

On display boot show unavailable readings until compatible valid state arrives. Link timeout masks affected values and makes status visible. Reconnect needs a new compatible session/full snapshot. Do not turn unknown warning states into reassuring inactive states; define a visible unknown/fault treatment before vehicle use. Preserve immediate valid warning activation; decorative transitions cannot delay it.

Brightness/backlight fault handling, watchdog coverage, acquisition overrun and MCU restart diagnostics need bench tests. A watchdog is recovery machinery, not proof of safe indication. Initial vehicle tests remain passive/read-only with OEM functionality retained.

## Source map

| Path | Implemented responsibility |
| --- | --- |
| `shared/model/` | Portable normalized observations, quality and validation |
| `embedded/application/` | Lifecycle, preferences schema, semantic actions and menu state |
| `embedded/presentation/` | Bounded formatting, quality policies and animation/transition math |
| `embedded/ui/` | LVGL composition, components, geometry, typography and themes |
| `simulator/lvgl/` | SDL host, keyboard adapter, preference file adapter and timed radio fixtures |

| `assets/`, `scripts/`, `tools/fonts/` | Licensed bundled assets, reproducible conversion and preview/audit tools |

The `firmware/display` board adapter uses ESP-IDF RGB/I2C APIs with pins, timings and touch reset derived from the user-tested Elecrow V3.0 example. PlatformIO orchestrates this separate ESP-IDF/CMake target; it does not use Arduino. The existing portable v3 discrete-input codec now serves the GPIO17→GPIO38 bench link; phone status/control/identity/metadata sidebands are implemented, while full vehicle telemetry/navigation/map contracts remain pending. No empty firmware or protocol folders imply an implementation. Shared model and application/presentation logic stay independent of LVGL, FreeRTOS and device SDKs.

## Current UI composition

The shared LVGL `Dashboard` is a composition shell for eleven focused components under `embedded/ui/components/`: header, tell-tales, tachometer, primary readout, ride information, fuel, Settings home, Appearance, Phone, Clock & Date and lifecycle artwork. Both modes reuse the same persistent objects. Geometry lives in `embedded/ui/Geometry.h`; theme/behavior tokens and font roles remain in `Tokens.h` and `Typography.h`. Components own their top-level LVGL objects; LVGL owns descendants. RAII destroys components before the dashboard root, under the single LVGL owner. A persistent instrument-visibility group and Settings root support the screen transition; no per-component tasks are introduced. Portable formatting and quality decisions live in embedded/presentation. DisplayOptions and DashboardInput hold display-local preferences/actions; the host keyboard adapter maps keys to those actions and simulates requested TCS changes. SettingsNavigation owns semantic category focus/gating; SettingsTransition owns the monotonic 280 ms progress without LVGL. Preferences holds a versioned display-owned theme/design record. The host PreferenceFile adapter loads/saves outside rendering; display NVS loads validated preferences at startup and saves via one bounded storage worker. SettingsNavigation emits semantic Phone actions; PhonePanel reads normalized PhoneState. The host PhoneBridge supplies validated phone observations and forwards semantic commands to the development service; native vehicle Phone actions now use the bounded UART sideband and NimBLE backend described in [BLE bench](development.md#vehicle-ble-uart-bench); application data services remain pending. Legacy radio fixtures remain isolated adapter tests. See [Settings](ui.md#settings-behavior).

## Agreed physical ownership

Vehicle ESP32 owns acquisition, validation, calculations, physical inputs and Bluetooth/Wi-Fi. Display ESP32 owns local Settings, presentation, warning visibility and LVGL. UART carries validated normalized observations and explicit requests/results; vehicle packets never manipulate widgets. Display-to-vehicle requests do not optimistically change confirmed state. Stale/lost communication stays explicit and is never interpreted as key-off.

Both boards have separate power branches, as specified in [power lifecycle](ui.md#agreed-power-topology). The supplied four-wire cable fits multiple connectors; the selected bench path is vehicle GPIO17 → display GPIO_D IO38 plus GND. UART0 shares the USB bridge and is not used for this receive path. No power is transferred between the boards through that cable in the USB bench setup.

The current narrow input bench uses bounded nonblocking UART polling on app_main before rendering; no parser task or cross-task LVGL access is introduced. See the [display bench contract](../firmware/display/README.md#vehicle-to-display-uart-bench).

## UART contract proposal

The [vehicle discrete-input test](../firmware/vehicle/README.md) samples configurable pins, defaulting GPIO4/5 to left/right. Portable version 3 packets carry N–6, three lamps and debounced Trip/Info press counters over USB to the Mac; the host serial adapter publishes normalized observations under the existing LVGL owner. All twelve bench inputs are enabled with weak pull-downs: left/right GPIO4/5, N GPIO6, high beam GPIO7, gears 1–6 GPIO8–13, Trip/Info GPIO14/15. The vehicle flash script verifies the chip/capacity and preserves a verified full backup before writing. Turn phase is explicit to prevent double blinking. The same packet is now mirrored over dedicated UART1 GPIO17 to the display UART1 GPIO38 receiver; this narrow input bench is separate from the full inter-MCU UART proposal below. Reported active levels are 2–3.3 V; 2 V needs resolution against the GPIO HIGH threshold. Exact module/flash and board schematic validation remain pending.

UART carries normalized observations and semantic actions, never framebuffer pixels. Display TX, if used, carries a tightly limited application command set such as trip/lap actions, not arbitrary CAN forwarding. Receive-only telemetry is sufficient for the first display bring-up. Do not expose vehicle control or radio management commands by default.

Candidate: binary packets with COBS delimiter framing and CRC-32/ISO-HDLC integrity checking. This is not a finalized wire specification. Before implementing, specify byte order, CRC parameters/coverage, max decoded/encoded lengths, version policy, message schemas/units/ranges, sequence wrap, session IDs, source age, parser timeouts and golden vectors. Do not transmit native C++ struct memory.

Required types: hello/capabilities/session, observation snapshots, input action, source health. Commands and acknowledgements follow only when needed; define IDs, timeout, duplicate handling and rejection before enabling them. Health separates UART status from vehicle-source status. Log/debug text must use a different channel from framed telemetry, especially if the board exposes the flashing console UART.

Proposed starting point: 115200 baud, 8N1, periodic full snapshots at 20 Hz plus prompt warning changes, with a maximum decoded frame of 256 bytes. At a conservative 260 encoded bytes including delimiter and 10 bits/byte, 20 frames/s uses 52,000 bit/s (~45%); budget event/health/input traffic separately and enforce bounded rates. Revise from measured latency and payload size. Freshness deadlines must follow measured source cadence; a host test timeout is not automatically a vehicle requirement. The current bounded Wi-Fi/Bluetooth models must not be assumed to fit that proposed frame limit: budget separate radio-status/scan messages and revisit framing limits before defining the codec.

## Resource and performance policy

See [resource budgets](architecture.md#resource-accounting-and-target-verification) for current host evidence, buffer arithmetic and target measurements. Start with RGB565 software rendering; select partial/direct buffering only after driver, VSYNC, DMA and PSRAM validation. Initial goals are 30 Hz UI, 20 Hz telemetry and <=100 ms accepted-UART-state to visible update under load. These are provisional goals, not panel or warning-latency guarantees.

## Hardware facts and gates

User reports identifying most major functions at a 20-pin harness. No pin numbers, voltage/polarity records, waveforms, CAN bitrate or decoded messages have been supplied here; none is encoded as verified knowledge.

The user supplied the [Elecrow CrowPanel 7-inch HMI reference](https://www.elecrow.com/wiki/esp32-display-702727-intelligent-touch-screen-wi-fi26ble-800480-hmi-display.html) on 2026-10-18, identifying the intended product family. Delivered PCB V3.0 is confirmed from user photos. Vendor documentation describes revision-specific initialization and UART/console sharing; marketing connector counts do not establish independent free UARTs. Do not select pins or LCD timing before matching the actual schematic and factory example. USB programming/logging and inter-MCU UART must be checked together for contention and boot text.

CAN H/L -> appropriate protected CAN transceiver bus pins -> controller logic interface. Ordinary resistor/opto input conditioning is not a CAN differential front end. If galvanic isolation is required, design signal and power isolation together with a suitable isolated interface; do not assume optocouplers alone provide system isolation. Do not add CAN termination without confirming bus topology. Confirm classic CAN versus CAN FD before choosing the vehicle controller/transceiver. ESP32-S3 TWAI is not a CAN FD solution.

Other harness signals require individual measured voltage/polarity, pull-up/loading, frequency and fault limits before choosing resistor/opto/ADC/pulse conditioning; one generic circuit is not approved for all wires. Board 5 V power input does not imply 5 V GPIO tolerance. The display's reported 5 V/2 A input is not a verified power budget for both MCUs. Review power loss/cranking/reset sequencing before bike testing.

## Requirements that need resolution

On 2026-10-21 the user selected an Android-first Flutter companion app with future iOS support, retaining exactly two ESP32s. Android Auto/CarPlay and the existing Google Maps app are not requirements. A real moving road map, directions, notifications and music information remain requirements; embedded rendering, licensed map delivery and end-to-end bandwidth remain feasibility gates. Phone owns GPS/routing/map preparation; vehicle MCU owns radios and bounded forwarding; display MCU owns LVGL/map presentation. Audio stays phone-to-headset initially. See the [companion architecture](../mobile/docs/architecture.md#companion-architecture-status-and-approved-ui), [journey](../mobile/docs/architecture.md#rider-journey-and-truthful-state) and [implementation plan](../mobile/docs/architecture.md#current-status-and-roadmap). BLE plus Wi-Fi does not by itself prove map capability. Do not silently replace the moving map with arrows or a route on an empty background.

The seven-inch board's physical envelope, viewing angle, brightness, weather exposure and OEM fit need measurement. Purchasing the board does not establish mechanical or environmental suitability.

## Sources reviewed

Official sources checked 2026-10-18; mutable documentation is background until versions are pinned.

- [Elecrow 7-inch CrowPanel wiki and revision resources](https://media-cdn.elecrow.com/wiki/esp32-display-702727-intelligent-touch-screen-wi-fi26ble-800480-hmi-display.html): candidate family, revision differences and schematic/example discovery. Not confirmation of the purchased revision.
- [Espressif RGB LCD driver](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/lcd/rgb_lcd.html): scanout buffering and PSRAM/bandwidth tradeoffs.
- [LVGL 9.2 display integration](https://lvgl.io/docs/open/9.2/porting/display): render modes and flush ownership; display integration reference; host dependency is pinned to 9.2.2.
- [LVGL integration/thread ownership](https://github.com/lvgl/lvgl/blob/master/docs/src/integration/overview.mdx): serialize access to LVGL.
- [Espressif ESP32-S3 TWAI](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/twai.html): controller limitations and listen-only behavior.
- [TI isolated CAN interface guide](https://www.ti.com/document-viewer/lit/html/slla484): isolation belongs in a designed transceiver interface.

## Ignition lifecycle

Implement the state and timing contract in [power lifecycle](ui.md#power-lifecycle): key-on -> 2.00 s boot animation -> instrumentation; key-off -> 2.00 s shutdown animation -> controlled power removal. Acquisition runs during artwork. Retained power, readiness coordination and fault behavior must be bench verified.

The [nearby pairing contract](../mobile/docs/development.md#mac-preview-adapter) describes actual Mac/Android BLE discovery, first-pair approval, Keychain/Keystore persistence and the remaining ESP32 access-point provisioning boundary. The current Mac bridge is a development adapter, not firmware ownership moving into the display MCU.

## Vehicle BLE bench implementation

See [the normative bench contract](development.md#vehicle-ble-uart-bench). R15G input snapshots remain independent of R15P phone status/control. Display console TX43 supplies the return channel to vehicle RX18; vehicle TX17 still supplies display RX38. BLE host callbacks own radio and vehicle NVS; a bounded expiring queue bridges semantic requests. The LVGL owner consumes validated snapshots only. BLE bond authentication is separate from future companion application authorization.

Optional pairing labels use a narrowly scoped write-only vehicle GATT service and the bounded R15N sideband, matched to the active pairing challenge. Phone names never authorize bonds or commands. The display retains the existing LVGL owner and persistent modal objects. See [phone-name contract](../mobile/docs/protocol.md#enrollment-name-c011) for the app handoff and compatibility.

Connected phone metadata is now vehicle-validated through an authenticated BLE write, then forwarded over bounded CRC-protected R15M frames tied to the vehicle boot and BLE connection. Display owns clock synchronization and rendering. See [metadata contract and app handoff](../mobile/docs/protocol.md#connected-metadata-c012); core physical metadata acceptance is user-confirmed; extended fault/load acceptance remains pending.


## Resource accounting and target verification

Regenerate current metrics instead of using an old image size as today's budget:

```sh
cmake --build build/lvgl --parallel 4
SDL_VIDEODRIVER=dummy bash scripts/preview.sh --render build/lvgl/previews
python3 scripts/audit-resources.py --build-dir build/lvgl --output build/lvgl/previews/resource-audit.json
```

`host-metrics.json` reports redraw/object/wrapper/buffer diagnostics; `resource-audit.json` reports compiled host asset sections and payloads, excluding debug sections. Host ABI/sections and elapsed microseconds are not ESP32 flash, heap or FPS. Source TTF/PNG inputs are not all runtime allocations. Rebuild current firmware and read its image/partition reports before claiming fit; current native code uses ESP-IDF 5.5.4, LVGL 9.2.2, C++17 and the pinned PlatformIO toolchain.

| RGB565 allocation | Bytes |
| --- | ---: |
| One 800 × 480 framebuffer | 768,000 |
| Two full framebuffers | 1,536,000 |
| One 800 × 40 partial draw buffer | 64,000 |
| Two such partial buffers | 128,000 |

These are alternatives, not a prescription to allocate all. The display uses two PSRAM scanout buffers in DIRECT mode and internal bounce buffers; validate actual placement, alignment, DMA/cache/VSYNC ownership and flush completion. PSRAM cannot substitute for internal DMA/task-stack headroom.

| Required target evidence | Measure under representative peak work |
| --- | --- |
| Flash | Actual images/sections, bootloader/partitions/NVS and remaining margin; OTA only if real slots/images fit |
| Internal SRAM | Static plus driver/NimBLE allocations, DMA buffers, all stacks/queues; minimum free, largest block, stack high-water |
| PSRAM | Framebuffer/cache ownership, minimum free/largest block and fragmentation after repeated page/theme changes |
| Rendering | Render/flush/dirty area, VSYNC/tearing/color ordering and visible warning latency |
| Communication | Source/receipt freshness, queue high-water, errors/overflow, radio/acquisition contention, reboot/reconnect |
| Persistence/power | Commit latency, rendering contention, durability/wear, retained shutdown power and rapid cycling |

The Light Signature uses bounded line/triangle work and a 192 × 64 A8 mask, with no full-screen texture/PNG decoder. Component-owned persistent numeric text buffers reduce label allocations; this does not prove all LVGL rendering is allocation-free. SDL uploads only after dirty flushes/repaint as needed; host redraw reductions do not establish panel FPS. Record revision, configuration, load, method and duration with every measurement.

Next work is [firmware background continuity](background-sync.md); Android owns background scheduling. Acquisition, warnings and both two-second lifecycle sequences must remain independent of optional phone/map work.

## Current status and roadmap

Updated: 2026-10-31. Current status and future gates only. Automated test suites are deferred by user instruction; do not create/restore/run them. Use builds, runtime/static diagnostics, development previews and manual bench/device checks. [Architecture](architecture.md#system-architecture-status-and-roadmap), [hardware](development.md#hardware-wiring-and-observed-checks), target READMEs and [UI/lifecycle](ui.md#ui-settings-and-lifecycle) remain authoritative for their topics.

### Implemented baseline and evidence

| Area | State and evidence |
| --- | --- |
| Shared host/display UI | Street/Track, Midnight/Daylight, approved instruments/warnings, Settings/Appearance/Clock & Date/Phone, non-touch actions and 2.00 s boot/shutdown implemented |
| Display target | ESP-IDF 5.5.4/LVGL 9.2.2 native V3.0 target builds/packages; initial layout/theme/touch/reset user-confirmed; current scanout/timing/load measurements remain separate |
| Preferences/clock | Validated host/native display preferences and bounded NVS worker; USB or authenticated phone UTC/offset, monotonic ticking; no retained RTC/cold-power time claim |
| Vehicle bench | Configurable discrete inputs/debounced Trip/Info, R15G UART, return controls and independent board power branches; core GPIO/UART behavior user-confirmed |
| Phone link | Numeric-comparison authenticated BLE, one saved bond, encrypted saved reconnect and current phone name/battery/time; core companion path user-confirmed |
| Development adapter | Separate opt-in Mac BLE/TLS/IPC preview; not physical ESP32 application/telemetry/navigation readiness |
| Pending | Android background continuity acceptance, real phone-bound vehicle telemetry, GPS/navigation/map contracts and rendering, Wi-Fi bulk, real calculations/adapters, extended hardware/power/resource verification |

All demo odometer/trip/lap/economy/average-speed values are labelled samples, not real calculated/persistent vehicle data. Phone success does not establish unavailable vehicle signals. User-selected display thresholds are not verified Yamaha limits. Full moving road maps remain required but unproved on the embedded path.

### Next sequence

| Gate | Firmware work | Exit evidence |
| --- | --- | --- |
| F1 / joint B1 | Follow [background-sync plan](background-sync.md): preserve existing endpoint/security, prepare bounded observations, support Android's service continuity run | Current firmware may need no change; 60-minute locked-phone soak, recovery/expiry/Stop and concurrent inputs/UI remain correct |
| F2 / joint B2 | Freeze and implement authenticated capabilities/telemetry notifications and bounded session recovery with mobile | Current real observations delivered to phone, source expiry/sequence/boot handling and resubscription; no invented telemetry/calibration |
| F3 / joint B3 | Review provider/data rights, compact navigation and bulk/map representation; measure UART/radio/display caches and rendering before expansion | Real licensed moving-map/route path through both MCUs, cancellation/stale/loss behavior and measured budgets; arrows alone are intermediate |
| Hardware continuation | Complete measured acquisition/calculations/persistence, passive bike observation and supply/retained-power/failure review | Measured signal mapping/provenance, OEM retained, no active bike transmission without explicit reviewed authorization |
| Replacement/release evaluation | Independent critical indications, reset/power/mechanical/environmental/update/security review | Explicit hardware/bike evidence; no compliance/production claim inferred from software |

Do not change pins, baud, task ownership or security to mask a phone lifecycle bug. Firmware owns acquisition/radios, display owns LVGL/preferences and Android owns its foreground service. Read [protocol](../mobile/docs/protocol.md) before any compatible wire change; F2/F3 need coordinated specification, not invented UUIDs/fields. Keep existing c011/c012 and UART behavior compatible until a reviewed coordinated update.

### Hardware, resource and safety gates

Delivered Elecrow DIS08070H PCB V3.0 and esptool ESP32-S3 revision v0.2 / 4 MB flash / 8 MB PSRAM are recorded. Vendor example was built/flashed with stable picture/touch; the verified factory backup, provenance/hashes and restoration route are retained in [display README](../firmware/display/README.md). The vehicle board is user-reported 7SEMI ESP32-S3 N8R8; verify actual identity/capacity rather than assuming it from a label. Build/flash commands retain backup/chip/partition guards.

[Hardware connections](development.md#hardware-wiring-and-observed-checks) records engine-off bike power through the reported buck supply. Cranking/running, supply protection, retention and environmental suitability are unverified. No direct vehicle voltage to GPIO; the reported low active level is not guaranteed HIGH at 3.3 V. Wiring/configured polarity belongs to [vehicle README](../firmware/vehicle/README.md) and [UART bench](development.md#vehicle-ble-uart-bench). Do not infer cable orientation/pinout from colors.

Measure actual internal heap/largest block, PSRAM, stacks, queues, radio/acquisition latency, source age, overflow and render/flush behavior under concurrent inputs, NVS and phone load. [Resource accounting](architecture.md#resource-accounting-and-target-verification) defines evidence; host numbers/static image fit do not prove runtime headroom. Reproduce rapid ignition/reset/UART loss and warning combinations. Shutdown requires retained supply; firmware cannot guarantee a two-second animation after hard power removal.

Keep real warning unknown/failure behavior explicit and define independent indications before replacement. Confirm signal provenance/calibration, odometer retention/reset authorization and power-loss atomicity/wear before real persistence. CAN requires a reviewed physical interface/transceiver/listen-only setup and measured IDs/bitrate; generic optocouplers directly on CAN H/L are not an approved interface. Preserve OEM function during initial passive observation.

### Remaining product decisions

Real sources/calibration for speed, RPM, fuel, gear, warnings, QS/VVA and vehicle calculations; mechanical/display visibility/weather suitability; ignition/retained power/readiness; future telemetry/map rates/limits; provider rights/route/map cache; Wi-Fi topology/cellular behavior; OTA image/slot/security feasibility and release ownership. Track decisions in the relevant existing authority, not new historical handoff files.

### Verification record policy

Core first pair, name/code approval, persistent link, saved reconnect after board power cycle, explicit Disconnect/Connect, app relaunch and connected phone name/battery/time are user-confirmed in the current development stage. Prior firmware build/package/partition and host preview checks passed; actual flashed hashes were not collected for every phone session. The mobile evidence and remaining fault checks are summarized in [development](../mobile/docs/development.md#current-evidence-and-remaining-checks).

Background locked-screen operation, sustained load, independent reboot/expiry/fault matrices, latest panel scanout performance, retained power and bike readiness remain open. For each new gate record revision/toolchain/configuration, device/image hashes, method/duration and observed diagnostics. No old automated-suite pass is a current gate; those suites were removed. Current cleanup/planning is documentation-only and supplies no new runtime evidence.
