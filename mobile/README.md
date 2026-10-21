# R15 Companion

Flutter application in the R15 Smart Cluster monorepo. Android first, future iOS; native Kotlin/Swift project shells are generated. Independent of the C++17/LVGL/CMake build.

## Implemented foundation

- Official Android/iOS Flutter project shells and pinned Flutter version.
- Explicit composition root, immutable feature-status model and a centered startup wordmark and connection-aware home screen.
- Light/dark system themes, scrollable text-scalable content, disconnected Connect cluster action and connected Navigation card.
- No third-party runtime packages, requested phone permissions or fake pairing actions.

An opt-in [nearby Mac preview connection](docs/mac-preview-connection.md) is implemented for development. ESP32 Wi-Fi provisioning, location/maps, notifications, media and native background services are **not implemented**. Use the bridge launch flags to connect to the cluster preview; the plain launch remains disconnected. Map rendering and end-to-end performance still need actual-board validation.

Start with [development](docs/development.md), then [architecture](docs/architecture.md), [implementation plan](docs/implementation-plan.md) and the [documentation index](docs/README.md).

Connect your Android phone with USB debugging enabled, then from the repository root:

```sh
bash scripts/preview-mobile.sh
```

It detects your phone and launches the app. If multiple phones are connected, pass a device ID. Press `r` for hot reload or `q` to quit. See [development](docs/development.md) for setup and manual commands.

Use Flutter 3.47.7 (Dart 3.13.5), recorded in `.flutter-version`; setup, checks, native-toolchain limits and security requirements are documented in the development guide. Native app IDs `dev.r15cluster.r15_companion` (Android) and `dev.r15cluster.r15Companion` (iOS) are provisional development identifiers. Store identity/signing and release configuration remain pending. Android release signing is intentionally unconfigured; no distribution credentials are committed.
