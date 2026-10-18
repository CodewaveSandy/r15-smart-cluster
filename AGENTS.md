# R15 Smart Cluster — Agent Instructions

## Read before work

Read RULES.md, docs/README.md, docs/architecture.md, docs/embedded-architecture.md and the relevant style/lifecycle/implementation documents, then inspect affected source/tests. RULES.md is the engineering authority. User instructions take precedence except where they would require unsafe vehicle behavior or invented hardware facts.

## Product and boundaries

Two ESP32 MCUs communicate over UART. Vehicle MCU owns acquisition, validation, calculations, physical inputs and Wi-Fi/BLE. Display MCU owns application presentation and LVGL rendering at 800 x 480 on an Elecrow 7-inch HMI. C++17 is the application baseline; framework/library versions require compatible pinning. Exact PCB revision and vehicle MCU remain unconfirmed.

Normalized shared model/protocol code must not depend on LVGL, board drivers or RTOS. Widgets never decode vehicle packets or own persistence. All LVGL access has one explicit owner. Keep source, hardware, application and presentation responsibilities separate. Do not create empty abstractions.

## Current phase

The first LVGL host slice is implemented. Continue incremental implementation and verification under docs/implementation-plan.md. Hardware firmware requires board-specific validation. Preserve the specified UI; no unsolicited redesign. Boot and shutdown animations each last 1–1.5 s as defined in docs/power-lifecycle.md. Shutdown requires retained power; never claim a firmware-only guarantee. Full Maps/projection remains unresolved.

## Work procedure

1. Inspect rules, docs, affected implementation and tests.
2. Make the smallest coherent change, preserving unrelated user work.
3. Test behavior and boundaries; build affected targets when available.
4. For UI changes, inspect 800 x 480 representative states in both layouts/themes, warnings and unavailable data.
5. Check runtime diagnostics, ownership, bounded work and non-touch navigation.
6. Update relevant docs and report actual verification and limitations.

Do not invent pinouts, CAN IDs, bitrates, signal polarity, calibration or warning semantics. Initial real-bike integration is passive/read-only; active transmission requires explicit reviewed authorization. Keep OEM function during early observation. No claim of compliance or production readiness without evidence.

Routine Git operations remain with the user. Never reset/discard unrelated work, force push or rewrite history. Keep builds, generated exports, temporary screenshots and local IDE state untracked. Before removal/replacement requested in this project reset, preserve necessary source/assets/docs in a verified backup.
