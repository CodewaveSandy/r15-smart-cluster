# Optimization implementation sequence

Started: 2026-10-19. Preserve the approved 800 × 480 appearance and single LVGL owner. Software improvements and board measurements are separate milestones. No firmware fit or performance claim follows from host evidence.

| Step | Scope | Status / acceptance |
| --- | --- | --- |
| 1 | Reproducible host baseline | Implemented: render hashes preserved locally, host flush counters/report and object count |
| 2 | Eliminate redundant widget mutations | Host implementation complete: change-aware styles/layout/visibility, input-based numeric formatting, minute-based host clock formatting and deferred hidden instrumentation with immediate urgent-warning reveal |
| 3 | Focused reusable components and geometry | Implemented: seven fixed-size components, named tell-tale roles, centralized Geometry.h, RAII ownership and repeated construction/destruction checks |
| 4 | Portable presentation and semantic input | Implemented: bounded portable formatting, quality/lamp presentation, display-local actions and host keyboard adapter; simulator alone handles TCS requests |
| 5 | Font subsets and configuration/size audit | Host implementation complete: role-specific subsets, reusable LVGL 9.2.2 feature profile and asset/object-section audit. Board OS/allocator/buffer configuration requires a verified firmware target |
| 6 | Panel buffering and resource budgets | Hardware gate: resource arithmetic/reporting documented; actual allocation and partial/direct comparison need the delivered board revision, matching example and measured firmware |
| 7 | Bounded UART and persistence | Future subsystem gate: architecture is specified; there is no UART/settings implementation to optimize yet. Review the wire/command/storage contracts before implementing queues, codec and persistence |

## Baseline and first result

Measured on the Mac host on 2026-10-19, with 100 identical Street/Midnight updates at a fixed monotonic time after one settled render:

| Metric | Before | After first pass |
| --- | ---: | ---: |
| Flush callbacks | 1,200 | 0 |
| Pixels submitted to host flush | 38,400,000 | 0 |
| LVGL objects including screen | 72 | 72 |
| Host RGB565 framebuffer bytes | 768,000 | 768,000 |
| Host partial draw-buffer bytes | 64,000 | 64,000 |

All 44 deterministic BMP files match the pre-change SHA-256 hashes byte for byte. Existing model/lifecycle, typography and render tests pass. Stable scenarios additionally cover both layouts/themes, shift palette, unavailable observations, icon tests, low-fuel/turn phase and boot artwork. This tests unchanged presentation; it does not suppress real blink, warning or telemetry changes.

The main source of redundant redraw was repeated style application and repeated visibility operations. An unavailable subtitle also alternated between muted and amber within one update; it now receives only its final color. Layout changes preserve current widget objects. No telemetry snapshot cache or runtime allocation was introduced by the change.

## Repeat the measurement

Build normally, then run:

```sh
ctest --test-dir build/lvgl --output-on-failure
```

The render test writes `build/lvgl/previews/host-metrics.json` with flush calls, submitted pixels, object count, host buffers and elapsed host microseconds. Timing varies with host/load/build configuration and is diagnostic only; tests assert zero redundant flushes rather than a timing threshold. Rendering also checks unchanged pixels. Generated reports and baseline captures stay in ignored build output.

## Hardware evidence still required

Measure the actual firmware image/partition fit, asset sections, internal SRAM, PSRAM and largest free blocks, stack/queue high-water marks, render latency and scanout stability. Include full-screen shift flashing, UART traffic and storage writes under load. Host asset source-file sizes are not compiled firmware sizes. Do not allocate every possible buffer or assume OTA fits the reported 4 MB flash. Exact buffer placement and LVGL configuration follow the verified board port.

## Component extraction — 2026-10-19

`Dashboard` now composes `Header`, `TellTaleRow`, `Tachometer`, `PrimaryReadout`, `RideInfoPanel`, `FuelGauge` and `LifecycleOverlay`. Named layout structures in `embedded/ui/Geometry.h` define their positions and bounds, deriving shared dimensions from `Metrics`. Typography checks use those same bounds. `WidgetPrimitives.h` centralizes the small construction and change-aware mutation operations; it is not a widget framework or a portable domain dependency.

Components are value members, with no wrapper allocations, virtual dispatch, extra containers, timers or tasks. Each owns its top-level LVGL objects through `ObjectOwner` (a standard unique pointer with an LVGL deleter). LVGL owns each object's descendants. Borrowed descendant pointers stay private. The dashboard root is declared before components so reverse member destruction removes the components before their parent. Gradient descriptors outlive their referencing objects. The caller must keep the supplied parent alive until the component/dashboard is destroyed, and all creation/update/destruction stays on the LVGL owner thread.

The 72-object tree and host buffers are unchanged. All 44 captured images remain byte-identical to the pre-extraction baseline; stable presentation still causes zero flushes. Eight repeated dashboard constructions, updates and destructions restore the previous object count and underlying pixels. These are host object-lifetime checks, not proof of target heap behavior or firmware fit. Presentation formatting and semantic input were subsequently extracted in the stage below.

## Portable presentation and semantic input — 2026-10-19

`embedded/presentation/` contains bounded header/readout/ride-info formatting, lamp decisions and fuel/shift policy without LVGL, SDL or board dependencies. It consumes validated normalized observations and display-local options; numeric range validation remains at the state boundary. Non-available qualities produce unavailable text. Text outputs use fixed buffers with an unavailable fallback on formatting overflow; no dynamic string or telemetry-state cache is introduced. Palette colors, geometry and LVGL mutations remain in the components. `InstrumentPresentation.h` moved here; there is no duplicate legacy header.

`DisplayOptions` owns the portable display preferences/page state. `DashboardInput.h` applies NextTrip, NextInfo, ActivateInfo, ToggleLayout, ToggleTheme and ToggleUnits actions. `simulator/lvgl/DesktopInput.h` translates the existing keys. Demo speed/RPM, ignition, warning and icon-test controls stay host-only. Full settings up/down/select/back navigation remains pending until those screens exist.

ActivateInfo on TCS returns `RequestTcsToggle`; it never changes motorcycle observations. The host handles that effect only for an available simulated TCS value and suppresses it in unavailable mode. No vehicle command, UART packet or control authority is implemented. Hardware adapters can later feed the same semantic actions after validation.

Five host test suites now pass: model/lifecycle, portable presentation/input, desktop key mapping, typography and rendering. The portable test target does not link LVGL or SDL. Coverage includes quality states, units, gear/neutral, phone connectivity, bounded formatting, lamp decisions, independent page cycling, Track trip suppression/retention and typed action effects. All 44 baseline captures remain byte-identical, with 72 LVGL objects and zero unchanged-state flushes. This improves portability and testability; it is not a measured ESP32 CPU or flash saving.

## Host optimization closure — 2026-10-19

All currently applicable host optimization steps are implemented. This does not close the board or transport/storage feature milestones.

- Role-specific Barlow subsets remove unused glyphs without reducing 4 bpp quality. Font regeneration is byte-reproducible; full required-glyph checks include animation text and unavailable symbols.
- `embedded/ui/LvglFeatures.h` disables unused widgets, flex/grid, observer and unused themes. Existing default-theme styles remain; the default font reuses Inter 14 rather than compiling Montserrat. Host configuration includes this profile. Firmware must validate it against its pinned compatible stack and supply its own OS/allocator/display integration.
- Per-field input keys skip numeric formatting when values/quality/units/pages are unchanged. Unavailable payload changes do not trigger formatting. There is no second telemetry snapshot or extra text-buffer cache. On the Mac ABI the Dashboard wrapper increases from 648 to 840 bytes; the 192-byte increase trades bounded input-key storage for avoided formatting. Target sizes must be measured separately.
- An opaque lifecycle cover defers dashboard component work; reveal consumes the latest snapshot immediately. The root/overlay palette remains current. Valid urgent warnings bypass the cover in the same call. Host wall-clock formatting updates on minute changes.
- `scripts/audit-resources.py` reports glyph bitmap bytes and compiled host asset sections excluding debug data. It rejects missing build objects or unrecognized section output. Executable sections are diagnostic host measurements including host BSS, not ESP32 flash usage.

| Asset metric | Before role subsets | After |
| --- | ---: | ---: |
| Bundled glyph bitmap bytes (fonts + icons) | 90,391 | 75,833 |
| Compiled host asset sections excluding debug | 101,413 | 85,796 |

The 44 image files remain byte-identical to the pre-change baseline. Five test suites pass, including fresh-versus-cached rendering, availability/unit/page changes, hidden telemetry and urgent-warning reveal. Object count remains 72, and 100 identical settled updates still cause zero flushes. Tests preserve actual blink and warning transitions. Font conversion and compiled object sizes are measured separately.

## Remaining gates, with required evidence

See [resource budgets](resource-budgets.md). These are not unfinished host refactors:

1. Board port: delivered PCB revision, matching factory example/schematic and compatible pinned ESP-IDF/LVGL versions. Required before selecting panel pins, OS/allocator configuration, DMA/bounce buffers and VSYNC ownership.
2. Target measurements: actual image/partitions, SRAM/PSRAM and largest free blocks, stack/queue margins, scanout stability and end-to-end latency under full-screen shift flashing, UART and storage load. Required before buffering or OTA promises.
3. UART feature: reviewed bounded schemas/version/session/quality/freshness and command authority, followed by codec/queue/fault tests. Do not add unconsumed queues or speculate about Yamaha packets to call an optimization finished.
4. Persistence feature: reviewed ownership/schema/write policy and a real platform adapter, followed by power-loss/wear/failure tests. No vehicle odometer storage or simulated final key-off write is introduced here.

The future implementation remains ordered by docs/implementation-plan.md. Source/host success is not panel, bench or bike verification.
