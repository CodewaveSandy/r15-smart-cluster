# Project status

Updated: 2026-10-19. Native LVGL host implementation and applicable host optimization pass complete; hardware firmware pending.

## Implemented

- Dual-ESP32 responsibility boundaries, normalized observations and explicit signal quality.
- C++17/LVGL 9.2.2/SDL2 host preview with shared UI source, seven reusable components, RAII ownership and centralized geometry.
- Street centered speed and Track centered current lap with secondary speed and best lap; Midnight/Daylight themes, 72 px gear/neutral display and bundled Inter/Barlow typography.
- Independent trip/info pages, semantic display actions and host keyboard adapter. ODO/trips/laps remain fixed simulated samples. TCS requests are handled only by the simulator.
- Automotive icon subsets, minimal-gap fuel segments, warning/shift presentation and 1,250 ms initial boot/shutdown artwork with urgent-warning reveal.
- Role-specific font subsets, minimal LVGL feature profile, change-aware mutations/formatting and deferred covered instrumentation. Host clock formatting updates by minute.

## Current verification

Build completes without compiler warnings. Five suites pass: model/lifecycle, portable presentation/input, desktop mappings, typography and LVGL rendering. The 44 deterministic captures remain byte-identical through the optimization pass; representative layouts/themes/warnings/unavailable views were visually inspected. Tests cover fresh-versus-cached presentation, hidden telemetry/urgent reveal and repeated creation/destruction.

The LVGL tree contains 72 objects. One hundred identical settled updates cause zero flushes, versus 1,200 callbacks / 38,400,000 pixels before optimization. Bundled bitmap payload is 75,833 bytes, versus 90,391; host asset sections excluding debug are 85,796 bytes, versus 101,413. The current Mac Dashboard wrapper is 840 bytes, including a 192-byte increase for small last-input keys. Host buffer sizes are unchanged. Font conversion is byte-reproducible; source licenses/provenance are bundled.

See [resource budgets](resource-budgets.md) for repeatable reports and the limits of host measurements, and [optimization sequence](optimization-plan.md) for implementation details.

## Pending functionality and evidence

Settings/clock editing, functional trips/laps, persistence, UART codec/integration, board drivers, firmware flashing and vehicle adapters are not implemented. Full Maps/Android Auto/CarPlay remains unresolved. No host test constitutes hardware or bike verification.

Exact delivered Elecrow PCB revision and vehicle MCU model remain unconfirmed. The user reports harness investigation, but detailed pin/voltage/polarity/waveform records have not been supplied here. No Yamaha protocol is treated as verified. Independent warning behavior and retained shutdown power require review and bench tests.

## Next actions

1. Confirm delivered board revision, matching schematic/factory example and vehicle MCU.
2. Establish the board panel smoke target and measure firmware/partition fit, internal SRAM/PSRAM, buffering, scanout and warning latency.
3. Implement remaining navigation/screens and final lifecycle artwork with deterministic verification.
4. Review the UART contract, then implement codec and synthetic bench integration before measured bike inputs.
5. Define persistence ownership and power-loss behavior before implementing storage; retain OEM function during initial passive bike observation.

See [implementation stages](implementation-plan.md) and [engineering gates](engineering-review.md). No simulator, source build or asset size establishes production readiness.
