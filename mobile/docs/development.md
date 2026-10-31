# Companion development and verification

## Android startup access and ESP32 discovery (2026-10-30)

Current radar rule: only names starting exactly with `R15 Cluster` are displayed (case-sensitive, no UUID/name exceptions). All retained real matches appear together inside a single radar, with adaptive sizes and stable decorative scatter; no paging or distance inference. Long names use complete fitted two-line labels plus accessible full names/tooltips. Pinch to zoom when dense. Scan windows run for five seconds, with a three-second rest before each repeat to avoid rapid scanner restarts; the radar keeps moving across these windows, except in reduced motion. Latest results arrive at the end of each window, not per advertisement. Stop/selection/background/disposal cancels scheduled and active scans, and late results are ignored. Errors stop scanning and expose retry. Devices absent from two successful rounds expire. Retained display is bounded to 32; native recent-selection validation retains at most 64 addresses for 20 seconds without logging them.

Manual checks: verify R15 Cluster and R15 Cluster-prefixed names appear while other names are hidden; watch multiple scan rounds, device arrival/disappearance and Cancel; inspect one/multiple/dense matches, two-line names, zoom and tap-to-pair. Verify background/resume and denied Bluetooth access stop/restart appropriately. These checks remain manual; build evidence does not prove physical radio behavior.

Physical ESP32 connection (2026-10-31): select the real radar result after opening Settings > Phone > Pair phone on the cluster. One native GATT owner immediately discovers the enrollment-name service and writes a printable ASCII phone label with response before requesting a new bond. It uses the local Bluetooth name, then genuine manufacturer/model, then explicit `Android phone` fallback, capped at 32 bytes. Missing/refused/late metadata does not abort OS pairing. Android owns numeric comparison; compare all six digits and approve both devices. The operation is bounded to 45 seconds. Success retains the same GATT until actual link loss, Disconnect or owner disposal.

Android persists only the successful selected address/name lookup hint; bond keys remain in Android. Foreground launch/resume rechecks OS bond state and reconnects to that peer. Automatic recovery has four attempts with 1/2/4/8-second delays. Remote-termination/authentication statuses (19/5/137) stop automatic retries because cluster Forget or refused trust may be involved; these statuses are not a definitive diagnosis. Explicit Retry remains available. Background cancels retry scheduling and backgrounded saved-connect attempts; deliberate first pairing survives the OS approval dialog. Background/process-death reconnect is not implemented. Disconnect cancels retries and retains bonds. Forget app selection clears only this app's hint; its confirmation directs the rider to Android Bluetooth settings and cluster Forget to remove both copies of trust. No hidden bond-removal API or remote Forget packet exists.

Home distinguishes the Bluetooth link from application readiness. No encryption/application authorization is inferred from metadata or the OS bond. Navigation, notifications and music remain pending. The opt-in Mac preview uses its separate existing session; use a plain launch for physical ESP32 integration.

Current verification: formatter and static analysis passed; arm64 Android debug APK built, installed and launched on Samsung SM-M346B / Android 16. Landing and radar were inspected in the phone's current dark appearance; radar found one real R15 Cluster without a service UUID advertising filter. Core physical acceptance passed in this session: user confirmed the correct name, matching-code approval and cluster connected status; native diagnostics confirmed GATT status 0, name-write status 0 and BONDING (11) then BONDED (12). The same link stayed live afterward. After the user restarted both boards, status 8 / disconnected was followed by status 0 / connected without another bonding transition. Explicit Disconnect retained the paired selection without automatic retry; Connect restored the link. Force-stop/relaunch also reconnected automatically without another bond transition. These observations establish the core milestone on this phone/firmware run, not extended reliability. Dark/light Home and light Home at 2x system text were inspected on the physical phone, including scrolling to Disconnect/Forget controls; original dark mode and 1x text were restored. Reduced-motion and the remaining fault matrix are pending. No automated test files were created or run; no firmware was modified. Follow [the handoff checklist](esp32-connection-handoff.md#manual-acceptance-before-calling-this-complete), recording observations without codes/keys/private identifiers. Build evidence does not complete the hardware milestone.

The app checks BLE permission/radio readiness after the brief branding screen and before showing the main app. Missing permissions are requested on each launch when requestable. Denial leaves an explicit retry/Settings screen; returning from Settings or resuming the app rechecks OS state. Modern Android requests Nearby devices only. Older supported Android needs Location permission/services for scanning; this is not navigation GPS tracking. No extra runtime permission is needed for Wi-Fi status. Native iOS discovery/permission setup is still pending.

The native scanner already used BLE; its old advertised-service filter matched only the Mac preview. The current ESP32 firmware advertises R15 Cluster without that UUID. Discovery now performs a five-second BLE advertisement scan, deduplicates up to 32 results and prioritizes R15-name/preview-service candidates. Unnamed devices remain Unnamed BLE device. Classic Bluetooth discovery is not used. Advertisements may be suppressed by Android's declared neverForLocation policy; this is not a promise to list every radio in range.

Manual verification:

1. Run `bash scripts/preview-mobile.sh` and allow Nearby devices/enabling Bluetooth before the landing opens.
2. Deny/revoke access in Android Settings, reopen the app and check retry or Open app settings; restore access and return. No automatic grant or repeated dialog loop is expected.
3. Power the ESP32, leave it advertising (disconnect nRF Connect if it is occupying the single BLE connection), and tap Connect cluster. Expect an R15 Cluster candidate without requiring the Mac service UUID. Also inspect unnamed/unrelated BLE results and empty/error/retry/Cancel behavior.
4. Tap the radar device icon to initiate Android Bluetooth bonding. Firmware bonding and companion application services remain separate; this does not implement ESP32 app data, telemetry or maps. Existing Mac-service results retain the identity/code/approval flow when the preview is configured.
5. Verify the permission screen in light/dark, large text, Bluetooth off, and after returning from Settings. Check saved preview reconnect happens after readiness. iOS requires separate future implementation/testing.

Static analysis/native build are software evidence; the above phone/radio checks remain manual and pending until the user reports results. No new automated tests are authored or run in this phase.

## Toolchain

Pinned Flutter **3.47.7**, framework revision **abaf9c5237**, engine revision **deb287481e**, bundled Dart **3.13.5**. `.flutter-version` records the version for humans/version managers; it does not automatically select an SDK. Verify `flutter --version` before resolving dependencies. Use the official [SDK archive](https://docs.flutter.dev/install/archive) and [installation guide](https://docs.flutter.dev/install/manual). Commit `pubspec.lock`; do not upgrade transitives casually.

This scaffold was generated with `flutter create --platforms=android,ios --org dev.r15cluster --project-name r15_companion --no-pub mobile`. The generated Android project pins AGP 9.1.0, Kotlin 2.4.0 and Gradle wrapper distribution 9.3.1. Java compilation targets 17; validate the actual Gradle runtime JDK using doctor/Gradle requirements before native builds. Android SDK/NDK levels follow the pinned Flutter tooling until plugin selection establishes explicit OS support. Generated iOS deployment target is 15.0; future plugins may revise it. Generated launcher branding is provisional.

## One-command Android preview

Connect and unlock your phone, enable USB debugging and approve this computer. From the repository root:

```sh
bash scripts/preview-mobile.sh
```

The script checks Flutter against `.flutter-version`, detects a single authorized physical Android phone, restores the locked dependencies and runs the debug app. It works from any working directory when invoked by its path. Flutter and Python 3 must be on PATH. For the opt-in phone-to-cluster Wi-Fi connection, use the two commands in [Mac preview connection](mac-preview-connection.md). The plain command does not start or connect the cluster preview.

With multiple phones, pass the exact device ID reported by `flutter devices`:

```sh
bash scripts/preview-mobile.sh <android-device-id>
```

No connected phone or ambiguous selection produces instructions instead of launching a desktop/web target. Emulators are excluded. During the session press `r` for hot reload, `R` for restart and `q` to quit. First builds may download Gradle/Android dependencies; network access is required for uncached dependencies. SDK installation, USB authorization and licenses remain explicit setup steps.

## Commands

### Current opening preview (2026-10-30)

On the configured Windows laptop, use Git Bash from the repository root: `bash scripts/preview-mobile.sh`. Flutter 3.47.7 is at `E:\learning\flutter`; Android SDK is at `E:\learning\AndroidStudioSDK`, with NDK 28.2.13676358 and Platform 36 installed. Git Bash startup exports the required tool paths. The user has confirmed the previous app preview installed and launched on this laptop.

The first Concept 01 slice adds the illustrated opening and blue theme. After launching, use **R** (hot restart) to see the 900 ms opening again. Check the bike/halo and typography in dark and light system appearance, increase Android font size and verify scroll access, then enable Remove animations and check the still opening. After branding, verify the existing disconnected/connected Home and pairing behavior. No telemetry/map services were added. Phone review and measured profile-mode performance for this new slice remain pending.

Per the user's current instruction, do not add automated test cases yet; defer test-file re-creation and use manual validation, format/static analysis and native builds during implementation.

First-connection follow-up: after the opening, an unpaired app shows the approved bike landing. Tap Connect cluster to request Android Nearby access and start actual discovery. Check radar/empty results, denied permission, Bluetooth off, Cancel/Back, large text, daylight and Remove animations. The reference's Bluetooth-status row is intentionally absent because the UI has no independent confirmed adapter-state observation. Discovery accepts the exact R15 Cluster name prefix without requiring a service UUID in advertising. Plain launch now performs physical BLE pairing and saved reconnect; application services still require a separate future contract. Mac preview pairing remains an opt-in development path. No device results or connection success are fabricated. Existing UI tests have not been updated or rerun for the new screen labels during this manual-testing phase.

Run from `mobile/` with the pinned SDK on PATH:

```sh
flutter --version
flutter doctor -v
flutter pub get

dart format --output=none --set-exit-if-changed lib test
flutter analyze --no-pub

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
- No physical phone/LAN bridge pairing, BLE, map provider, ESP32 or bike test has run. Host TLS/IPC and real Dart TLS integration are checked separately in the bridge guide. No changes to C++ source; cluster visual regression is outside this foundation change.

Initial generation used a temporary SDK at `/private/tmp/r15-flutter-sdk`. The user subsequently installed the matching SDK durably at `/opt/homebrew/share/flutter`. Tool caches and generated machine-local paths are ignored. Template originals replaced during setup were preserved and checked under `/private/tmp/r15-mobile-template-backup`.

## Upcoming Mac/phone testing

The first M2 development slice is implemented; run both previews with the [Wi-Fi bridge flags](mac-preview-connection.md). Running without those flags does not connect the app to the simulator. Then use an opt-in authenticated host adapter on a shared LAN, development bootstrap with expiring trust, validated capabilities and recorded/live location. Device inputs flow through normalized models under the LVGL owner. Test source staleness, network loss, reboot, flood/oversize, old-route data and command duplication with bounded simulated UART bandwidth.

Host rendering can prove geometry and behavior; it cannot establish board memory/render throughput, radio coexistence, UART capacity or safe vehicle operation. Real Android tests must cover background/locked screen/permission revocation and vendor battery restrictions. Future iOS tests use a separate capability matrix.

## Source and generated files

Commit Dart/native source, manifests, Gradle configuration, Xcode project, assets, lockfile and Flutter metadata. Ignore build outputs, SDK caches, IDE state, `local.properties`, generated Flutter platform configuration, signing files, private environment values and screenshots. Flutter can regenerate ignored Gradle wrapper launchers/artifacts during tooling setup; retain the distribution properties in source.

## Wi-Fi connection slice verification

See [connection verification](mac-preview-connection.md#verification) for the current session/TLS/UI test evidence and exact two-terminal launch commands. Earlier automated-suite evidence is historical; those test files/runners were removed on 2026-10-31. Use manual verification and affected builds for current changes. The new physical BLE discovery/approval/restart journey remains to be tried by the user; the previous Wi-Fi-only connection worked.

Current discovery verification: native Mac BLE advertising reported active; a temporary-item Keychain round trip passed; Android BLE/Keystore native code compiled. Cluster render coverage is now 210 scenarios with three Settings categories and unified Phone. See the connection guide for the new search/select/approve/Pair workflow and saved restart behavior.

### BLE bench acceptance record (2026-10-31)

App: this uncommitted mobile workstream on top of repository revision `a7c8ab7`; Samsung SM-M346B, Android 16. Firmware: user-supplied current handoff images on both boards, exact flashed hashes not independently collected on this laptop. nRF Connect is not used for the companion run. Previous stale pairing initially produced an honest rejected/bond-unavailable result; the user cleared the old pairing before the successful first-pair run. No code/key/private device identifiers are retained here. Native `R15BleLink` diagnostics emit only numeric GATT state/status, bond state and label-write result, never the label, address or code.

Passed: real radar discovery; phone-name ATT write callback status 0; user-confirmed correct display label, actual matching codes and cluster confirmation; persistent app link after OS bond; foreground automatic recovery after both boards restart; manual Disconnect/Connect retaining the bond; app process force-stop/relaunch automatic saved-peer reconnect. Native connected callbacks have GATT status 0, BONDING=11 and BONDED=12 on first pair only. This proves the reported run, not a negotiated application session or an independent app-side encryption proof.

Still pending: long/non-ASCII/missing labels, deliberately rejected/timed-out/closed-window enrollment, sole-slot contention, cluster Forget while this app is connected, Bluetooth/permission revocation, rapid lifecycle/selection races, the remaining small-screen/radar-large-text/reduced-motion matrix and prolonged stability. No automated suites were authored or run. Mac preview runtime was preserved in code but not exercised during this physical run.

## Concept 01 Home and bottom navigation (2026-10-31)

The saved-cluster Home now follows the approved My bike reference: heading, observed connection pill, bundled transparent side-profile illustration, four quality/source/unit-bearing telemetry tiles and Where to next entry. The bottom icon tabs switch My bike and Navigate. The settings icon/status pill opens Cluster connection; Back returns to the previous tab. Connection and trust owners remain above the shell, so page changes do not connect/disconnect or own keys. First-time landing/radar remain the existing flow; a saved peer remains on Home during link loss. Cluster connection reuses explicit Connect/Disconnect and confirmed Forget-app-selection behavior.

The four readings are honestly unavailable in the normal app; no sample 0/N/64%/12.6 V was copied from the board. Actual vehicle telemetry and map/search/navigation services are not implemented. Navigate is a pending-service page. This change adds no radio protocol or firmware edits, packages, permissions, background service or automated test files.

Software checks: pinned Flutter formatter/static analysis and arm64 Android debug build passed; app installed/launched on Samsung SM-M346B / Android 16. The earlier drawer iteration was phone-inspected for dark connected Home, drawer selection to Navigate, Back to Home, light cluster controls, explicit Disconnect returning to saved Home with disconnected status, and light Home at temporary 320 x 568 logical display / 2x system text. The metric grid becomes one column and content scrolls. Native diagnostics showed a successful saved GATT reconnect on launch; no page-switch disconnect or Flutter/Android error appeared in the inspected diagnostics. Small-display override, font scale and night mode are restored after preview. Captures/backups stay ignored under mobile/build.

Remaining verification: exhaustive denied/revoked permissions, pairing rejection/timeouts, rapid lifecycle changes, screen reader navigation, reduced-motion settings and physical iOS runtime; real live/stale telemetry cannot be accepted before its contract/producer exists. No performance/frame-rate guarantee or real route functionality is claimed. The Mac preview composition was preserved but was not physically run in this Home inspection.

Bottom-navigation correction (2026-10-31): the approved concept's My bike/Navigate icon-and-label tabs replace the drawer, with a blue selected underline. The settings shortcut/status pill opens Cluster connection; Back returns to the previous tab. Format, static analysis and arm64 debug build passed. Initial install attempts were blocked by Android USB authorization; after the user accepted the fresh prompt, the final APK installed and launched successfully. Actual phone inspection confirmed My bike/Navigate bottom-tab switching and the blue selected icon/label/underline, with the saved BLE link connected. Opening connection settings from Navigate and pressing Back returned to Navigate; the app was then left on My bike. No new Flutter/Android runtime error or BLE disconnect appeared during this focused run. Prior Home light/dark/small-screen/large-text observations above apply to the earlier Home iteration, not independent verification of the corrected bar. Follow-up verification is scoped to the changed navigation; the full accessibility matrix is not repeated for every minor edit.

## Connected phone metadata integration (2026-10-31)

Implemented the app side of [the connected metadata handoff](phone-connected-metadata-handoff.md) and preserved [enrollment names](phone-name-integration.md). `PhoneMetadataSource.kt` owns actual phone battery/label/UTC/offset sampling and OS battery/time/date/timezone receivers. `BleBond.kt` retains sole GATT ownership with explicit discovery/enrollment/MTU/metadata operations and callback deadlines. Metadata is best-effort and never converts unsupported/failed metadata into link failure or deletes trust. Actual permission revocation still cancels Bluetooth work. Native resume refreshes snapshots, and the existing app-wide connection lifecycle is preserved across bottom tabs.

The 44-byte full snapshot is generated immediately before each dispatch, validated locally and never persisted or logged. Fresh writes are spaced at least 1.1 seconds apart, normally every 30 seconds, with coalesced battery/time/resume events. A temporary write refusal has three bounded retries; subsequent recovery waits for normal refresh/events. GATT callback timeout disables optional work for that connection to avoid overlap; reconnect reconstructs everything from current observations. A successful write's app-side sync status expires at 90 seconds without a fresh success. Metadata is not vehicle telemetry: the Home bike-battery field remains unavailable and no navigation capability is enabled.

Verification: pinned formatter, `flutter analyze --no-pub` (no issues) and arm64 debug APK build passed. Installed/launched on Samsung SM-M346B / Android 16. Native results on the saved peer: successful connected callback (GATT 0), MTU 256 / status 0, connected metadata write status 0. The user confirmed the cluster showed the correct actual phone name, battery percentage and local time/date using the updated firmware on both boards. Exact flashed firmware hashes were not collected on this laptop. After reconnecting USB, native diagnostics confirmed repeated successful metadata writes approximately every 30 seconds over four minutes, plus event-triggered writes separated by about 1.17 seconds. No Flutter/Android error appeared in the inspected diagnostics. Explicit Disconnect cleared the sync message while retaining the bond and stayed disconnected. Connect then produced a new connected callback, MTU 256/status 0 and metadata status 0. App force-stop/relaunch also reconnected and repeated MTU 256/status 0 and metadata status 0 without a bond transition or new code. The connection sync message was visually inspected on the normal dark phone layout, and the app was left connected on Home. This narrow status-text addition did not repeat the earlier full large-font inspection.

Pending: explicit first pair on this revised ordering; updated-metadata board-restart delivery checks; old firmware/missing endpoint and low/failed MTU; delayed security/ATT refusal retry exhaustion; battery 0/100/unknown and invalid/unsupported labels/time; DST/time/date rollover/manual zone/12-hour behavior; metadata expiry while link remains live; UART loss/Forget/race/flood/malformed payload rejection and full cluster warning/lifecycle matrix. The app implements runtime validation and cancellation but builds/success of one real snapshot do not prove this full matrix. No automated test files/suites were created or run, no firmware sources were changed, and no large-font/display setting changes were made for this integration.

Android API references used: [GATT operations/MTU](https://developer.android.com/reference/android/bluetooth/BluetoothGatt), [OS battery/time broadcasts](https://developer.android.com/reference/android/content/Intent). The app uses callback-observed MTU, including Android 14+ negotiation behavior, rather than assuming the requested size was granted.
