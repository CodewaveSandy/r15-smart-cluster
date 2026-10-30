# Companion implementation plan

Updated: 2026-10-30. Android and iOS Flutter companion; Android implementation/device verification first, with separate iOS verification. Host, phone, bench, target and bike evidence remain separate.

## Approved implementation sequence

Current verification override, requested by the user on 2026-10-30: no new automated test cases for now. Validate manually with preview/device checks, formatting/static analysis and affected builds; preserve existing tests. Automated test/golden authoring gates below are deferred until the user resumes that work. Do not claim manual checks not actually performed.

The user approved [Concept 01 and its visual contract](visual-design.md) on 2026-10-30 and requires modular architecture, reusable components, standard practices, smooth performance and a final result faithful to the approved visuals. This is a locked sequence and acceptance plan, not a claim of implementation or authorization to begin coding yet.

All implementation in this chat is limited to `mobile/`, including its Dart, Android/iOS adapters, tests, assets and documentation. Firmware, shared C++, simulator, root tooling and hardware configuration remain read-only. A missing accessory service is an external dependency to document, never permission to change firmware. Existing working preview/session behavior must survive each incremental stage.

| Step | Implementation scope | Completion gate |
| --- | --- | --- |
| 1. Design system and asset preparation | Measure Concept 01; freeze dark/light tokens, text roles, spacing, motion, responsive rules and approved bike/rectangular-HMI assets. Establish screenshot fixtures from current behavior. | Reviewed artwork and screen-state specifications; no speculative packages; all assets local, licensed/provenance recorded and appropriately sized. |
| 2. Reusable presentation foundation | Add shared controls only when first consumed: screen shell, action button, status pill, surface, telemetry tile, bike hero, modal layout and two-destination navigation. Keep constructor-based dependency composition. | Existing tests remain green; focused component/semantics checks; light/dark, text scaling and small-screen inspection; no service calls or parsing inside widgets. |
| 3. Opening and first-time flow | Implement the illustrated splash/pulse and explicit first-time/returning-user routing. Preserve native launch behavior separately; branding never waits for radios. | Cancellable animation, reduced motion, background/resume/disposal and deterministic captures; returning users can see disconnected/reconnecting Home. |
| 4. Pairing experience | Implement bounded radar, found-device cards and approval/error states using the existing development session through narrow adapters. Preserve saved trust, disconnect and confirmed forget. | Scan/empty/denied/disabled/wrong-peer/timeout/reject/reconnect/storage-failure tests; no fake device/success; Android manual preview. Clearly distinguish the Mac adapter from physical ESP32 bonding. |
| 5. My bike and telemetry presentation | Build the approved hero/Home and navigation entry. Introduce immutable quality/source/unit-bearing telemetry models and an application controller when real streams require them. Keep unavailable data honest. | Pixel/visual review against approved Home; stale/disconnect/reboot boundaries and independent field updates; live contract absent means unavailable values, not sample bike readings. |
| 6. Map and navigation contract decision | Research current primary provider docs and evaluate Android/iOS SDKs, search/routing, licensing/attribution, regional/offline behavior, costs and permitted embedded delivery. Specify cancellation, source quality and bounded data interfaces. | Record selected provider and limitations before adding its SDK. Resolve required credentials/paid-service decisions with the user. No assumption that a Flutter map widget grants cluster data rights. |
| 7. Destination search and route overview | Integrate the selected phone map, permission-aware location, search, destination details, full route path, ETA/distance and Start ride. | Real provider-backed phone results; debounced/cancellable search, old-result rejection, no-route/offline/denied states, legal attribution and reviewed map styling. |
| 8. Active phone navigation | Add moving position, next-turn overlay, GPS speed, route-follow/overview/recenter, rerouting, arrival and End ride. Add platform background services only as supported and needed. | Real Android GPS/locked-screen/background/revocation tests, stale/low-accuracy treatment, route-generation cancellation and profile-mode measurements. Phone navigation remains distinguishable from cluster projection. |
| 9. Mobile accessory integration | Implement only mobile adapters against an available reviewed accessory contract: capabilities, authenticated sessions, real telemetry and navigation delivery. Retain Mac development support without making it the production transport. | Physical accessory evidence, shared-contract fixtures consumed on the mobile side, reconnect/reset/flood/partial-forget tests. Currently blocked by absent companion application GATT/Wi-Fi/navigation services; no firmware edits or invented UUIDs/packets. |
| 10. iOS validation and final polish | Verify Swift/platform capabilities on macOS/Xcode and real iPhone; audit all approved screens, accessibility, lifecycle, assets, dependencies, privacy and performance. | iOS builds and device evidence separately recorded; visual fidelity accepted; unsupported capabilities explicit; signed release/distribution remains a separately authorized milestone. |

Steps 1–5 can proceed without real accessory services, using honest unavailable states and isolated labelled design fixtures. Step 6 precedes map SDK commitment; step 9 depends on a separately delivered accessory contract. iOS-friendly boundaries and layout checks apply from step 1; step 10 supplies native evidence rather than deferring portability until the end. Notifications/media are outside this four-screen implementation and remain later optional milestones.

At every step: format, analyze, build affected mobile targets where available, perform the current manual checks, review representative screenshots and report measured evidence/limitations. Do not start the next dependent step with a known regression. Design iteration is allowed to achieve the approved appearance; adding screens, services, paid providers or changes to the approved direction requires an explicit recorded decision. Routine review does not require repeated approval for already authorized work.

### First implementation slice: 2026-10-30

Follow-up: the user approved the [first-connection refinement](visual-design.md#approved-first-connection-refinement). The illustrated no-trust landing, real-scan radar, rectangular-HMI discovery cards and existing code approval flow are implemented. Plain launches can now request native Android discovery; without a configured development session, a discovered device does not become paired and the missing application adapter is explained. No new protocol/firmware service was added. Existing saved/connected presentation is retained pending the Home milestone. New UI checks remain manual per user instruction; native discovery, large-text/light/reduced-motion appearance and pairing failure states require phone review.

User manual preview acceptance (2026-10-30): the user reports the new landing/discovery presentation looks perfect and requests committing it. This records visual acceptance of the preview, not proof of accessory pairing, exhaustive permission/large-text/reduced-motion coverage, iOS behavior or measured performance.

Shared dark/daylight blue theme tokens, component theme defaults, a reusable local bike illustration and the illustrated opening/pulse are implemented. Branding still hands off after 900 ms to the existing connection-aware screen. Pairing, Home telemetry and map screens have not yet been migrated to Concept 01. First-time/returning routing and the rectangular HMI artwork remain upcoming, so steps 1–3 are partially complete rather than fully accepted.

Format/static analysis and the Android ARM64 debug build pass. Before the manual-testing instruction arrived, the existing 18 tests plus 13 temporary opening/theme checks passed (live Mac TLS test skipped). The new test file was subsequently removed, preserving a verified ignored backup; no new test cases remain in source. Host opening captures were visually inspected in dark/light at 390 x 844 and with large text at 320 x 568. Actual Android appearance, reduced motion and profile-mode smoothness still require device review. No iOS or performance acceptance claim. See the development guide for manual preview checks.

## Current scope

Current discovery refinement: exact `R15 Cluster` prefix filtering is enforced in native Android results, Dart decoding and popup rendering. All retained matches appear together in a zoomable radar with adaptive icons and stable decorative scatter. Five-second scan windows repeat after a three-second rest; selections cancel scanning before bonding, and background/disposal invalidates late results. User rules are also recorded in mobile AGENTS.md. Manual multi-device/radio/visual acceptance is pending; no new tests are authored.

User preview feedback (2026-10-30): the user reports the current filtered radar presentation looks good and requests committing these changes. This is functional/visual preview acceptance; physical code-confirmed bonding, exhaustive lifecycle/permission faults, dense layouts and measured performance are not established by that feedback. Static analysis and Android ARM64 debug build pass. No firmware files changed.

Radar popup follow-up: discovery results now appear as tappable device icons with two-line full names inside static radar rings, with swipe paging rather than large cards. Selecting a physical BLE device starts OS-managed numeric-comparison bonding; the Android receiver checks the actual bond state and never converts it into an app session. Work is bounded to 45 seconds, has explicit failure/denial, and releases its temporary GATT/receiver resources. Firmware remains unchanged. Android code approval and actual ESP32 bonding need manual acceptance; no new automated tests are added.

Android access/discovery follow-up (2026-10-30): startup/resume permission verification, explicit denied/settings/radio states and delayed saved preview reconnect are implemented. BLE advertisement scanning no longer requires the Mac-only UUID; it prioritizes unverified R15 candidates among bounded real results. Firmware was inspected read-only to establish its advertised name and absent application service. ESP32 app pairing/data and native iOS services remain pending. Manual acceptance instructions are in the development guide; no new automated tests were added.

This change establishes generated Android/iOS projects, a Flutter composition root, immutable feature-status values, a branded startup and connection-aware home screen, tests and documentation. A [development Wi-Fi Mac preview session](mac-preview-connection.md) now implements TLS enrollment, heartbeat, disconnect/resume and confirmed forget. Native Android/Mac BLE discovery and protected saved pairing are implemented. ESP32 Wi-Fi provisioning and navigation/media/notification services remain pending. The unified cluster Phone page uses normalized bridge observations, including first-pair approval. Separate rider-facing radio pages are removed. Exact executed verification belongs in [development](development.md).

## Broader product roadmap (context, not this chat's edit scope)

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

## Integration decisions still needed

- Use the latest root hardware/bench guides for recorded board identities and UART details; measured resource headroom and the full application/map contract remain external dependencies.
- Map provider/routing engine, data licensing/attribution/caching, regional/offline coverage and cost.
- Wire representation, numeric/text/map limits, freshness deadlines and golden vectors.
- Authenticated enrollment, crypto/library choice, secure storage, reconnect and revocation semantics.
- Radio/network topology and phone internet behavior; Android/iOS background support.
- Final app identifiers, OS minimums matched to selected plugins, SDK retention and signing ownership.
- Navigation ride-view geometry, physical input semantics and safe setup access policy.

Record decisions in the authoritative guide above rather than scattering status across new reports.
