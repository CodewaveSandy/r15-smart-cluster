# R15 Smart Cluster — Agent Instructions

## Development-phase verification override (2026-10-31)

Per the user, all repository-owned automated test files have been removed for this development phase. Do not create, restore, expand or run automated test files/suites unless the user explicitly requests resuming that work. This applies to firmware, shared code, host preview and mobile. Defer automated unit/integration/golden tests until the later testing stage. Use affected builds, formatting/static analysis, runtime diagnostics, manual preview inspection and physical bench checks as appropriate; report exactly what was and was not verified. Preserve runtime validation, CRC/security checks, flash/backup/partition guards, development previews and normative protocol vectors. This override supersedes automated-test authoring/preservation gates elsewhere in project documents.


## Read before work

Read RULES.md, README.md, docs/architecture.md and docs/ui.md and the relevant development/background-sync guides, then inspect affected source. RULES.md is the engineering authority. User instructions take precedence except where they would require unsafe vehicle behavior or invented hardware facts.

## Product and boundaries

Two ESP32 MCUs communicate over UART. Vehicle MCU owns acquisition, validation, calculations, physical inputs and Wi-Fi/BLE. Display MCU owns application presentation and LVGL rendering at 800 x 480 on an Elecrow 7-inch HMI. C++17 is the application baseline; framework/library versions require compatible pinning. Delivered display PCB V3.0 is recorded; vehicle board identity/capacity still needs device verification.

Normalized shared model/protocol code must not depend on LVGL, board drivers or RTOS. Widgets never decode vehicle packets or own persistence. All LVGL access has one explicit owner. Keep source, hardware, application and presentation responsibilities separate. Do not create empty abstractions.

## Current phase

The host dashboard, lifecycle, Settings, Appearance preferences and Phone UI are implemented. Native display bench firmware and vehicle discrete-input USB firmware are implemented; discrete-input/return UART, authenticated BLE pairing/reconnect and connected phone name/battery/time sync are implemented and core behavior is user-confirmed. Background ride sync, full telemetry/navigation/map services, retained/network clock and real vehicle calculations remain pending; USB bench sync and persisted clock formats remain implemented. Vehicle owns acquisition/inputs/calculations/radios; display owns presentation/local Settings. Each board has a separate power branch. One-way input bench UART uses vehicle GPIO17 TX to display GPIO_D IO38 RX plus GND, preserving USB UART0; physical acceptance and final power hardware still require verification. Continue incremental implementation under docs/architecture.md and docs/background-sync.md; mobile work starts at mobile/docs/background-sync.md. Hardware firmware requires board-specific validation. Preserve the specified UI; no unsolicited redesign. Boot lasts 2.00 s and shutdown lasts 2.00 s as defined in docs/ui.md. Shutdown requires retained power; never claim a firmware-only guarantee. Full Maps/projection remains unresolved.

## Work procedure

1. Inspect rules, docs and affected implementation.
2. Make the smallest coherent change, preserving unrelated user work.
3. Manually check behavior and boundaries; build affected targets when available.
4. For UI changes, inspect 800 x 480 representative states in both layouts/themes, warnings and unavailable data.
5. Check runtime diagnostics, ownership, bounded work and non-touch navigation.
6. Update relevant docs and report actual verification and limitations.

Do not invent pinouts, CAN IDs, bitrates, signal polarity, calibration or warning semantics. Initial real-bike integration is passive/read-only; active transmission requires explicit reviewed authorization. Keep OEM function during early observation. No claim of compliance or production readiness without evidence.

Routine Git operations remain with the user. Never reset/discard unrelated work, force push or rewrite history. Keep builds, generated exports, temporary screenshots and local IDE state untracked. Before removing or replacing project files, preserve necessary source/assets/docs in a verified backup.

Prototype Settings access is unrestricted by speed, data quality, UART status or warnings per the user (2026-10-29); preserve visible warnings and lifecycle ownership. Gear color communicates UART freshness; time/date come from a real source. Authenticated phone metadata or the Mac USB helper supplies current time; never claim that saving a timestamp provides cold-power retention.
