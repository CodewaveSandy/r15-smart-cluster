# Companion development and verification

## Toolchain

Pinned Flutter **3.47.7**, framework revision **abaf9c5237**, engine revision **deb287481e**, bundled Dart **3.13.5**. `.flutter-version` records the version for humans/version managers; it does not automatically select an SDK. Verify `flutter --version` before resolving dependencies. Use the official [SDK archive](https://docs.flutter.dev/install/archive) and [installation guide](https://docs.flutter.dev/install/manual). Commit `pubspec.lock`; do not upgrade transitives casually.

This scaffold was generated with `flutter create --platforms=android,ios --org dev.r15cluster --project-name r15_companion --no-pub mobile`. The generated Android project pins AGP 9.1.0, Kotlin 2.4.0 and Gradle wrapper distribution 9.3.1. Java compilation targets 17; validate the actual Gradle runtime JDK using doctor/Gradle requirements before native builds. Android SDK/NDK levels follow the pinned Flutter tooling until plugin selection establishes explicit OS support. Generated iOS deployment target is 15.0; future plugins may revise it. Generated launcher branding is provisional.

## One-command Android preview

Connect and unlock your phone, enable USB debugging and approve this computer. From the repository root:

```sh
bash scripts/preview-mobile.sh
```

The script checks Flutter against `.flutter-version`, detects a single authorized physical Android phone, restores the locked dependencies and runs the debug app. It works from any working directory when invoked by its path. Flutter and Python 3 must be on PATH. It does not start the Mac cluster preview or establish a phone-to-cluster connection.

With multiple phones, pass the exact device ID reported by `flutter devices`:

```sh
bash scripts/preview-mobile.sh <android-device-id>
```

No connected phone or ambiguous selection produces instructions instead of launching a desktop/web target. Emulators are excluded. During the session press `r` for hot reload, `R` for restart and `q` to quit. First builds may download Gradle/Android dependencies; network access is required for uncached dependencies. SDK installation, USB authorization and licenses remain explicit setup steps.

## Commands

Run from `mobile/` with the pinned SDK on PATH:

```sh
flutter --version
flutter doctor -v
flutter pub get

dart format --output=none --set-exit-if-changed lib test
flutter analyze --no-pub
flutter test --no-pub

flutter devices
flutter run -d <android-device-id>
flutter build apk --debug --no-pub
# Full Xcode and simulator tooling required:
flutter build ios --simulator --no-pub
```

Android: install compatible Android SDK, build/platform tools and JDK, accept licenses, enable USB debugging and authorize your device. iOS: full Xcode, selected developer directory and simulator/device setup; signing/team configuration is needed for physical devices. Install additional native package tooling only when the selected integration requires it. Release signing is intentionally absent. Android debug build/install/launch is confirmed by the user on a physical device; iOS remains unverified.

C++ preview remains independent: run `bash scripts/preview.sh` from repository root. No mobile SDK is required by its CMake build. Conversely Flutter tests do not build C++ or require SDL/ESP-IDF.

## Current verification: 2026-10-21

- Dependency resolution and lockfile generation succeeded with the pinned SDK.
- Static analysis: no issues.
- Eleven widget tests passed for branded startup, pending connection action, confirmed connection/loss, disabled unavailable navigation, timer disposal, and all four connection states at 320 x 568 with 2x text in light/dark themes.
- Four temporary 390 x 844 connected/disconnected light/dark captures were rendered with real text/icon fonts and visually inspected; content is readable and unclipped. Captures and their helper remain ignored under `build/`. These are host renders, not device screenshots.
- Updated startup/home Android debug APK built successfully with `flutter build apk --debug --no-pub`; the new UI has not yet been inspected on the physical phone.
- Android tooling installed: Flutter doctor reports a valid Android toolchain and accepted licenses. User confirmed successful debug build, installation and launch on Samsung SM-M346B running Android 16 (API 36). This verifies foundation startup only.
- iOS simulator build attempted: Flutter returned “Application not configured for iOS”; this Mac has no full Xcode and selects only CommandLineTools. The generated iOS shell is present, but native configuration/build must be verified after installing Xcode. No iOS native build/device claim.
- No BLE, Wi-Fi session, Mac bridge, map provider, ESP32 or bike test has run. No changes to C++ source; cluster visual regression is outside this foundation change.

Initial generation used a temporary SDK at `/private/tmp/r15-flutter-sdk`. The user subsequently installed the matching SDK durably at `/opt/homebrew/share/flutter`. Tool caches and generated machine-local paths are ignored. Template originals replaced during setup were preserved and checked under `/private/tmp/r15-mobile-template-backup`.

## Upcoming Mac/phone testing

Implement M1/M2 in the [plan](implementation-plan.md) first; running the current app does not connect it to the simulator. Then use an opt-in authenticated host adapter on a shared LAN, development bootstrap with expiring trust, validated capabilities and recorded/live location. Device inputs flow through normalized models under the LVGL owner. Test source staleness, network loss, reboot, flood/oversize, old-route data and command duplication with bounded simulated UART bandwidth.

Host rendering can prove geometry and behavior; it cannot establish board memory/render throughput, radio coexistence, UART capacity or safe vehicle operation. Real Android tests must cover background/locked screen/permission revocation and vendor battery restrictions. Future iOS tests use a separate capability matrix.

## Source and generated files

Commit Dart/native source, manifests, Gradle configuration, Xcode project, assets, lockfile and Flutter metadata. Ignore build outputs, SDK caches, IDE state, `local.properties`, generated Flutter platform configuration, signing files, private environment values and screenshots. Flutter can regenerate ignored Gradle wrapper launchers/artifacts during tooling setup; retain the distribution properties in source.
