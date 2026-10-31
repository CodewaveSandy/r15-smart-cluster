# Companion work instructions

Follow repository RULES.md and AGENTS.md. Read docs/README.md, docs/architecture.md, docs/implementation-plan.md and the relevant protocol/journey/security guides here before changing this app.

Flutter/Dart shares application/UI logic; Kotlin/Swift own platform integrations. Keep domain types free of Flutter/plugins. Compose concrete dependencies explicitly; add abstractions only for actual responsibilities. No service locator, speculative packages, fake success, sensitive logs or empty feature folders.

User discovery rule (2026-10-30): show only real BLE devices whose advertised names start with the exact, case-sensitive prefix `R15 Cluster`. Show all retained matches together inside the continuously animated radar, with adaptive icon sizes, two-line complete names and stable scattered positions. Positions are decorative, not radio-derived distance/bearing. Preserve explicit OS pairing approval; a matching name is never authentication. Firmware remains read-only unless the user explicitly authorizes a firmware change.

## Development-phase verification override (2026-10-31)

Per the user, all repository-owned automated test files have been removed for this development phase. Do not create, restore, expand or run automated test files/suites unless the user explicitly requests resuming that work. This applies to firmware, shared code, host preview and mobile. Defer automated unit/integration/golden tests until the later testing stage. Use affected builds, formatting/static analysis, runtime diagnostics, manual preview inspection and physical bench checks as appropriate; report exactly what was and was not verified. Preserve runtime validation, CRC/security checks, flash/backup/partition guards, development previews and normative protocol vectors. This override supersedes automated-test authoring/preservation gates elsewhere in project documents.

For UI changes inspect light/dark, small-screen/large-text and unavailable/permission states. Use real devices for background, notifications, radios and OS permission evidence. A Mac bridge is not ESP32 feasibility evidence. Keep signing credentials, SDKs, caches, screenshots and generated build outputs untracked.
