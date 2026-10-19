# Project status

Updated: 2026-10-19. Native LVGL host implementation and applicable host optimization pass complete; hardware firmware pending.

## Implemented

- Dual-ESP32 responsibility boundaries, normalized observations and explicit signal quality.
- C++17/LVGL 9.2.2/SDL2 host preview with shared UI source, seven reusable components, RAII ownership and centralized geometry.
- Street centered speed and Track centered current lap with secondary speed and best lap; Midnight/Daylight themes, 72 px gear/neutral display and bundled Inter/Barlow/Chakra Petch typography.
- Independent trip/info pages, semantic display actions and host keyboard adapter. ODO/trips/laps remain fixed simulated samples. TCS requests are handled only by the simulator.
- Automotive icon subsets, minimal-gap fuel segments, warning/shift presentation and 3,000 ms boot / 2,000 ms shutdown R15 Light Signature with urgent-warning reveal.
- Role-specific font subsets, minimal LVGL feature profile, change-aware mutations/formatting and deferred covered instrumentation. Host clock formatting updates by minute.

## Current verification

Build completes without compiler warnings. Eight suites pass: model/lifecycle, Light Signature timeline, portable presentation/input, desktop mappings, Bluetooth state/requests, host preferences, typography and LVGL rendering. There are 238 deterministic captures; fuel captures include 0/6/16/17/56/100% in both layouts/themes; representative layouts/themes/warnings/unavailable views were visually inspected. Tests cover fresh-versus-cached presentation, hidden telemetry/urgent reveal and repeated creation/destruction.

The LVGL tree contains 142 objects. One hundred identical settled updates cause zero flushes, versus 1,200 callbacks / 38,400,000 pixels before optimization. Font/icon bitmap payload is 90,564 bytes with the angular numeral subsets. The current Mac Dashboard wrapper is 1,424 bytes, including the last-input keys and Light Signature frame state. Host buffer sizes are unchanged. Font conversion is byte-reproducible; source licenses/provenance are bundled. The animation wordmark adds 12,288 bytes (12 KiB), replacing the former 336 KiB animation textures. Combined bitmap payload is 102,852 bytes and compiled host asset sections total 113,130 bytes. The source PNG is not a firmware resource.

See [resource budgets](resource-budgets.md) for repeatable reports and the limits of host measurements, and [optimization sequence](optimization-plan.md) for implementation details.

## Pending functionality and evidence

Wi-Fi/Navigation/clock screens, functional trips/laps, firmware NVS persistence, UART codec/integration, board drivers, firmware flashing and vehicle adapters are not implemented. Full Maps/Android Auto/CarPlay remains unresolved. No host test constitutes hardware or bike verification.

Exact delivered Elecrow PCB revision and vehicle MCU model remain unconfirmed. The user reports harness investigation, but detailed pin/voltage/polarity/waveform records have not been supplied here. No Yamaha protocol is treated as verified. Independent warning behavior and retained shutdown power require review and bench tests.

## Next actions

1. Confirm delivered board revision, matching schematic/factory example and vehicle MCU.
2. Establish the board panel smoke target and measure firmware/partition fit, internal SRAM/PSRAM, buffering, scanout and warning latency.
3. Implement remaining navigation/screens and further lifecycle artwork tuning with deterministic verification.
4. Review the UART contract, then implement codec and synthetic bench integration before measured bike inputs.
5. Define persistence ownership and power-loss behavior before implementing storage; retain OEM function during initial passive bike observation.

See [implementation stages](implementation-plan.md) and [engineering gates](engineering-review.md). No simulator, source build or asset size establishes production readiness.

Fuel now has partial-piece coverage, a forward slant and a 16% reserve indication derived from the user-supplied 11 L / approximately 1.76 L capacities. Portable coverage/reserve boundary checks and rendered mixed filled/empty piece checks pass. Actual tank/sensor calibration remains unverified.

OEM observation update: boot is 3 seconds; shutdown remains 2 seconds. Street trips are ODO/Trip 1/Trip 2. The seven info pages follow coolant, Track, real-time fuel, average fuel, average speed, TCS and an unidentified unavailable placeholder. Consumption/average speed are separate validated observations with static host samples; their vehicle-side measurement windows remain unverified.

Settings first slice: four-category home, semantic focus/back navigation, entry below 20 km/h, persistent gear/speed/tell-tales and reversible 280 ms speed transition. Both themes/layouts, warning/stale-speed interruption, exact dashboard restoration and zero settled Settings redraws pass. Appearance controls and confirmed theme/design persistence are implemented on the host; Bluetooth UI/demo is implemented; actual radio adapters and firmware NVS remain pending; see [Settings](settings.md).

Appearance second slice: Midnight/Daylight preview, current Instrument design, Apply/Cancel and confirmed host restart persistence. Storage corruption/failure/restart and preview-cancellation checks pass; Settings and Appearance stay idle without redundant flushes.

Bluetooth third slice: normalized quality-bearing service state, dynamic actions, pairing/connect progress, Disconnect, confirmed Forget and failure/retry/unavailable presentation. Eight suites pass with 238 deterministic captures. Host-only timed fixtures supply the radio observations; no real pairing, radio key storage or UART commands are implemented.
