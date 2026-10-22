# Companion architecture

## Decisions and scope

Flutter/Dart shares mobile UI and application logic across Android and future iOS. Kotlin and Swift implement platform-specific services where needed. One repository permits atomic protocol/app/firmware changes; independent Flutter/Gradle/Xcode and CMake/ESP-IDF builds avoid requiring mobile tooling for the cluster. App and firmware versions release independently and negotiate protocol capabilities.

No Android Auto/CarPlay receiver or additional processor is required by the selected product direction. The requirement is a real moving road map, route and position, maneuvers and ETA, with phone notifications and music information. Embedded map feasibility remains unproved; arrows and a route on an empty background are milestones/fallbacks, not fulfillment.

## Ownership

```text
Android/iOS OS services + map/routing provider
                  |
Flutter app: destination, route session, permissions, phone connection
                  | BLE control + Wi-Fi bulk (proposed)
Vehicle ESP32: authenticated session, validation, bounded forwarding
              + vehicle acquisition/calculations/physical controls
                  | reviewed UART protocol
Display ESP32: coherent state -> presenter -> one LVGL owner
```

Phone owns location, search, routing/rerouting, map preparation/downloads and permitted notification/media access. Vehicle MCU owns radios and acquisition; navigation handling does not mean running a routing engine. Display owns map rendering/cache and presentation. Map data crosses both phone-to-vehicle and vehicle-to-display links. No pixels over the telemetry UART. Do not bypass vehicle radio ownership to hide a bottleneck.

Mac development substitutes a host phone-service adapter for the vehicle service, delivering normalized messages to the existing simulator. Network callbacks never call LVGL. A bounded mailbox carries latest observations; commands use a bounded ordered queue. Native Android BLE scanning/GATT and protected trust storage, a Mac CoreBluetooth advertiser/Keychain helper, a TLS bridge and bounded host IPC adapter are now implemented; see [Wi-Fi Mac preview connection](mac-preview-connection.md). The physical phone/LAN run and ESP32 adapters remain separate evidence.

## Code organization and dependency rules

`lib/app/` is the composition root and app theme. `lib/features/phone/domain/` contains immutable capability/status values without Flutter imports. `presentation/` renders status. Add an application controller when there are actual asynchronous actions/state transitions to own. The home uses explicit constructor composition and immutable connection/feature snapshots. A widget-owned cancellable timer handles the brief startup branding only; it owns no phone service. Buttons open honest pending-feature explanations until adapters are implemented. No global locator, generic event bus, generated dependency-injection framework, or repositories without a real data source.

Add navigation/media/notification feature folders when implemented. Introduce transport and protocol modules with the first specified executable contract, not empty placeholders. Platform plugins belong behind narrow typed adapters; domain/application logic must not import Bluetooth, location, map SDKs, or native method channels. Widgets never parse packets, own keys/storage, or call OS permissions directly.

When live connection work starts, a session service owns connection lifetime and cancellation. A repository may combine its observed state for consumers; views emit intents and never optimistically assert successful pairing/disconnection. All subscriptions, timers and sockets have explicit teardown. Inject clocks and adapters for deterministic tests. Monotonic time governs deadlines; UTC is only for timestamps with explicit quality. Bound map caches, transfer buffers, notification text, command queues and diagnostic histories before enabling streams.

## Platform boundaries

Android integrations: location/background execution, BLE, Wi-Fi network requests, notification listener, active media sessions and supported controls. iOS needs a separate permission/background/accessory capability assessment; ANCS/AMS may require direct ESP32 accessory implementation. Shared Flutter code cannot grant access to other apps or guarantee Android/iOS parity. Unsupported is distinct from permission denied, unavailable and stale.

Map provider selection must establish routing coverage, offline behavior, external-display/data rights, attribution, caching, costs, Flutter/native integration and a legal embedded data path. A phone map widget alone is insufficient. Do not scrape Google Maps notifications as the navigation contract or forward rendered SDK maps without permitted support.

## Scalability and release

Keep feature modules cohesive and extract shared packages only after actual reuse. Start without third-party runtime dependencies; evaluate plugins for maintenance, license, platform/background support and testability before pinning. Commit app dependency lockfile and generated native project source, not build products. Pin SDK revision and generated platform toolchain together. Application identifiers are provisional development identifiers until ownership/signing is decided.

No backend/account is required for the local companion connection. Provider API requirements are assessed separately. No app path may control vehicle functions. Phone failures must not block boot, shutdown, warnings or acquisition.

## References

Reviewed 2026-10-21: [Flutter architecture](https://docs.flutter.dev/app-architecture/guide), [platform channels](https://docs.flutter.dev/platform-integration/platform-channels), [Android media sessions](https://developer.android.com/reference/android/media/session/MediaSessionManager), [Apple ANCS](https://developer.apple.com/library/archive/documentation/CoreBluetooth/Reference/AppleNotificationCenterServiceSpecification/Introduction/Introduction.html). Recheck mutable APIs when selecting pinned dependencies.

## Typography

The app uses the cluster’s Inter Medium/SemiBold for interface text and Chakra Petch Bold Italic for the R15 brand mark. Mobile bundles matching copies under `mobile/assets/fonts/`, including SIL OFL licenses. Font paths must stay inside the Flutter project: parent-directory font references can appear in the manifest while being omitted from the Android APK. The authoritative provenance and source files remain under root `assets/fonts/`; synchronize mobile copies when updating those sources. No runtime font download is required. Mobile sizes follow Material text roles rather than the cluster’s fixed pixel sizes.
