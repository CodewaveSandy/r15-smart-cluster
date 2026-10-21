# Companion work instructions

Follow repository RULES.md and AGENTS.md. Read docs/README.md, docs/architecture.md, docs/implementation-plan.md and the relevant protocol/journey/security guides here before changing this app.

Flutter/Dart shares application/UI logic; Kotlin/Swift own platform integrations. Keep domain types free of Flutter/plugins. Compose concrete dependencies explicitly; add abstractions only for actual responsibilities. No service locator, speculative packages, fake success, sensitive logs or empty feature folders.

Run format, analyze and meaningful tests with the pinned SDK. Build native targets when tooling is available; record missing evidence precisely. For UI changes inspect light/dark, small-screen/large-text and unavailable/permission states. Use real devices for background, notifications, radios and OS permission evidence. A Mac bridge is not ESP32 feasibility evidence. Keep signing credentials, SDKs, caches, screenshots and generated build outputs untracked.
