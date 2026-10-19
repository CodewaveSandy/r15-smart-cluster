# Implementation stages and verification gates

Updated: 2026-10-19. Status: host dashboard and host optimization pass complete; remaining screens and hardware gates pending.

## Sequence

| Stage | Deliverable | Exit evidence |
| --- | --- | --- |
| E0 — Design | Architecture, style guide, responsibility map and unresolved decisions | User reviews plan; exact board identity, vehicle MCU and Maps scope tracked |
| E1 — Reproducible baseline | Define deterministic UI scenarios; vendor board example; pin toolchain/dependencies | Host test baseline established; known vendor example builds; schematic/revision and flash/PSRAM/ports match actual board |
| E2 — Shared state and host LVGL | Portable quality/state/input model, LVGL host window and tokens | Same LVGL sources build on host; state/format/navigation tests; no LVGL/IDF dependencies in shared model |
| E3 — UI implementation | Street/Track, both themes, instruments, warnings, menus and lifecycle animations | Deterministic 800 x 480 captures against specified geometry; visual review; behavior boundaries pass |
| E4 — Display firmware | Minimal board port, fonts, settings, demo source and diagnostics | Firmware fits actual flash; RGB test and demo UI run on panel; measured memory/render/latency; no artifact/leak growth |
| E5 — UART bench | Reviewed protocol specification, codec, synthetic sender, display receiver | Golden vectors; truncated/oversized/corrupt/version/reset/overflow tests; cable loss, restart and stale signal behavior |
| E6 — Vehicle MCU | Verified input adapters/calculations and bounded Wi-Fi/BLE services | Bench sources match observations; radio load cannot starve acquisition; semantic input and command tests |
| E7 — Passive bike observation | Reviewed electrical interface and measured mappings | OEM retained; listen-only verification; compare logged observations to independent references; no speculative decode |
| E8 — Replacement evaluation | Power/boot/reset/fault, independent tell-tales and mechanical/environmental assessment | Explicit reviewed evidence before replacement; software parity alone is insufficient |

E2/E3 host work can proceed before delivery. E4 compile preparation can overlap, but board bring-up is a prerequisite to claiming display readiness. E5 can begin with synthetic UART data without Yamaha decoding. Full Maps/projection is a separate scope/feasibility gate; it is not hidden inside E6 BLE work.

## Current implementation and next gates

The host baseline, dashboard, portable presentation/input, component extraction and applicable host optimizations are implemented. See [current status](status.md), [optimization record](optimization-plan.md) and [resource accounting](resource-budgets.md).

1. Confirm delivered board revision, schematic/factory example, UART/programming conflicts and vehicle MCU.
2. Reproduce the matching vendor example and establish a minimal panel target with compatible pinned dependencies.
3. Measure actual flash/partition fit, internal SRAM/PSRAM, driver buffers, timing and scanout behavior before claiming display readiness.
4. Complete remaining screens/navigation and any further artwork tuning without changing the approved dashboard baseline unintentionally.
5. Review the versioned UART contract and persistence ownership, then implement and fault-test their real adapters.

No empty firmware scaffolding, unused queues or speculative board pins substitute for these gates.

## Test strategy

- Portable unit tests: numeric/quality validation, stale deadlines, unit conversion, formatting, calculations, action routing, schema migration.
- Codec tests: all split points, multiple frames/read, garbage and boot logs, max lengths, CRC corruption, missing fields, versions/enums/ranges, sequence wrap, duplicate/out-of-order frames, reboot/session and source age.
- Host LVGL tests: important widget state and navigation, deterministic render scenarios at 800 x 480. Use the specified behavior and meaningful boundary cases.
- Target tests: RGB color bars, rotation/stride, PSRAM, panel timings, flush completion, fonts, settings writes, UART stress, brownout/restart simulation and independent MCU boot order.
- Load/soak: worst-case RPM/fuel/turn/shift updates with menus and settings writes; vehicle radio load; log frame times, latency, heap largest block, stack margins, queue high-water marks and loss counters.
- Vehicle validation: passive only initially; track every signal's evidence and compare actual readouts. Bench and host success never count as bike verification.

Performance objectives are provisional in embedded-architecture.md. Establish representative runs and measured budgets before optimizing or promising frame rate/startup time. Do not gate correctness on internet services or a phone connection.

## Open decisions and missing evidence

| Item | Needed before |
| --- | --- |
| Elecrow product-family URL supplied; delivered PCB revision, schematic and matching factory archive still needed | Board pin/timing/UART selection |
| Vehicle ESP32 model and available peripherals | CAN/input/radio task implementation |
| User's measured harness inventory | Any bike adapter; record connector view, pin numbering, conditions, polarity, range and uncertainty |
| CAN physical/protocol confirmation | Controller/transceiver choice; no guessed IDs/bitrate |
| Full Maps/projection retained as unresolved by user; receiver feasibility and call/audio path still needed | Promising final feature coverage |
| Physical control source and event semantics | Hardware navigation; touch remains optional |
| Real versus calculated odometer, retention and calibration | Persistent vehicle values |
| Clock source, retention and disconnected behavior | Hardware clock editor/restore action |
| Warning unknown/failure behavior and independent indicators | Replacement evaluation |
| Image size/partition fit and update strategy | OTA or dual-image claims |

## Current evidence

Portable state/lifecycle/presentation tests and the LVGL 9.2.2 desktop dashboard now build and pass. Both layouts/themes, fuel/shift states and two-second R15 Light Signature are implemented. The render test produces 86 deterministic scenarios without LVGL warnings; native Mac controls were exercised. See getting-started.md. No flashed firmware, serial implementation, circuit validation or bike verification is claimed. E1 hardware evidence remains pending; E2 is implemented for the host slice and E3 is partial (menus/ride features pending).

Include [power lifecycle](power-lifecycle.md) tests in E2–E4: injected-clock duration checks, missing startup data, urgent warning interruption, key-off/key-on cancellation, animation failures, actual panel startup measurement and retained-power shutdown tests.
