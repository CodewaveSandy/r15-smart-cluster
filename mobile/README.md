# R15 Companion

Android navigation v2 is source-integrated against firmware `52bae3e`: independent live location/maps, private raw-vector cache, motorcycle search/routes, corridor downloads and c019–c01b display sharing. Read the [implementation, key setup and manual APK checklist](docs/navigation-v2-implementation.md). Android compilation/device acceptance remain pending; build only through GitHub Actions.

App workflow: finish code locally, push through the normal Git workflow, let GitHub Actions build the APK, then install with `bash scripts/install-mobile-apk.sh` and test manually. Target modern/latest Android devices at normal text size; skip small-screen/2× text checks unless requested. Do not attempt local Android builds or install an Android SDK/Studio; [app instructions](AGENTS.md#app-build-and-verification-policy-2026-11-03) preserve this user preference.

For map/location development, use the [phase 1 integration handoff](../docs/map-phase-1.md) and [proposed wire extension](docs/protocol.md#phase-1-map-extension--implementation-specification). Firmware and Android phase 1 map/location source are integrated under v1; both boards are user-reported flashed. Full paired APK/transfer/locked-screen acceptance remains pending.

Flutter application in the R15 Smart Cluster monorepo. Android first, future iOS; native Kotlin/Swift project shells are generated. Independent of the C++17/LVGL/CMake build.

## Current state

Android implements BLE discovery/pairing, a native foreground connection service/retry owner, Concept 01 My bike/Navigate tabs, authenticated phone name/battery/time metadata and strict c013–c015 discrete-input telemetry integration. Real Available gear feeds Home; speed/RPM/fuel/bike voltage remain unsupported and Navigate now uses pinned MapLibre/local Kolkata OSM with explicit foreground location sharing. The latest background/telemetry APK packaging and physical B1/B2 acceptance remain pending; Route guidance/search, automatic regional downloads, Wi-Fi provisioning, optional phone services and native iOS remain future work.

Start independent app work with [background-sync strategy and plan](docs/background-sync.md); firmware work starts at [the firmware plan](../docs/background-sync.md). Current contracts are consolidated in [protocol](docs/protocol.md), and the separate Mac development adapter is in [development](docs/development.md#mac-preview-adapter).

Start with [development](docs/development.md), then [architecture](docs/architecture.md#companion-architecture-status-and-approved-ui), [implementation plan](docs/architecture.md#current-status-and-roadmap) and the [documentation index](README.md#documentation).

For development on a Mac without Android Studio/Flutter, push changes to GitHub and run `bash scripts/install-mobile-apk.sh` to download the Actions APK and install over USB. See [cloud builds, Mac installation and signing](docs/development.md#build-on-github-install-from-the-mac) for one-time GitHub CLI setup.

For local Flutter builds/hot reload, connect your Android phone with USB debugging enabled, then from the repository root:

```sh
bash scripts/preview-mobile.sh
```

It detects your phone and launches the app. If multiple phones are connected, pass a device ID. Press `r` for hot reload or `q` to quit. See [development](docs/development.md) for setup and manual commands.

Use Flutter 3.47.7 (Dart 3.13.5), recorded in `.flutter-version`; setup, checks, native-toolchain limits and security requirements are documented in the development guide. Native app IDs `dev.r15cluster.r15_companion` (Android) and `dev.r15cluster.r15Companion` (iOS) are provisional development identifiers. Store identity/signing and release configuration remain pending. Android release signing is intentionally unconfigured; no distribution credentials are committed.

## Documentation

Four ongoing mobile guides:

| Guide | Responsibility |
| --- | --- |
| [Architecture, status and approved UI](docs/architecture.md#companion-architecture-status-and-approved-ui) | Ownership, journey/security, current roadmap and approved appearance |
| [Development and verification](docs/development.md) | Pinned tools, commands, physical evidence and Mac development adapter |
| [BLE protocol and contracts](docs/protocol.md) | Current name/battery/time payloads and future telemetry/navigation gates |
| [Background-sync plan](docs/background-sync.md) | Independent Android M1–M3 work, service configuration and shared acceptance |

Read [mobile instructions](AGENTS.md) and repository [RULES](../RULES.md). The [firmware background plan](../docs/background-sync.md) is the other developer's starting point. Automated suites remain deferred. Board recovery guides and asset licenses are intentionally separate; keep completed-work notes in the relevant authority rather than adding more guides.
