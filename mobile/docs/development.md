# Companion development, verification and Mac adapter

**Android integration — 2026-11-06:** The notification listener/parser, native sharing session, existing-owner BLE channel and My bike controls are now source-integrated against firmware `50f6bc2`. See the [implementation and manual APK checklist](turn-guidance-implementation.md). Local verification is formatting/static analysis/syntax only; APK/Android compilation belongs to GitHub Actions and phone/board acceptance remains pending. Earlier “Android pending/unimplemented” passages describe the previous handoff state.

**2026-11-05 removal:** The custom map/navigation implementation and its menu/tab have been removed. Pairing, metadata/time, telemetry and the UART/vehicle-stack fixes remain. Google Maps notification turn guidance is planned, not implemented. See the [removal record](../../docs/navigation-removal.md).

## Toolchain and launch

Pinned Flutter 3.47.7, framework abaf9c5237, engine deb287481e, Dart 3.13.5; `.flutter-version` does not select the SDK automatically. Keep `pubspec.lock`. Generated Android tooling pins AGP 9.1.0, Kotlin 2.4.0 and Gradle 9.3.1, Java target 17; verify installed JDK/SDK compatibility through doctor and the pinned project. iOS deployment target is 15.0; full Xcode/native setup and separate device verification are required. App IDs/signing are development configuration; release signing remains unconfigured.

**Current user workflow (2026-11-03): code locally, build APKs in GitHub Actions, install through platform-tools/ADB, and manually test on the phone.** Do not attempt local Android builds, Flutter device runs, ad-hoc native compilation, Android SDK installation or Android Studio installation. Earlier local compilation records below are historical evidence, not instructions to repeat.

Optional non-build checks from `mobile/`, when the existing Flutter/Dart tooling is available:

```sh
flutter pub get --enforce-lockfile
dart format --output=none --set-exit-if-changed lib
flutter analyze --no-pub
```

App UI verification targets modern/latest Android devices at normal text size. Small-screen/2× text checks are not required unless the user requests them; older records below are historical. For APK generation/install use the cloud workflow below. The existing local launcher is a legacy tool, not the current app development workflow; do not invoke it automatically. iOS work remains separate and is not authorized by this Android phase.

Keep generated native project source and locks in Git. Keep SDKs/caches, build products, captures, isolated stores and credentials out. Do not build/install a mobile toolchain on the firmware Mac merely to verify unrelated firmware/docs.

## Build on GitHub, install from the Mac

The `Mobile APK` workflow (`.github/workflows/mobile-apk.yml`) runs on every push, on all branches, and can also be started from GitHub Actions → Mobile APK → Run workflow. It builds only `mobile/` on a standard Ubuntu runner with Java 21 and the Flutter version in `mobile/.flutter-version`. It restores locked dependencies, checks Dart formatting/static analysis and builds a universal debug APK. No automated suites or firmware builds run. The `r15-companion-debug` artifact contains the APK, SHA-256 checksum and source commit/run metadata; retention is seven days. Public-repository standard runner compute is free; artifact/cache storage has separate limits. Download before expiration or rerun the workflow.

The Mac needs GitHub CLI and your existing platform tools, with no Android Studio, Flutter, JDK or full Android SDK required for this path:

```sh
# One-time lightweight CLI setup (if not already installed):
brew install gh
gh auth login --hostname github.com

# After committing/pushing through your normal Git workflow:
bash scripts/install-mobile-apk.sh
# Optional explicit branch, run or USB device:
bash scripts/install-mobile-apk.sh --branch main
bash scripts/install-mobile-apk.sh --run YOUR_RUN_ID --device YOUR_USB_SERIAL
bash scripts/install-mobile-apk.sh --download-only
```

The script selects the newest workflow run on the current local branch, prints its remote branch/commit/link, waits for completion and refuses failed builds rather than falling back to an old success. It installs the remote commit shown, including when local changes are unpushed. Immediately after pushing, Actions registration may take a moment; retry if no run exists. For an exact revision use `--run`. Concurrent pushes can supersede queued runs; a canceled run fails explicitly. Numbered stage messages identify build waiting, artifact lookup/download, extraction/checksum verification, USB discovery and installation. GitHub CLI looks up the artifact; macOS's bundled `curl` downloads the ZIP with a live meter showing percentage, received bytes, speed, elapsed time and ETA (total/ETA depend on the server supplying a content length). Slow/stalled downloads fail after 90 seconds below 1 KiB/s and transient failures are retried up to three times. The GitHub token is passed privately through stdin and is not forwarded to another host on redirects. Bundled `unzip` extracts into a fresh ignored `build/mobile-actions/run-...` directory, then the script verifies the APK checksum. Downloads, including the ZIP, accumulate locally; remove old directories manually when no longer needed. Artifact downloads require GitHub authentication even for a public repo. Missing/expired artifacts fail explicitly. USB transfer/installation reports its stage and ADB's own output, without inventing an installation percentage.

Default ADB: `$HOME/Desktop/learning/platform-tools/adb`; override with `R15_ADB=/absolute/path/to/adb`. Default repository: `CodewaveSandy/r15-smart-cluster`; override with `R15_GITHUB_REPO=OWNER/REPO`. Connect a data-capable USB cable, unlock the phone, enable USB debugging and approve the Mac. Only authorized USB devices are eligible; multiple phones require `--device`. Installation uses `adb install -r` and never uninstalls, clears data or silently accepts downgrades. Open the app manually afterward. Cloud builds have no local hot reload and do not inject Mac-preview bootstrap credentials.

### Stable development signing

The workflow accepts repository Actions secret `ANDROID_DEBUG_KEYSTORE_BASE64`, containing a standard Android debug keystore (alias `androiddebugkey`, key/store passwords `android`). Prefer the key used for the APK already on your phone, recovered from the old development machine's `~/.android/debug.keystore` if available. Upload it without committing it or printing it:

```sh
base64 < /path/to/debug.keystore | gh secret set ANDROID_DEBUG_KEYSTORE_BASE64 --repo CodewaveSandy/r15-smart-cluster
```

Without the secret, Android generates a development key and Actions caches it for reuse. Caches are evictable and branch-scoped; this fallback cannot guarantee the same signer across branches or cache loss. The persistent secret gives stable signing across runs. A different key from the installed app causes `INSTALL_FAILED_UPDATE_INCOMPATIBLE`. Recover/use the original key to preserve app data; if it is irretrievable, uninstalling is a separate deliberate user action that clears app data/selection/trust and requires setup again. The install script never performs that removal. Debug APKs/keys are for development; release signing remains unconfigured. Public artifacts contain no Mac bridge bootstrap or signing keystore.

Verification on 2026-11-02: shell syntax/help and diff whitespace checks passed. GitHub run `41427729738` for commit `b7f7028` completed successfully. A real download of its 147.7 MiB artifact showed live percentage, received bytes, speed and ETA; it was deliberately interrupted after confirming the meter. Full download/extraction/checksum and phone installation through the revised script remain unverified. No automated suites were run.

## Current evidence and remaining checks

| Milestone | Recorded evidence |
| --- | --- |
| Android BLE baseline | Samsung SM-M346B / Android 16: actual cluster found, GATT status 0, enrollment name-write status 0, OS BONDING→BONDED, matching real code/name and cluster connected state user-confirmed |
| Saved recovery | User restarted both boards; native disconnect then successful reconnect without new pairing. Explicit Disconnect retained selection/no unwanted retry; Connect restored link. Force-stop followed by explicit app relaunch reconnected without another bond |
| Home and tabs | Approved landing/radar and Home; My bike/Navigate bottom-tab state and connection-page Back behavior physically inspected. Home dark/light and 2x text inspected; reduced motion/extended matrix remain open |
| Metadata | Pinned format/analyze and ARM64 debug build/install passed; MTU 256/status 0, c012 write status 0; user confirmed actual phone name/battery/local date/time. Periodic writes succeeded for over four minutes; Disconnect/Connect and explicit app relaunch delivered fresh accepted snapshots |
| Firmware | Build/package/partition and host preview checks previously passed; user-operated images supplied on both boards, exact flashed hashes not collected for every app run |

These are core milestone observations, not background, extended reliability, iOS or bike acceptance. No automated suites were run for the latest milestones. These older milestones predate the native foreground-service/telemetry integration and current phase 1 map implementation; consult the later verification sections for current source status.

Still manually verify the latest combined first-pair ordering; reject/cancel/timeouts/closed enrollment; other app occupying the sole slot; phone/cluster asymmetric Forget; radio/permission loss; invalid/unsupported/long names; battery 0/100/unknown; invalid/unavailable time; date rollover and automatic/manual timezone/12-hour formats; unsupported endpoint/MTU failure; missing callback; metadata expiry and delayed/old-session data; independent board reset; NVS/render/radio concurrent load. Preserve warning visibility, focus/back and both 2-second lifecycle sequences. Run [B1 background acceptance](../../docs/background-sync.md#joint-acceptance-b1-locked-screen-continuity) independently of earlier foreground checks.

Record revision/build/configuration, device/OS, image hashes where available, exact steps, duration, power/debug state, numeric native state/write outcomes, gaps/recovery and failures. Capture phone UI in dark/light and unavailable/permission states on the current modern target device at normal text size; no small-screen/2× text checks unless requested. Never include pairing codes, keys, labels/addresses, phone contents, routes or raw payloads. Debugger/USB-connected runs cannot establish untethered pocket behavior.

## Background-sync integration verification

2026-11-02: `ClusterSyncService`/M1 and c013–c015/M2 are source-integrated; no firmware source was changed. Flutter 3.47.7/Dart 3.13.5 formatting and `flutter analyze --no-pub` passed. All Android Kotlin sources compiled with the cached pinned Kotlin 2.4.0 compiler, Java 17 and official Android API 36 compile jar plus cached Flutter/AndroidX classes. Cached pinned aapt2 9.1.0 compiled/linked the app resources against API 36, using a temporary manifest copy with the standard application placeholder/namespace resolved; Kotlin then compiled against those generated resource IDs. These checks do **not** establish the full Gradle/Flutter/plugin manifest merge, DEX/APK packaging or runtime acceptance. SDK platform download/check outputs and verified pre-edit source backups remain ignored under `build/`; no Android Studio/full SDK was installed. Pinned Flutter defaults resolve Android min SDK 24, target/compile SDK 36; confirm the final merged APK in Actions.

The published capabilities, no-source/fresh/stale telemetry and health vectors were inspected manually in a Java REPL against the Kotlin decoder. Observed exact boot/token, gear 6, lamp phases, press counters, unsupported slots, source/metadata ages and health counters matched the normative examples. A timed read with 20ms RTT produced a conservative 43ms source-age bound; a later observation became Stale, duplicate sequence was refused and a corrupt packet decoded to no value. Additional REPL inspection confirmed notification-before-read anchoring without rewinding the newest receipt, uint32 sequence/uptime rollover, invalid quality with a valid CRC rejected, and connection-token mismatch raising a session fault. No executable test files/suites were created or run. These spot checks are not the full negative/fault matrix or hardware acceptance.

An isolated ignored web preview built from the actual Home widget was used for manual inspection of dark neutral gear, light real gear 6, light stale/Unsupported tiles at 2x text (390 × 844), and dark unavailable/Unsupported tiles on a 320 × 568 screen, including scrolling. This is presentation evidence only, with explicit preview observations; no native BLE was simulated as device success. Android notification permission/Stop, Activity recreation, live gear/lamps/counters, radio/range/board reset, saved security recovery and the full untethered 60-minute B1/B2 matrix remain pending. No iOS, GPS/navigation, energy/resource or bike acceptance is claimed.

Push the reviewed changes through the normal Git workflow to build the actual Android APK in Actions, then install with `bash scripts/install-mobile-apk.sh`. Connect while visible; check the foreground notification and real cluster metadata/gear, then lock/untether the phone for B1/B2. Record Actions run/commit, APK hash, flashed firmware hashes, phone/OS, notification/Nearby permissions and battery settings with numeric observations.

For a cloud diagnostic build, use GitHub Actions → Mobile APK → Run workflow and enable `link_diagnostics` (ordinary push builds leave it off). Use that Actions input for diagnostic APKs; do not build them locally. The artifact build-info records the diagnostics flag. This enables ten-second serialized c015 polling and numeric `R15BleLink` age/gap/RTT/drift-budget/counter logs while sync is active. Normal builds read one health baseline only. Collect with the existing platform tools:

```sh
~/Desktop/learning/platform-tools/adb logcat -s R15BleLink:I
```

Logs expose no pairing codes/keys, device names/addresses or raw payloads. Health attempts are not delivery acknowledgements. Keep raw captures untracked and inspect source/receipt age independently from metadata/link/service state. The provisional 1000ppm + 2ms clock uncertainty budget is enlarged by observed anchor timing changes and still needs physical validation. Service recovery after process death/force-stop/reboot requires reopening the app; explicit Stop remains stopped until Connect. See [implemented policy](background-sync.md#implemented-android-policy-and-remaining-acceptance).

## Physical connection diagnostics

Only one app may occupy the vehicle BLE slot. Disconnect nRF Connect before the companion run. For a new phone open cluster Settings → Phone → Pair phone; saved bonds do not require this. Check real OS approval and numeric GATT/bond results; an absent fault log does not prove a live connection. Firmware accepts `BLE_HS_EALREADY` as a peer-started security procedure rather than disconnecting it. Never fix a failure by weakening MITM, auto-approving codes or erasing NVS/bonds.

Firmware-side log collection (actual port may change):

```sh
/Users/codewavesandy/.platformio/penv/bin/python tools/vehicle/ble_log.py /dev/cu.usbmodem1101
```

The existing app logs numeric `R15BleLink` GATT/MTU/bond/write outcomes without payloads. The native foreground service owns four reconnect delays of 1/2/4/8 seconds and a 30-second cooldown while active access/bond/radio remain valid. Remote-termination/authentication statuses 19/5/137 stop automatic retry, but are not definitive diagnosis. Explicit Retry remains available. An eight-second missing ATT callback closes that GATT before service-owned recovery. [M1](background-sync.md) records the implemented policy and pending physical acceptance.

## Mac preview adapter

Development-only alternative to real ESP32: public BLE discovery of a Mac endpoint, then pinned TLS application enrollment/session over the same reachable LAN. USB only installs/debugs; it is not cluster traffic. Mac adapter trust/session flags must never imply physical BLE/application/navigation readiness.

From repository root in separate terminals:

```sh
bash scripts/preview.sh --phone-bridge
bash scripts/preview-mobile.sh --mac-preview
# Optional host interface override if a VPN selects the wrong address:
R15_PREVIEW_HOST=YOUR_MAC_LAN_IPV4 bash scripts/preview.sh --phone-bridge
```

The launcher builds the Swift CoreBluetooth/Keychain helper as `R15 Nearby.app`, starts the TLS bridge/LVGL host and supplies bootstrap to the app. Allow relevant macOS Bluetooth/Keychain/network and Android access prompts. The plain host preview has no listener. Use the current bootstrap for first trust; already saved trust may rediscover a changed endpoint on foreground reconnect. Stop old previews first.

| Boundary | Development contract |
| --- | --- |
| BLE | Service `813b1240-8fd9-4d67-a53b-9f0a2531c001`, read-only characteristic `813b1240-8fd9-4d67-a53b-9f0a2531c002`; public stable hex ID/name/IPv4/port, max 512 bytes; no secrets |
| TLS/trust | Stable Keychain CA/identity and short-lived IP certificate; protected developer bearer bootstrap, no accept-all TLS/global trust installation. Android Keystore AES-GCM/no-backup atomic trust; no credential-bearing APK/bootstrap distribution |
| HTTP | `/offer`, code/cluster-approved `/pair`, ticket-bound `/resume`, `/status`, `/disconnect`, `/forget`; bounded 512-byte requests/2048-byte responses and operation timeouts |
| Enrollment | Offers 60 seconds, enrollment windows five minutes, invalid-code attempts ten; Pair phone reopens enrollment. Saved trust resumes without new pairing; Disconnect retains trust and pauses session retry; confirmed Forget revokes it |
| Host IPC | Private Unix datagram `R15D2:C/D`, little-endian CLOCK_MONOTONIC ms, flags (paired/requested/approved), six ASCII digits: 22 bytes; legacy 15-byte decoding remains for compatibility. Strict bounded parser publishes normalized state outside LVGL |
| Controls | Local bounded nonblocking P/A/R/D/F semantic effects; Python owns TLS/Keychain/network, one LVGL owner consumes state |
| Freshness | Phone heartbeat two seconds; host disconnected after six seconds; IPC unavailable after 1.5 seconds. No old heartbeat revives an expired session |

Secrets stay in protected stores/private temporary files; bootstrap/build outputs are ignored. This adapter neither provisions ESP32 Wi-Fi nor implements road background execution, live bike telemetry, navigation or iOS integration. Its physical phone/Mac BLE/LAN acceptance is separate from the confirmed real ESP32 BLE path. Preserve it during M1; do not copy its UUIDs, HTTP tickets, timeouts or bootstrap into the product contract.
