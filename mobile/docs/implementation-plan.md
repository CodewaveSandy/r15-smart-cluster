# Companion implementation plan

Updated: 2026-10-21. Designed for Android first and future iOS using Flutter. Host, phone, bench, target and bike evidence remain separate.

## Current scope

This change establishes generated Android/iOS projects, a Flutter composition root, immutable feature-status values, a branded startup and connection-aware home screen, tests and documentation. All phone services are honestly marked not implemented. No BLE/Wi-Fi/protocol/map/plugin/backend integration is supplied. Existing cluster UI is unchanged. Exact executed verification belongs in [development](development.md).

## Delivery stages

| Stage | Deliverable | Acceptance evidence |
| --- | --- | --- |
| M0 Foundation | Independent mobile build, feature boundaries, docs and SDK pin | Format/analyze/tests; Android/iOS build where SDKs available; no fake connected states |
| M1 Contract and map spike | Reviewed contract, security bootstrap, provider/data-rights evaluation; real map area and recorded route in LVGL host | Licensed surrounding-road map/route/position, bounded geometry/cache, measured host timings; target budget documented, no embedded feasibility claim |
| M2 Phone-to-Mac vertical slice | Opt-in authenticated host adapter, Android local-network connection and hello/capability snapshot | Real phone drives preview; malformed/stale/reconnect/reboot/overflow tests; callbacks never touch LVGL; shared golden vectors |
| M3 Android navigation | Permission-aware location, search/route/reroute, map transfer and ride view | Actual Android-to-LVGL moving map; both themes/layouts at 800 x 480; GPS loss, offline coverage, old-route cancellation and link faults |
| M4 Phone services | Supported media metadata/actions, filtered notifications and available call indication | Real supported player/messaging apps, denied/revoked permissions, expired content; no private logging; audio stays with headset |
| M5 Unified Phone journey | Replace cluster Bluetooth/Wi-Fi tiles with Phone; feature readiness, pairing/forget and non-touch flows | Visual review and deterministic captures across themes/layouts/warnings; retain existing diagnostics where useful; no unsupported status claims |
| M6 ESP32 integration | Actual radio adapters, UART codec/scheduler/cache, display map renderer and board port | Both physical MCUs; memory/flash/UART/latency/coexistence/acquisition load; power/reset and limited-throughput faults |
| M7 iOS | Swift/accessory integrations and capability-specific onboarding | Real iPhone build/device tests; permission/background and supported feature matrix; no assumption of Android parity |
| M8 Release evaluation | Signed builds, dependency/license/security/privacy review, operating documentation | Reproducible release artifacts and separate hardware/bike gates; no production/compliance claims without evidence |

M1/M2 may proceed before board delivery. Perform target map proof as soon as hardware arrives; do not wait for a polished full app to discover memory/bandwidth limits. M5 can proceed after behavior/design review; no need to wait for real radios to test the journey. No calendar or frame-rate promise is implied.

## Test strategy

Pure Dart: capability/quality decisions, session transitions, protocol validation and stale deadlines with injected clocks. Flutter: rendering, semantic labels, text scaling, permission states and navigation. Native: Android service lifetime, OS revocation, locked screen, reconnect, background/battery restrictions and supported media controls. Integration: real Android plus Mac bridge, rate limiting, bandwidth/fault injection, simultaneous warnings and lifecycle interruptions. Embedded: board memory/render/UART/radio acquisition load and power tests. iOS runs independent device tests.

CI should use the pinned Flutter revision and lockfile, run format/analyze/tests, Android debug build on suitable runner and unsigned iOS simulator build on macOS/Xcode. CMake checks remain independent. Add CI when runner/toolchain policy is established; don't represent a missing workflow as an enforced gate.

## Decisions still needed

- Exact vehicle MCU and delivered display revision, UART pins/baud and measured resource budget.
- Map provider/routing engine, data licensing/attribution/caching, regional/offline coverage and cost.
- Wire representation, numeric/text/map limits, freshness deadlines and golden vectors.
- Authenticated enrollment, crypto/library choice, secure storage, reconnect and revocation semantics.
- Radio/network topology and phone internet behavior; Android/iOS background support.
- Final app identifiers, OS minimums matched to selected plugins, SDK retention and signing ownership.
- Navigation ride-view geometry, physical input semantics and safe setup access policy.

Record decisions in the authoritative guide above rather than scattering status across new reports.
