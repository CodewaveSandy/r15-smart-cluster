# Companion work instructions

## App build and verification policy (2026-11-03)

Per the user, the Mac has no Android SDK and app APK builds belong in GitHub Actions after the code is pushed. Do not attempt local Android/Gradle/APK builds, `flutter run`, ad-hoc native compilation, or SDK/Android Studio installation as part of app development. Complete the code and dependency/workflow updates; formatting and static analysis may be used when already available without an Android SDK. The user installs the Actions APK with the existing ADB helper and performs manual device testing. Do not add/run automated suites. Git commit/push remains user-owned unless explicitly requested. This overrides earlier local-build guidance for app-side work.

Follow repository RULES.md and AGENTS.md. Read mobile/README.md, mobile/docs/architecture.md and mobile/docs/protocol.md; start background work from mobile/docs/background-sync.md. Journey, security, status and approved UI are consolidated in mobile/docs/architecture.md.

Flutter/Dart shares application/UI logic; Kotlin/Swift own platform integrations. Keep domain types free of Flutter/plugins. Compose concrete dependencies explicitly; add abstractions only for actual responsibilities. No service locator, speculative packages, fake success, sensitive logs or empty feature folders.

User discovery rule (2026-10-30): show only real BLE devices whose advertised names start with the exact, case-sensitive prefix `R15 Cluster`. Show all retained matches together inside the continuously animated radar, with adaptive icon sizes, two-line complete names and stable scattered positions. Positions are decorative, not radio-derived distance/bearing. Preserve explicit OS pairing approval; a matching name is never authentication. Firmware remains read-only unless the user explicitly authorizes a firmware change.

## Development-phase verification override (2026-10-31)

Per the user, all repository-owned automated test files have been removed for this development phase. Do not create, restore, expand or run automated test files/suites unless the user explicitly requests resuming that work. This applies to firmware, shared code, host preview and mobile. Defer automated unit/integration/golden tests until the later testing stage. Use affected builds, formatting/static analysis, runtime diagnostics, manual preview inspection and physical bench checks as appropriate; report exactly what was and was not verified. Preserve runtime validation, CRC/security checks, flash/backup/partition guards, development previews and normative protocol vectors. This override supersedes automated-test authoring/preservation gates elsewhere in project documents.

Per the user (2026-11-03), app UI targets modern/latest Android devices at normal text size. Do not perform small-screen or2× text-size checks unless explicitly requested. Inspect light/dark and unavailable/permission states on the current target-device layout. Use real devices for background, notifications, radios and OS permission evidence. A Mac bridge is not ESP32 feasibility evidence. Keep signing credentials, SDKs, caches, screenshots and generated build outputs untracked.
