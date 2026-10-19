# R15 Smart Cluster — Engineering Rules

## Priorities

Correctness and vehicle safety, clear ownership/layers, maintainability/testability, deterministic behavior, responsive readable UI, portability and measured performance. Prefer simple explicit code over framework-heavy abstractions.

## Architecture and stack

Vehicle ESP32 handles acquisition/calculations/connectivity; display ESP32 handles LVGL. UART carries validated normalized state and semantic actions, not pixels or raw vehicle fields exposed to widgets. Proposed tools are ESP-IDF/FreeRTOS, C++17, CMake and a shared-source host LVGL preview. Pin compatible dependencies after board validation. Do not silently change language standard or selected hardware direction.

Keep hardware adapters, transport codec, portable domain model, application/presenter and UI separate. Vehicle code cannot manipulate widgets; UI cannot parse CAN/UART or own storage. Instantiate only cohesive modules with real responsibilities. No generic service locators/event buses or deep inheritance without demonstrated need.

## State and validation

Explicit units, scoped enums, range/length/version/type validation, bounded buffers and explicit quality: available, unsupported, unavailable, invalid and stale. Never replace unknown speed/fuel/gear with zero/empty/neutral. Distinguish link freshness from source freshness. Reject malformed input atomically. Sequence/session handling must survive reboot/wrap/reconnect. Assertions do not replace runtime validation.

## Ownership, tasks and timing

Use RAII and clear allocation/lifetime strategies. Serialize LVGL access through its owner; callbacks never retain dangling UI objects. No blocking serial/storage/network work in rendering. Keep queues/tasks/stacks bounded; avoid per-update allocation and unnecessary notifications/redraws. Use monotonic clocks for durations, freshness, animation and laps; explicit clock quality for date/time. Introduce threads/tasks only when justified.

## UI

Follow docs/lvgl-style-guide.md and docs/power-lifecycle.md. Centralize geometry, colors, fonts, opacity and timing. Preserve non-touch navigation and warning visibility across screens. Critical truth never waits for decoration. Boot/shutdown artwork each lasts 2.00 s; total startup and retained power require measurement. Optimize based on profiling while preserving legibility at 800 x 480.

## Persistence and failure

Separate display preferences, session/simulation state and vehicle-owned persistence. Never seed a real odometer from demo data or overwrite it without a verified contract. Validate/version storage, coalesce writes, bound shutdown work and report failures. No reliance on final key-off writes alone. Bound logs and avoid private phone content. Stale data, reset, UART loss, queue overflow, panel failure and storage failure need explicit architectural behavior.

## Hardware

Never invent Yamaha protocols, connector pins, ECU commands, resistor values or calibration. Record measurement provenance and uncertainty. CAN requires a suitable physical interface; no generic optocouplers directly in CAN H/L. Confirm board pins, voltage levels and power topology before use. Early bike work is passive/read-only with OEM functionality preserved. Independent critical indications and failure behavior require review. Do not claim regulatory compliance without verification.

## Resources and dependencies

Measure internal SRAM, PSRAM bandwidth/capacity, task stacks, DMA needs, flash/image/partition fit and render timing. Do not assume capacity proves performance or OTA feasibility. Runtime-critical fonts/assets ship locally with licenses and reproducible conversion. Evaluate dependency necessity, footprint, licensing, maintenance and embedded support.

## Verification and delivery

Nontrivial behavior needs practical tests. Cover below/minimum/normal/maximum/above numeric boundaries and disconnected/valid/malformed/stale/reconnect/reset cases. Test simultaneous warnings, lifecycle interruption and rapid ignition changes. Use deterministic host clocks/scenarios and isolated storage. Build affected targets, run relevant tests and inspect runtime warnings. Significant UI changes require actual visual inspection, including both modes/themes and unavailable data. No source-only completion claims.

Maintain docs for behavior, architecture, workflow and decisions. Report designed, implemented, simulated, bench verified, hardware verified and bike verified as distinct states. Use modern target-oriented CMake and explicit dependencies. Keep generated artifacts untracked; respect user work and Git control. Preserve necessary backups before authorized removal/replacement.
