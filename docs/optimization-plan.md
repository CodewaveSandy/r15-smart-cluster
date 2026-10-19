# Optimization implementation record

Current host scope is implemented. Board measurements and new UART/storage features remain separate gates. Preserve the approved 800 × 480 dashboard and single LVGL owner.

| Step | Implemented host behavior |
| --- | --- |
| Baseline and reports | Deterministic scenarios, flush counters/object count and asset/object-section audit |
| Change-aware updates | Apply only changed text/styles/visibility; layout on mode changes; numeric formatting on changed input; host clock formatting by minute |
| Components and ownership | Seven value components, RAII top-level LVGL ownership, private descendant handles, named geometry/tell-tales |
| Presentation and input | Portable bounded formatting/quality decisions; semantic display actions; keyboard and simulated vehicle mutation remain host-only |
| Assets and configuration | Role-specific font subsets, minimal LVGL feature profile and reproducible asset conversion |
| Hidden presentation | Defer covered instrumentation, reveal latest state immediately, bypass artwork for urgent warnings |

## Current evidence

Six test suites pass. Running/warning/unavailable/icon/page captures preserve the dashboard baseline; lifecycle captures use the selected R15 Light Signature. Repeated construction/destruction and fresh-versus-cached tests pass. The small input-key memory tradeoff avoids a second telemetry/text cache.

| Metric | Earlier baseline | Current |
| --- | ---: | ---: |
| Flush callbacks for 100 identical settled updates | 1,200 | 0 |
| Pixels submitted in those updates | 38,400,000 | 0 |
| LVGL objects including screen | 72 | 69 |
| Animation bitmap payload | 344,064 bytes (superseded neon art) | 12,288 bytes (A8 wordmark) |
| Current combined font/icon/animation bitmap payload | — | 102,440 bytes |
| Current host asset sections excluding debug | — | 112,544 bytes |

The superseded neon textures, emblem/ray/flash/zoom renderer, old timeline/test and unused animation-title glyphs were removed after backup when the user selected concept 02. The current small white wordmark and procedural slashes are the only active animation assets/path. [Design decisions](decisions.md) preserve the superseded architectural choices explicitly.

## Repeatable checks

```sh
cmake --build build/lvgl --parallel 4
ctest --test-dir build/lvgl --output-on-failure
python3 scripts/audit-resources.py --build-dir build/lvgl --output build/lvgl/previews/resource-audit.json
```

Reports/captures stay in ignored build output. Host timing and ABI section sizes are diagnostic, not target flash/RAM or frame-rate evidence. See [resource accounting](resource-budgets.md) for memory/buffer arithmetic and required measurements.

## Remaining gates

1. Confirm delivered board revision, schematic/factory example and compatible toolchain before selecting panel pins, allocator/RTOS configuration, DMA/bounce buffers and VSYNC ownership.
2. Measure actual firmware/partitions, internal SRAM/PSRAM, largest blocks, stacks, scanout and warning latency under load before promising buffer strategy, OTA fit or panel performance.
3. Review the UART schemas/version/session/freshness and command authority, then implement bounded codec/mailbox/input handling and fault tests. There is no existing transport implementation to optimize; do not introduce unused queues.
4. Review persistence ownership/schema/write policy and implement its platform adapter with power-loss/wear/failure tests. Demo odometer/session data must never become vehicle persistence.

Follow [implementation stages](implementation-plan.md). No host result is panel, bench or bike verification.
