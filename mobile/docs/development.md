# Companion development, verification and Mac adapter

Updated: 2026-10-31. Android is the implemented physical path; iOS shell/device integration remains separate. Read [background-sync](background-sync.md) for the next implementation task and [protocol](protocol.md) for current wire behavior. No automated test files/suites may be created/restored/run in this phase. Use builds/static analysis, numeric diagnostics and manual device checks; preserve runtime validation and normative vectors.

## Toolchain and launch

Pinned Flutter 3.47.7, framework abaf9c5237, engine deb287481e, Dart 3.13.5; `.flutter-version` does not select the SDK automatically. Keep `pubspec.lock`. Generated Android tooling pins AGP 9.1.0, Kotlin 2.4.0 and Gradle 9.3.1, Java target 17; verify installed JDK/SDK compatibility through doctor and the pinned project. iOS deployment target is 15.0; full Xcode/native setup and separate device verification are required. App IDs/signing are development configuration; release signing remains unconfigured.

With Flutter/Python on PATH, authorized Android USB debugging, compatible Android SDK/JDK and accepted licenses, run from repository root:

```sh
bash scripts/preview-mobile.sh
# If multiple phones are connected, pass the ID from flutter devices:
bash scripts/preview-mobile.sh YOUR_ANDROID_DEVICE_ID
```

The launcher verifies SDK version, selects a real device and resolves locked dependencies. Uncached first builds need network access. Press `r` for hot reload, `R` for restart/opening and `q` to quit. SDK installation, licenses and device authorization remain explicit setup steps. Host firmware/SDL builds require no mobile SDK.

Run checks from `mobile/`:

```sh
flutter --version
flutter doctor -v
flutter pub get --enforce-lockfile
dart format --output=none --set-exit-if-changed lib
flutter analyze --no-pub
flutter build apk --debug --no-pub
# Optional run on the selected real device:
flutter run --no-pub -d YOUR_ANDROID_DEVICE_ID
# Separate iOS gate, only with full Xcode/simulator tooling:
flutter build ios --simulator --no-pub
```

Keep generated native project source and locks in Git. Keep SDKs/caches, build products, captures, isolated stores and credentials out. Do not build/install a mobile toolchain on the firmware Mac merely to verify unrelated firmware/docs.

## Build on GitHub, install from the Mac

The `Mobile APK` workflow (`.github/workflows/mobile-apk.yml`) runs on every push, on all branches, and can also be started from GitHub Actions → Mobile APK → Run workflow. It builds only `mobile/` on a standard Ubuntu runner with Java 17 and the Flutter version in `mobile/.flutter-version`. It restores locked dependencies, checks Dart formatting/static analysis and builds a universal debug APK. No automated suites or firmware builds run. The `r15-companion-debug` artifact contains the APK, SHA-256 checksum and source commit/run metadata; retention is seven days. Public-repository standard runner compute is free; artifact/cache storage has separate limits. Download before expiration or rerun the workflow.

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

The script selects the newest workflow run on the current local branch, prints its remote branch/commit/link, waits for completion and refuses failed builds rather than falling back to an old success. It installs the remote commit shown, including when local changes are unpushed. Immediately after pushing, Actions registration may take a moment; retry if no run exists. For an exact revision use `--run`. Concurrent pushes can supersede queued runs; a canceled run fails explicitly. GitHub CLI downloads/extracts the ZIP into a fresh ignored `build/mobile-actions/run-...` directory, then the script verifies the APK checksum. Downloads accumulate locally; remove old directories manually when no longer needed. Artifact downloads require GitHub authentication even for a public repo. Missing/expired artifacts fail explicitly.

Default ADB: `$HOME/Desktop/learning/platform-tools/adb`; override with `R15_ADB=/absolute/path/to/adb`. Default repository: `CodewaveSandy/r15-smart-cluster`; override with `R15_GITHUB_REPO=OWNER/REPO`. Connect a data-capable USB cable, unlock the phone, enable USB debugging and approve the Mac. Only authorized USB devices are eligible; multiple phones require `--device`. Installation uses `adb install -r` and never uninstalls, clears data or silently accepts downgrades. Open the app manually afterward. Cloud builds have no local hot reload and do not inject Mac-preview bootstrap credentials.

### Stable development signing

The workflow accepts repository Actions secret `ANDROID_DEBUG_KEYSTORE_BASE64`, containing a standard Android debug keystore (alias `androiddebugkey`, key/store passwords `android`). Prefer the key used for the APK already on your phone, recovered from the old development machine's `~/.android/debug.keystore` if available. Upload it without committing it or printing it:

```sh
base64 < /path/to/debug.keystore | gh secret set ANDROID_DEBUG_KEYSTORE_BASE64 --repo CodewaveSandy/r15-smart-cluster
```

Without the secret, Android generates a development key and Actions caches it for reuse. Caches are evictable and branch-scoped; this fallback cannot guarantee the same signer across branches or cache loss. The persistent secret gives stable signing across runs. A different key from the installed app causes `INSTALL_FAILED_UPDATE_INCOMPATIBLE`. Recover/use the original key to preserve app data; if it is irretrievable, uninstalling is a separate deliberate user action that clears app data/selection/trust and requires setup again. The install script never performs that removal. Debug APKs/keys are for development; release signing remains unconfigured. Public artifacts contain no Mac bridge bootstrap or signing keystore.

Workflow/script syntax and static checks can be performed locally without mobile tooling. The first actual Actions build, artifact download and phone installation remain separate verification gates until run; no cloud-build or device success is implied by adding this workflow.

## Current evidence and remaining checks

| Milestone | Recorded evidence |
| --- | --- |
| Android BLE baseline | Samsung SM-M346B / Android 16: actual cluster found, GATT status 0, enrollment name-write status 0, OS BONDING→BONDED, matching real code/name and cluster connected state user-confirmed |
| Saved recovery | User restarted both boards; native disconnect then successful reconnect without new pairing. Explicit Disconnect retained selection/no unwanted retry; Connect restored link. Force-stop followed by explicit app relaunch reconnected without another bond |
| Home and tabs | Approved landing/radar and Home; My bike/Navigate bottom-tab state and connection-page Back behavior physically inspected. Home dark/light and 2x text inspected; reduced motion/extended matrix remain open |
| Metadata | Pinned format/analyze and ARM64 debug build/install passed; MTU 256/status 0, c012 write status 0; user confirmed actual phone name/battery/local date/time. Periodic writes succeeded for over four minutes; Disconnect/Connect and explicit app relaunch delivered fresh accepted snapshots |
| Firmware | Build/package/partition and host preview checks previously passed; user-operated images supplied on both boards, exact flashed hashes not collected for every app run |

These are core milestone observations, not background, extended reliability, iOS or bike acceptance. No automated suites were run for the latest milestones. Current normal bike telemetry is unavailable; Navigate has no provider/map service. Current metadata is transient best-effort native scheduling, with no foreground service.

Still manually verify the latest combined first-pair ordering; reject/cancel/timeouts/closed enrollment; other app occupying the sole slot; phone/cluster asymmetric Forget; radio/permission loss; invalid/unsupported/long names; battery 0/100/unknown; invalid/unavailable time; date rollover and automatic/manual timezone/12-hour formats; unsupported endpoint/MTU failure; missing callback; metadata expiry and delayed/old-session data; independent board reset; NVS/render/radio concurrent load. Preserve warning visibility, focus/back and both 2-second lifecycle sequences. Run [B1 background acceptance](../../docs/background-sync.md#joint-acceptance-b1-locked-screen-continuity) independently of earlier foreground checks.

Record revision/build/configuration, device/OS, image hashes where available, exact steps, duration, power/debug state, numeric native state/write outcomes, gaps/recovery and failures. Capture phone UI in dark/light, small-screen/large-text and reduced motion as affected. Never include pairing codes, keys, labels/addresses, phone contents, routes or raw payloads. Debugger/USB-connected runs cannot establish untethered pocket behavior.

## Physical connection diagnostics

Only one app may occupy the vehicle BLE slot. Disconnect nRF Connect before the companion run. For a new phone open cluster Settings → Phone → Pair phone; saved bonds do not require this. Check real OS approval and numeric GATT/bond results; an absent fault log does not prove a live connection. Firmware accepts `BLE_HS_EALREADY` as a peer-started security procedure rather than disconnecting it. Never fix a failure by weakening MITM, auto-approving codes or erasing NVS/bonds.

Firmware-side log collection (actual port may change):

```sh
/Users/codewavesandy/.platformio/penv/bin/python tools/vehicle/ble_log.py /dev/cu.usbmodem1101
```

The existing app logs numeric `R15BleLink` GATT/MTU/bond/write outcomes without payloads. Existing foreground reconnect uses four attempts with 1/2/4/8-second delays; background cancels retries. Remote-termination/authentication statuses 19/5/137 stop automatic retry, but are not definitive diagnosis. Explicit Retry remains available. Optional callback timeout blocks optional GATT work until reconnect. [M1](background-sync.md) defines the intended service-owned replacement; it is not implemented yet.

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
