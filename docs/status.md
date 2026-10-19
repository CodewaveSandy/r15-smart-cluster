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

Architecture, resource/concurrency proposals, style guide, power lifecycle, implementation stages and verification gates are documented. Active documentation describes the project baseline. The first LVGL host slice is implemented: portable observations/quality, model validation, monotonic ignition lifecycle, shared palettes, Inter/Barlow subsets, dashboard layouts/themes and keyboard simulator. Default CMake and scripts/preview.sh build the native SDL2 preview. Host dependency: LVGL 9.2.2 pinned by archive hash; font converter 1.5.3 pinned by lockfile.

Verification: a fresh Debug configure/build completes without compiler warnings; portable model/lifecycle/fuel/shift tests and 44 deterministic render scenarios pass.  Typography bounds and required-glyph checks pass. Render tests reject runtime warnings. Representative layouts/themes/fuel/unavailable/warning/lifecycle captures inspected; native Mac mode/theme/speed, key-off/on and unavailable controls exercised. Board flash, UART codec, vehicle adapter and hardware validation remain pending. Settings, persistence, functional trips/laps are not implemented in this slice.

## Next actions

1. Review architecture, lifecycle exceptions and scope.
2. Confirm delivered board revision, matching schematic/example and vehicle MCU model.
3. Preview/tune the host UI; establish the board-specific panel smoke target after revision confirmation.
4. Refine components/artwork and add remaining navigation/screens with deterministic tests.
5. Define/test UART, then integrate synthetic bench data before measured bike inputs.

See [implementation plan](implementation-plan.md). No simulator or design result counts as bike verification.

## Typography implementation

Inter 4.1 Medium/SemiBold and pinned Barlow Semi Condensed SemiBold are bundled with their OFL licenses and hashes. Labels and large numerals use centralized role mappings; RPM text is 16 px. Font conversion is reproducible through the existing locked converter. Native Mac enlargement uses linear filtering. F2 provides a preview-only all-icons test; render verification includes both icon-test themes for 44 scenarios. Icon artwork uses the bundled MDI system described below.

## Dashboard presentation

The dashboard omits the footer and GEAR caption. Speed is horizontally centered in Street/Track; rendered digit bounds are tested for unknown/one/two/three-digit values. Trip and info lists cycle independently, with simulated TCS and display-only Track toggles. Fuel segments use minimal separation. The render suite covers all four list pages in both modes/themes (44 saved scenarios); source status is visible in the header subtitle and simulation remains identified by the host window/terminal.

## Track information hierarchy

Track mode uses the large centered 79 px lap readout and a secondary 46 px speed readout on the right. Street remains unchanged. The host provides a fixed lap sample; portable tests cover mm:ss.hh formatting, minute boundaries, stale values and display overflow. Both themes, warnings and unavailable views are rendered and inspected.

## Track best lap and shared spacing

Track lower-left is a fixed BEST LAP observation (host sample 01:18.42); Enter only cycles trips in Street. Unknown best lap displays dashes. Both modes share RPM height, lower divider and lower-panel/fuel positions. Render checks enforce shared vertical geometry and verify that Track best lap does not change when Street trip selection changes. Functional lap recording remains pending.

## Automotive icon assets

The 12 selected MDI 7.4.47 glyphs replace rough line paths and MSG/CALL/TCS/ABS text stand-ins. Typed icon widgets use local antialiased subsets; neutral and QS/VVA retain text. Source/license/hash records and regeneration script are included. All-icon checks now cover both themes/layouts, for 44 saved render scenarios overall; glyph coverage/bounds checks include every selected icon. OEM conformity is unverified.

## Optimization first pass — 2026-10-19

Host render metrics now report work for 100 identical settled updates. Redundant flushes fell from 1,200 callbacks / 38,400,000 pixels to zero, with the same 72 LVGL objects and unchanged host buffer sizes. All 44 baseline renders remain byte-identical. Stable-state regression checks include both layouts/themes, unknown values, warnings, blink phase and boot artwork. Firmware measurements remain pending. See [optimization sequence](optimization-plan.md) for remaining stages and boundaries.

## Reusable dashboard components — 2026-10-19

Dashboard composition now delegates to seven focused components with RAII object ownership, named tell-tales and centralized geometry. The LVGL object count remains 72; all 44 baseline renders are byte-identical and idle updates still flush zero pixels. Repeated creation/destruction tests check object-count restoration and underlying pixels. Font bounds checks use the layout definitions. Build and host tests pass; no hardware claims are added. Next: portable presentation decisions and semantic input.
