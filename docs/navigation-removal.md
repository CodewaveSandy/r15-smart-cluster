# Custom navigation removal — 2026-11-05

The user requested removal first, before implementing Google Maps notification guidance. This change removes the custom phase 1/v2 maps and navigation from firmware, Android and the host preview. The replacement remains a plan in [turn-by-turn-plan.md](turn-by-turn-plan.md).

## Removed live behavior

- Display Navigation menu, map screen, road renderer, map gesture/camera, GPS marker, route/guidance state and PSRAM map allocation. Settings has Appearance, Phone and Clock & Date; keyboard/physical focus wraps across three choices.
- Android Navigate tab, Where to next card, map downloads/cache/route search/provider key UI, GPS provider and location foreground service. MapLibre, Google location dependency and bundled map assets are gone. Android 12+ no longer requests location; fine location stays restricted to API30 and older for BLE scans.
- Map/navigation BLE c016–c01b handlers, queues, subscriptions and transfer pumps, plus UART fragment assembly/encoding/bulk scheduling. c011–c015 and all original G/P/N/M wire bytes remain unchanged. A bounded UART record reader ignores retired R15Q payloads rather than feeding them into live control decoders.
- Map-only preview binaries, importer, desktop map tools and map copyright font generator entry. Old docs, static normative vectors, Kolkata attribution/licence/provenance and dataset are [archived](archive/custom-navigation/README.md), excluded from all runtime assets/builds.

## Preserved behavior

Authenticated pairing/numeric approval, saved bonds, metadata/clock, one GATT/ATT owner, foreground BLE sync/retries/Stop, c013–c015 telemetry/security checks, discrete inputs, dashboard warnings and quality, 2.00s lifecycle, preferences, board/pin/flash guards, 7Semi LED-off initialization, 8192-byte vehicle main-stack configuration/guard, 4096-byte UART RX rings, bounded drains, atomic console write+flush and core UART/heap/stack diagnostics remain.

An upgrade cleanup worker deletes only the retired private `noBackupFilesDir/navigation` tree, navigation/navigation-quota preferences and AndroidKeyStore `r15-navigation` alias. BLE trust and Mac pairing keys remain. Failed cleanup retries on later service creation; no route/key payload is logged.

## Backup and migration

Before removal, all 337 tracked/nonignored files were saved and every archive member verified against a SHA-256 source manifest: `build/source-backups/before-navigation-removal-20261105-174743/source.tar.gz` with `SHA256.json`. This local ignored backup includes the previous uncommitted reliability fixes. Git history also retains the removed implementation.

Deploy both firmware images and the new Actions-built APK together. Old APK map/navigation features are unsupported after the BLE endpoints are removed; missing optional endpoints must not affect pairing/metadata/telemetry. Service Changed remains signaled after authenticated connection to refresh bonded caches. No automatic re-pairing, bond erase, serial access or flash was performed. Physical mixed-version/cache/reconnect and cleanup acceptance remain manual gates.

## Verification

Vehicle build/package and host preview build pass. Dart dependency resolution/format/analyze pass. Host manual render and visual checks cover 800×480 Settings/Clock in both themes/layouts plus warnings/unavailable screens. Display build/package also passes. The four changed Kotlin files parse successfully with standalone ktfmt on temporary copies; this is syntax validation only, not Android type checking. Android compilation remains the GitHub Actions gate. No automated tests, local APK/Gradle/native Android build or physical board/phone check was run.

## Source removal manifest

Directories include their map-only children. Historical documents/data were moved to the archive; source is retained in the verified backup and Git history.

```text
embedded/ui/components/MapPage.h
embedded/ui/components/MapPage.cpp
embedded/ui/components/MapPanel.h
embedded/ui/components/MapPanel.cpp
shared/application/NavigationStore.h
shared/application/NavigationBridge.h
shared/application/MapStore.h
shared/application/MapGesture.h
shared/application/MapCamera.h
shared/model/MapScene.h
shared/model/MapProjection.h
shared/protocol/NavigationProtocol.h
shared/protocol/MapProtocol.h
shared/protocol/MapSha256.h
shared/protocol/MapTunnel.h
firmware/display/main/MapMemory.h
assets/fonts/lvgl/inter_map_copyright_16.c
simulator/lvgl/MapFile.h
simulator/lvgl/MapDemo.h
simulator/lvgl/map_preview.cpp
simulator/lvgl/firmware_map_preview.cpp
simulator/lvgl/navigation_preview.cpp
mobile/android/app/src/main/kotlin/dev/r15cluster/r15_companion/NavigationRoute.kt
mobile/android/app/src/main/kotlin/dev/r15cluster/r15_companion/NavigationSession.kt
mobile/android/app/src/main/kotlin/dev/r15cluster/r15_companion/NavigationSync.kt
mobile/android/app/src/main/kotlin/dev/r15cluster/r15_companion/NavigationWire.kt
mobile/android/app/src/main/kotlin/dev/r15cluster/r15_companion/VectorMaps.kt
mobile/android/app/src/main/kotlin/dev/r15cluster/r15_companion/MapSync.kt
mobile/android/app/src/main/kotlin/dev/r15cluster/r15_companion/MapWire.kt
mobile/android/app/src/main/kotlin/dev/r15cluster/r15_companion/PhoneLocation.kt
docs/map-phase-1.md
docs/navigation-phase-2.md
docs/navigation-phase-2-firmware.md
mobile/docs/navigation-phase-2.md
mobile/docs/navigation-v2-implementation.md
shared/protocol/map-phase-1-vectors.txt
shared/protocol/navigation-v2-vectors.txt
tools/maps/README.md
mobile/assets/maps/
mobile/lib/features/navigation/
tools/maps/
scripts/import-osm-map.py
```

Final build outputs (build-only, no device accessed): vehicle factory SHA-256 `ae4826f8b7e9d34629767cd939a31a3561d53539e24d4230fb46ed5c2af5d83f`; display factory SHA-256 `fdc22cd05add1235798c93bae125d6d979c507a3ecb2b0d630305e6256302e4d`. Final affected firmware build logs contain no compiler warnings. The host render captures are under ignored `build/lvgl/navigation-removal-review/`; no screenshots/build outputs are tracked.
