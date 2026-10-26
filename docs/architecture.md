# System architecture

Updated: 2026-10-22. The shared native LVGL host application is implemented. The display desk-test firmware builds; its first R15 panel validation, UART and vehicle adapters are pending. [Implementation status and gates](implementation-plan.md) distinguish host evidence from hardware readiness.

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
| Time source and retained clock | Explicit clock service | No assumption of RTC backup or Wi-Fi time availability |

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
| Display firmware | Display ESP32: LVGL, instruments, map rendering and display-owned preferences | Separate ESP-IDF/CMake project/configuration and firmware image; C++17 | Vendor V3.0 baseline flashed by user; R15 ESP-IDF target under `firmware/display`, pending its first panel verification |
| Vehicle firmware | Other ESP32: acquisition, validation, calculations, physical inputs, Wi-Fi/BLE and bounded phone-data forwarding | Separate ESP-IDF/CMake project/configuration and firmware image; C++17 | Pending MCU identification and implementation |
| Companion app | Android phone first; GPS, routing/map preparation, permitted notifications/media; future iOS | Flutter/Dart, Android Gradle for APK/AAB; future iOS Xcode tooling | Android debug foundation launched on user's phone; services pending |
| Host cluster preview | Mac development simulation using shared display sources; no ESP32 flashing | Existing CMake/SDL2/LVGL target | Implemented; `bash scripts/preview.sh` |

Mobile launcher: `bash scripts/preview-mobile.sh`. It runs only the phone app; it does not start or pair with the Mac preview. The opt-in [Wi-Fi Mac phone-service bridge](../mobile/docs/mac-preview-connection.md) substitutes for the vehicle service during development and is not a fourth product firmware.

Keep these targets in this repository with separate entry points, board configurations, dependency locks and output directories. Build/flash each ESP32 with its own image and verified chip/port; never reuse a display image for the vehicle MCU or assume both chips have identical configuration. A new app release need not reflash firmware when negotiated protocol versions/capabilities remain compatible. Protocol changes require compatibility tests across affected targets.

Share portable C++ domain/protocol logic between firmware and host where appropriate. Never copy the dashboard into a separate competing firmware implementation. Dart and C++ share a normative contract/test vectors rather than assuming native structs or the same language runtime. Real board project directories and flash commands are added only after board identities and toolchains are verified; existing root CMake is the host build, not an ESP-IDF firmware build.

## Display concurrency and lifetime

Proposed execution model: a bounded UART receive/parser task plus one UI task that owns all LVGL calls, state presentation and navigation. ISR callbacks only signal driver completion or queue bounded events using supported APIs. Settings storage runs through a separate service/task if writes can block. Avoid extra tasks until measured need exists; assign priorities/core affinity from profiling, not habit.

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
| `tests/embedded/` | Portable checks and deterministic LVGL verification |
| `assets/`, `scripts/`, `tools/fonts/` | Licensed bundled assets, reproducible conversion and preview/audit tools |

The `firmware/display` board adapter uses ESP-IDF RGB/I2C APIs with pins, timings and touch reset derived from the user-tested Elecrow V3.0 example. PlatformIO orchestrates this separate ESP-IDF/CMake target; it does not use Arduino. Shared UART codec remains pending. No empty firmware or protocol folders imply an implementation. Shared model and application/presentation logic stay independent of LVGL, FreeRTOS and device SDKs.

## Current UI composition

The shared LVGL `Dashboard` is a composition shell for ten focused components under `embedded/ui/components/`: header, tell-tales, tachometer, primary readout, ride information, fuel, Settings home, Appearance, Phone and lifecycle artwork. Both modes reuse the same persistent objects. Geometry lives in `embedded/ui/Geometry.h`; theme/behavior tokens and font roles remain in `Tokens.h` and `Typography.h`. Components own their top-level LVGL objects; LVGL owns descendants. RAII destroys components before the dashboard root, under the single LVGL owner. A persistent instrument-visibility group and Settings root support the screen transition; no per-component tasks are introduced. Portable formatting and quality decisions live in embedded/presentation. DisplayOptions and DashboardInput hold display-local preferences/actions; the host keyboard adapter maps keys to those actions and simulates requested TCS changes. SettingsNavigation owns semantic category focus/gating; SettingsTransition owns the monotonic 280 ms progress without LVGL. Preferences holds a versioned display-owned theme/design record. The host PreferenceFile adapter loads/saves outside rendering; ESP32 NVS remains pending. SettingsNavigation emits semantic Phone actions; PhonePanel reads normalized PhoneState. The host PhoneBridge supplies validated phone observations and forwards semantic commands to the development service; real vehicle command handling remains unimplemented. Legacy radio fixtures remain isolated adapter tests. See [Settings](settings.md).

## UART contract proposal

The [vehicle discrete-input test](../firmware/vehicle/README.md) samples configurable pins, defaulting GPIO4/5 to left/right. Portable version 3 packets carry N–6, three lamps and debounced Trip/Info press counters over USB to the Mac; the host serial adapter publishes normalized observations under the existing LVGL owner. All twelve bench inputs are enabled with weak pull-downs: left/right GPIO4/5, N GPIO6, high beam GPIO7, gears 1–6 GPIO8–13, Trip/Info GPIO14/15. The vehicle flash script verifies the chip/capacity and preserves a verified full backup before writing. Turn phase is explicit to prevent double blinking. This is separate from the full inter-MCU UART proposal below. Reported active levels are 2–3.3 V; 2 V needs resolution against the GPIO HIGH threshold. Exact module/flash and board schematic validation remain pending.

UART carries normalized observations and semantic actions, never framebuffer pixels. Display TX, if used, carries a tightly limited application command set such as trip/lap actions, not arbitrary CAN forwarding. Receive-only telemetry is sufficient for the first display bring-up. Do not expose vehicle control or radio management commands by default.

Candidate: binary packets with COBS delimiter framing and CRC-32/ISO-HDLC integrity checking. This is not a finalized wire specification. Before implementing, specify byte order, CRC parameters/coverage, max decoded/encoded lengths, version policy, message schemas/units/ranges, sequence wrap, session IDs, source age, parser timeouts and golden vectors. Do not transmit native C++ struct memory.

Required types: hello/capabilities/session, observation snapshots, input action, source health. Commands and acknowledgements follow only when needed; define IDs, timeout, duplicate handling and rejection before enabling them. Health separates UART status from vehicle-source status. Log/debug text must use a different channel from framed telemetry, especially if the board exposes the flashing console UART.

Proposed starting point: 115200 baud, 8N1, periodic full snapshots at 20 Hz plus prompt warning changes, with a maximum decoded frame of 256 bytes. At a conservative 260 encoded bytes including delimiter and 10 bits/byte, 20 frames/s uses 52,000 bit/s (~45%); budget event/health/input traffic separately and enforce bounded rates. Revise from measured latency and payload size. Freshness deadlines must follow measured source cadence; a host test timeout is not automatically a vehicle requirement. The current bounded Wi-Fi/Bluetooth models must not be assumed to fit that proposed frame limit: budget separate radio-status/scan messages and revisit framing limits before defining the codec.

## Resource and performance policy

See [resource budgets](resource-budgets.md) for current host evidence, buffer arithmetic and target measurements. Start with RGB565 software rendering; select partial/direct buffering only after driver, VSYNC, DMA and PSRAM validation. Initial goals are 30 Hz UI, 20 Hz telemetry and <=100 ms accepted-UART-state to visible update under load. These are provisional goals, not panel or warning-latency guarantees.

## Hardware facts and gates

User reports identifying most major functions at a 20-pin harness. No pin numbers, voltage/polarity records, waveforms, CAN bitrate or decoded messages have been supplied here; none is encoded as verified knowledge.

The user supplied the [Elecrow CrowPanel 7-inch HMI reference](https://www.elecrow.com/wiki/esp32-display-702727-intelligent-touch-screen-wi-fi26ble-800480-hmi-display.html) on 2026-10-18, identifying the intended product family. Exact delivered PCB revision remains to be confirmed. Vendor documentation describes revision-specific initialization and UART/console sharing; marketing connector counts do not establish independent free UARTs. Do not select pins or LCD timing before matching the actual schematic and factory example. USB programming/logging and inter-MCU UART must be checked together for contention and boot text.

CAN H/L -> appropriate protected CAN transceiver bus pins -> controller logic interface. Ordinary resistor/opto input conditioning is not a CAN differential front end. If galvanic isolation is required, design signal and power isolation together with a suitable isolated interface; do not assume optocouplers alone provide system isolation. Do not add CAN termination without confirming bus topology. Confirm classic CAN versus CAN FD before choosing the vehicle controller/transceiver. ESP32-S3 TWAI is not a CAN FD solution.

Other harness signals require individual measured voltage/polarity, pull-up/loading, frequency and fault limits before choosing resistor/opto/ADC/pulse conditioning; one generic circuit is not approved for all wires. Board 5 V power input does not imply 5 V GPIO tolerance. The display's reported 5 V/2 A input is not a verified power budget for both MCUs. Review power loss/cranking/reset sequencing before bike testing.

## Requirements that need resolution

On 2026-10-21 the user selected an Android-first Flutter companion app with future iOS support, retaining exactly two ESP32s. Android Auto/CarPlay and the existing Google Maps app are not requirements. A real moving road map, directions, notifications and music information remain requirements; embedded rendering, licensed map delivery and end-to-end bandwidth remain feasibility gates. Phone owns GPS/routing/map preparation; vehicle MCU owns radios and bounded forwarding; display MCU owns LVGL/map presentation. Audio stays phone-to-headset initially. See the [companion architecture](../mobile/docs/architecture.md), [journey](../mobile/docs/user-journey.md) and [implementation plan](../mobile/docs/implementation-plan.md). BLE plus Wi-Fi does not by itself prove map capability. Do not silently replace the moving map with arrows or a route on an empty background.

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

Implement the state and timing contract in [power lifecycle](power-lifecycle.md): key-on -> 3.00 s boot animation -> instrumentation; key-off -> 2.00 s shutdown animation -> controlled power removal. Acquisition runs during artwork. Retained power, readiness coordination and fault behavior must be bench verified.

The [nearby pairing contract](../mobile/docs/mac-preview-connection.md) describes actual Mac/Android BLE discovery, first-pair approval, Keychain/Keystore persistence and the remaining ESP32 access-point provisioning boundary. The current Mac bridge is a development adapter, not firmware ownership moving into the display MCU.
