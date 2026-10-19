# Optimization implementation sequence

Started: 2026-10-19. Preserve the approved 800 × 480 appearance and single LVGL owner. Software improvements and board measurements are separate milestones. No firmware fit or performance claim follows from host evidence.

| Step | Scope | Status / acceptance |
| --- | --- | --- |
| 1 | Reproducible host baseline | Implemented: render hashes preserved locally, host flush counters/report and object count |
| 2 | Eliminate redundant widget mutations | First pass implemented: unchanged effective styles/visibility do not trigger setters; layout/font changes only on mode transition; immutable geometry set at construction. Formatting and broader hidden-widget scheduling remain for component/presenter work |
| 3 | Focused reusable components and geometry | Implemented: seven fixed-size components, named tell-tale roles, centralized Geometry.h, RAII ownership and repeated construction/destruction checks |
| 4 | Portable presentation and semantic input | Next: bounded formatting outputs and shared actions; preserve host-only simulated mutations |
| 5 | Font subsets and firmware configuration | Pending: role-specific glyph audit, compiled size comparison, compatible target-only LVGL feature selection |
| 6 | Panel buffering and resource budgets | Requires delivered PCB revision, matching schematic/example, working board target and measured driver ownership; compare supported partial/direct modes |
| 7 | Bounded UART and persistence | Requires reviewed versioned wire specification; latest-state mailbox, ordered input queue, freshness/session tests; settings writes outside rendering |

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

The 72-object tree and host buffers are unchanged. All 44 captured images remain byte-identical to the pre-extraction baseline; stable presentation still causes zero flushes. Eight repeated dashboard constructions, updates and destructions restore the previous object count and underlying pixels. These are host object-lifetime checks, not proof of target heap behavior or firmware fit. Presentation formatting and semantic input are the next stage.
