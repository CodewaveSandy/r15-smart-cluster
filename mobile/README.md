# R15 Companion

Flutter application in the R15 Smart Cluster monorepo. Android first, future iOS; native Kotlin/Swift project shells are generated. Independent of the C++17/LVGL/CMake build.

## Implemented foundation

- Official Android/iOS Flutter project shells and pinned Flutter version.
- Explicit composition root, immutable feature-status model and a read-only Phone screen.
- Light/dark system themes, scrollable text-scalable content and widget checks.
- No third-party runtime packages, requested phone permissions or fake pairing actions.

BLE/Wi-Fi, Mac bridge, location/maps, notifications, media and native background services are **not implemented**. This foundation does not connect to the cluster preview yet. Map rendering and end-to-end performance still need actual-board validation.

Start with [development](docs/development.md), then [architecture](docs/architecture.md), [implementation plan](docs/implementation-plan.md) and the [documentation index](docs/README.md).

```sh
cd mobile
flutter pub get
flutter run -d <android-device-id>
```

Use Flutter 3.47.7 (Dart 3.13.5), recorded in `.flutter-version`; setup, checks, native-toolchain limits and security requirements are documented in the development guide. Native app IDs `dev.r15cluster.r15_companion` (Android) and `dev.r15cluster.r15Companion` (iOS) are provisional development identifiers. Store identity/signing and release configuration remain pending. Android release signing is intentionally unconfigured; no distribution credentials are committed.
