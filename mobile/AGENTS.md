# Companion work instructions

Follow repository RULES.md and AGENTS.md. Read docs/README.md, docs/architecture.md, docs/implementation-plan.md and the relevant protocol/journey/security guides here before changing this app.

Flutter/Dart shares application/UI logic; Kotlin/Swift own platform integrations. Keep domain types free of Flutter/plugins. Compose concrete dependencies explicitly; add abstractions only for actual responsibilities. No service locator, speculative packages, fake success, sensitive logs or empty feature folders.

User discovery rule (2026-10-30): show only real BLE devices whose advertised names start with the exact, case-sensitive prefix `R15 Cluster`. Show all retained matches together inside the continuously animated radar, with adaptive icon sizes, two-line complete names and stable scattered positions. Positions are decorative, not radio-derived distance/bearing. Preserve explicit OS pairing approval; a matching name is never authentication. Firmware remains read-only unless the user explicitly authorizes a firmware change.

User instruction (2026-10-30): do not write new automated test cases during the current implementation phase. Testing will be manual; defer new test cases until the user requests them later. Preserve existing tests. Use formatting/static analysis and affected native builds when tooling is available, then provide manual preview checks and report actual evidence/limitations. This overrides automated-test authoring gates in the mobile implementation/design documents for this phase.

For UI changes inspect light/dark, small-screen/large-text and unavailable/permission states. Use real devices for background, notifications, radios and OS permission evidence. A Mac bridge is not ESP32 feasibility evidence. Keep signing credentials, SDKs, caches, screenshots and generated build outputs untracked.
