# Dual-ESP32 embedded design

Date: 2026-10-18. Status: first LVGL host slice implemented; no hardware firmware implemented.
The user selected two ESP32 MCUs, a UART interconnect, and an LVGL port that implements the specified UI. Framework/version/buffer/protocol details below are proposals until their validation gates pass.

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

Vehicle MCU model is not yet specified. The vehicle MCU is not presumed safety certified; allocating radios and acquisition to it requires load/failure testing. Independent critical indications and safe behavior if either MCU fails remain unresolved.

## Software stack proposal

- ESP-IDF with FreeRTOS, CMake, and C++17 application/domain code.
- Host preview uses LVGL 9.2.2 with an SHA-256-pinned source archive. Board firmware compatibility, esp_lcd RGB driver and board port remain pending.
- An SDL desktop LVGL simulator builds the same screens/presenters with host adapters.
- C interfaces at vendor boundaries; ordinary structs/scoped enums and explicit ownership internally. No host GUI framework dependency in firmware or portable code.
- Pin exact ESP-IDF, LVGL, component and font-converter versions together only after vendor-example compatibility is established. Do not combine LVGL 8 examples with LVGL 9 APIs.
- Prefer hand-maintained focused widgets. No GUI generator or new framework is required.

ESP-IDF is recommended for explicit task, memory, driver and diagnostic control. First reproduce the vendor's exact working example for the actual revision, even if it uses a different toolchain; then validate the proposed stack with a minimal panel test before implementing screens.

## Display concurrency and lifetime

Proposed execution model: a bounded UART receive/parser task plus one UI task that owns all LVGL calls, state presentation and navigation. ISR callbacks only signal driver completion or queue bounded events using supported APIs. Settings storage runs through a separate service/task if writes can block. Avoid extra tasks until measured need exists; assign priorities/core affinity from profiling, not habit.

The parser publishes a coherent latest-state mailbox; the UI consumes the newest valid state rather than replaying an old telemetry backlog. Physical navigation uses a separate bounded ordered queue. Queue overflow is observable; critical state is recoverable from full snapshots. Define command idempotence and release/cancel behavior before implementing held buttons.

Create widgets on screen entry or startup under one explicit owner; retain frequently used dashboard objects, update only changed fields, and unregister callbacks/timers on destruction. No widget pointers may cross into the acquisition or parser task. Bound all queues, text buffers, packet buffers, logs and diagnostic histories. No per-update allocation by application code in the telemetry/render hot path.

## Normalized state and failure behavior

Use portable value types with explicit units and fixed-width wire encodings. Each signal carries quality: available, unsupported, unavailable, invalid or stale. Connection/session status is independent of individual signal quality. False warning, neutral gear and zero speed are valid observations only when quality permits.

Track source age and local monotonic receipt age separately. A fresh UART heartbeat must not make an old CAN observation fresh. Restart/session identity invalidates old observations and prevents sequence comparisons across boots. Reject malformed values without partially publishing a snapshot. Repeated invalid input cannot refresh freshness. Critical invalidity must become visible by an explicit signal-status update or its bounded expiry.

On display boot show unavailable readings until compatible valid state arrives. Link timeout masks affected values and makes status visible. Reconnect needs a new compatible session/full snapshot. Do not turn unknown warning states into reassuring inactive states; define a visible unknown/fault treatment before vehicle use. Preserve immediate valid warning activation; decorative transitions cannot delay it.

Brightness/backlight fault handling, watchdog coverage, acquisition overrun and MCU restart diagnostics need bench tests. A watchdog is recovery machinery, not proof of safe indication. Initial vehicle tests remain passive/read-only with OEM functionality retained.

## Proposed repository organization

```text
firmware/display/          ESP-IDF display executable and board configuration
firmware/vehicle/          ESP-IDF vehicle executable, when implementation starts
shared/model/              portable observations, quality and semantic input
shared/protocol/           bounded codec and golden packet vectors
embedded/ui/               LVGL screens, widgets, tokens, presenter
embedded/platform/         actual board/display, clock and storage adapters
simulator/lvgl/            host entry point and deterministic scenario adapter
tests/embedded/            portable behavior, codec, presenter and render checks
```

This is a responsibility map, not an instruction to create empty folders. Implement the specified UI against deterministic scenario captures. Shared model/protocol must not depend on LVGL, FreeRTOS or ESP-IDF. Vehicle firmware must not link UI. Host and display targets reuse the LVGL UI source; hardware port is the substitution point. Keep host and firmware build targets explicit.

Keep domain behavior, monotonic timing and storage services portable and independently testable.

## UART contract proposal

UART carries normalized observations and semantic actions, never framebuffer pixels. Display TX, if used, carries a tightly limited application command set such as trip/lap actions, not arbitrary CAN forwarding. Receive-only telemetry is sufficient for the first display bring-up. Do not expose vehicle control or radio management commands by default.

Candidate: binary packets with COBS delimiter framing and CRC-32/ISO-HDLC integrity checking. This is not a finalized wire specification. Before implementing, specify byte order, CRC parameters/coverage, max decoded/encoded lengths, version policy, message schemas/units/ranges, sequence wrap, session IDs, source age, parser timeouts and golden vectors. Do not transmit native C++ struct memory.

Required types: hello/capabilities/session, full observation snapshot, input action, source health. Commands and acknowledgements follow only when needed; define IDs, timeout, duplicate handling and rejection before enabling them. Health separates UART status from vehicle-source status. Log/debug text must use a different channel from framed telemetry, especially if the board exposes the flashing console UART.

Proposed starting point: 115200 baud, 8N1, periodic full snapshots at 20 Hz plus prompt warning changes, with a maximum decoded frame of 256 bytes. At a conservative 260 encoded bytes including delimiter and 10 bits/byte, 20 frames/s uses 52,000 bit/s (~45%); budget event/health/input traffic separately and enforce bounded rates. Revise from measured latency and payload size. Freshness deadlines must follow measured source cadence; a host test timeout is not automatically a vehicle requirement.

## Resource and performance plan

800 x 480 x 2-byte RGB565 = 768,000 bytes (750 KiB) per full framebuffer. Two total 1,536,000 bytes (~1.46 MiB). Two 800 x 40-line RGB565 draw buffers add 128,000 bytes (125 KiB). These are arithmetic examples, not final allocations; LVGL draw buffers, scanout framebuffers and driver bounce buffers have different jobs.

Start with RGB565 and software rendering. Compare partial rendering into the board driver's framebuffer with supported direct/double-buffer modes; choose after inspecting flush completion, VSYNC, cache and DMA ownership. PSRAM capacity alone does not prove sufficient bandwidth. Flash writes and full-screen shift flashes must be included in stress tests.

| Resource | Initial policy / evidence required |
| --- | --- |
| 4 MB reported flash | Measure app/assets/NVS/boot overhead; do not promise dual-slot OTA until two real images fit |
| 8 MB reported PSRAM | Measure framebuffers, caches, fonts and largest free block, leaving explicit headroom |
| Internal SRAM | Budget task stacks, DMA/bounce buffers, queues and driver needs independently |
| Fonts | Convert bundled Inter/Barlow to deterministic subset assets; digits-only large fonts, required UI glyphs at small sizes |
| Drawing | Persistent widgets, fixed numeric bounds, changed-field updates, clipped RPM gradient, no unnecessary full-screen invalidation |
| Update cadence | Initial UI target 30 Hz, telemetry 20 Hz; measured goals, not panel refresh specifications or guarantees |
| Latency | Initial target <=100 ms accepted UART state to visible update under load; establish separate source-to-display warning requirements before vehicle use |
| Stability | Record heap/stack high-water marks, frame/render times, packet errors, dropped events and panel artifacts during soak tests |

Large PSRAM allocations and flash partition design are decided before extra themes/assets/features. Avoid image-per-state UI, runtime vector parsers, unnecessary alpha layers, large shadows and animation queues. An 800 x 1 RGB565 gradient ramp is only 1,600 bytes before metadata; test a ramp/custom draw path instead of storing a full-screen tachometer image.

## Hardware facts and gates

User reports identifying most major functions at a 20-pin harness. No pin numbers, voltage/polarity records, waveforms, CAN bitrate or decoded messages have been supplied here; none is encoded as verified knowledge.

The user supplied the [Elecrow CrowPanel 7-inch HMI reference](https://www.elecrow.com/wiki/esp32-display-702727-intelligent-touch-screen-wi-fi26ble-800480-hmi-display.html) on 2026-10-18, identifying the intended product family. Exact delivered PCB revision remains to be confirmed. The supplied URL returned HTTP 403 during retrieval; the vendor CDN copy linked below was reviewed during planning. Vendor documentation describes revision-specific initialization and UART/console sharing; marketing connector counts do not establish independent free UARTs. Do not select pins or LCD timing before matching the actual schematic and factory example. USB programming/logging and inter-MCU UART must be checked together for contention and boot text.

CAN H/L -> appropriate protected CAN transceiver bus pins -> controller logic interface. Ordinary resistor/opto input conditioning is not a CAN differential front end. If galvanic isolation is required, design signal and power isolation together with a suitable isolated interface; do not assume optocouplers alone provide system isolation. Do not add CAN termination without confirming bus topology. Confirm classic CAN versus CAN FD before choosing the vehicle controller/transceiver. ESP32-S3 TWAI is not a CAN FD solution.

Other harness signals require individual measured voltage/polarity, pull-up/loading, frequency and fault limits before choosing resistor/opto/ADC/pulse conditioning; one generic circuit is not approved for all wires. Board 5 V power input does not imply 5 V GPIO tolerance. The display's reported 5 V/2 A input is not a verified power budget for both MCUs. Review power loss/cranking/reset sequencing before bike testing.

## Requirements that need resolution

Full Google Maps, Android Auto/CarPlay, calls and audio are product requirements with unresolved implementation feasibility. This two-ESP32 LVGL design establishes no projection receiver, decoder, rendering or licensing path. Wi-Fi/BLE support alone does not establish those capabilities. The user explicitly confirmed on 2026-10-18 that full Maps/projection should remain an unresolved requirement to investigate. Retain it pending a separate feasibility proof; do not silently replace full Maps with arrows or a fake map.

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

Implement the state and timing contract in [power lifecycle](power-lifecycle.md): key-on -> 2.00 s boot animation -> instrumentation; key-off -> 2.00 s shutdown animation -> controlled power removal. Acquisition runs during artwork. Retained power, readiness coordination and fault behavior must be bench verified.
