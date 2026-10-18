# Project status

Baseline: 2026-10-18. Current phase: first host implementation.

## Established direction

- Two ESP32s, UART interconnect and LVGL display rendering.
- Elecrow 7-inch 800 x 480 HMI family selected; user supplied the vendor wiki. Delivered PCB revision and vehicle MCU model remain unconfirmed.
- UI geometry, themes, typography and instrument behavior specified in the style guide.
- Key-on boot and key-off shutdown animations each specified at 1–1.5 s. Retained power circuit and total startup timing remain unverified.
- User reports investigating the 20-pin harness and identifying most major functions. Detailed pin/voltage/polarity/waveform evidence has not yet been supplied here.
- Full Maps/projection remains unresolved by explicit user choice.

## Current deliverables

Architecture, resource/concurrency proposals, style guide, power lifecycle, implementation stages and verification gates are documented. Active documentation describes the project baseline. The first LVGL host slice is implemented: portable observations/quality, model validation, monotonic ignition lifecycle, shared palettes, Bricolage subsets, dashboard layouts/themes and keyboard simulator. Default CMake and scripts/preview.sh build the native SDL2 preview. Host dependency: LVGL 9.2.2 pinned by archive hash; font converter 1.5.3 pinned by lockfile.

Verification: Debug configure/build, portable model/lifecycle/fuel/shift tests and 24 deterministic render scenarios pass. Render tests reject runtime warnings. Representative layouts/themes/fuel/unavailable/warning/lifecycle captures inspected; native Mac mode/theme/speed, key-off/on and unavailable controls exercised. Board flash, UART codec, vehicle adapter and hardware validation remain pending. Settings, persistence, functional trips/laps and final icons are not implemented in this slice.

## Next actions

1. Review architecture, lifecycle exceptions and scope.
2. Confirm delivered board revision, matching schematic/example and vehicle MCU model.
3. Preview/tune the host UI; establish the board-specific panel smoke target after revision confirmation.
4. Refine components/artwork and add remaining navigation/screens with deterministic tests.
5. Define/test UART, then integrate synthetic bench data before measured bike inputs.

See [implementation plan](implementation-plan.md). No simulator or design result counts as bike verification.
