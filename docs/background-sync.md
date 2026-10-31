# Firmware background-sync strategy and work plan

Updated: 2026-10-31. Status: planned; this document does not implement new services. Start here for firmware work. App developers start at [the mobile plan](../mobile/docs/background-sync.md). [Current BLE protocol](../mobile/docs/protocol.md) is the single wire-contract authority; [bench wiring](development.md#vehicle-ble-uart-bench) remains authoritative for physical connections.

## Outcome and current baseline

The rider opens the app, connects/selects a route, then locks the phone and puts it in a pocket. Phone work continues independently of its UI. Vehicle acquisition, warnings and display lifecycle remain independent of phone availability.

Working baseline: authenticated numeric-comparison BLE pairing, one saved bond/connection, saved-peer reconnect, enrollment name and authenticated name/battery/UTC metadata. Core phone/board behavior is user-confirmed; extended fault/load checks remain open. Firmware has no Android foreground/background setting. The immediate continuity change belongs to Android; existing firmware can validate it without a new UUID, heartbeat or faster metadata cadence.

Current boundaries: vehicle owns NimBLE, bond NVS and validation; display owns clock/presentation under one LVGL owner. R15G discrete-input, R15P status/control, R15N enrollment identity and R15M metadata are implemented. Telemetry notifications to the phone, navigation messages, Wi-Fi map delivery and display maps are absent. BLE connected does not prove fresh app data. Metadata expires after 90 seconds; display clock continues ticking after a valid sync, with no cold-power retention claim.

## Work independently: F1 continuity baseline

1. Read RULES.md, [architecture](architecture.md#system-architecture-status-and-roadmap), [implementation status](architecture.md#current-status-and-roadmap), this plan and [protocol](../mobile/docs/protocol.md). Inspect `firmware/vehicle/main/BlePhone.{h,cpp}`, `main.cpp`, `shared/protocol/PhoneMetadata.h`, `PhoneLink.h`, `PairingIdentity.h`, `GearPacket.h`, `firmware/display/main/VehicleUart.h` and display `main.cpp`.
2. Build both firmware targets with the [build-only commands](development.md#vehicle-ble-pairing-bench). Do not change runtime code merely to create a firmware diff. Preserve security, saved bonds, storage-failure handling, callbacks and all transport validation.
3. Establish a baseline with the current app: authenticated connection, normal 30-second metadata delivery, correct clock/name/battery, independent UART input updates, explicit Disconnect/Forget and restart. Record revision/toolchain and image hashes rather than assuming installed images match HEAD.
4. Prepare diagnostics for the mobile M1 run. Existing numeric `@BLE` link/security/fault messages and display UART/heap/flush counters are the starting point. Add bounded counters or rate-limited numeric diagnostics only if needed to observe accepted metadata age, parser rejection, overflow or loop starvation. Never log names, addresses, pairing codes, keys or payloads. Keep observation separate from success claims.
5. Run the joint B1 checks below with the app's new service. No wire-format change is needed. If failures occur, distinguish app scheduling, radio loss, stale metadata, UART loss and display rendering before changing timeouts.

F1 completes when firmware remains responsive and the app's background metadata path passes B1. It does not establish bidirectional telemetry or navigation. No radio/clock task belongs on display; no BLE callback may call LVGL or block input sampling.

## F2 reviewed bidirectional data contract

Begin only after B1 passes. Coordinate the specification in [protocol](../mobile/docs/protocol.md#future-telemetry-and-navigation-contract-gate) with the mobile developer before implementing codecs. Freeze exact new UUIDs/message versions, security requirements, payload limits, units/quality, subscriptions, negotiated rates, deadlines and cancellation semantics there. Do not repurpose c011/c012 or claim new endpoints already exist.

Vehicle responsibilities: authenticated telemetry notifications or indications as the reviewed contract requires, feature/version discovery, session/boot identity, current full snapshot after reconnect, independent source age and bounded subscription state. Only forward real available observations; unsupported speed/RPM/fuel/calculations stay unavailable. No phone command may control bike functions.

Navigation input needs distinct per-stream sequence and route generation, current position quality/age, maneuvers/progress, acknowledgement where necessary and explicit end/cancel behavior. A heartbeat cannot refresh old location. Define stale/frozen navigation presentation before enabling it. An authenticated BLE link and a metadata write prove neither route readiness nor display receipt.

Keep bounded latest-state mailboxes for replaceable observations and bounded ordered queues for semantic actions. Separate urgent status/input work from optional bulk transfer. Handle reconnect, reboot, duplicates, malformed frames, missing callbacks, overflow and backpressure without corrupting state. Retain CRC for UART corruption detection; CRC is not authentication. New protocol vectors may be normative data; automated test suites remain deferred per user instruction.

F2 exit: both developers agree the contract and implement compatible bounded paths; actual phone receives real quality-bearing telemetry, source expiry is visible, subscriptions restore after reconnect and input/display behavior survives concurrent load. Agree numeric rates and deadlines from measurement, not assumptions.

## F3 navigation and map feasibility

Phone owns GPS, search/routing/rerouting and permitted map preparation. Vehicle owns radios and bounded forwarding; display owns validated navigation/cache and rendering. First evaluate compact maneuvers/position/route geometry over BLE; this is an intermediate milestone, not fulfillment of the required moving road map.

At 115200 8N1 UART the theoretical raw ceiling is 11,520 bytes/s before framing and existing traffic. An 800 × 480 RGB565 frame is 768,000 bytes (about 66.7 seconds raw). Do not stream dashboard-sized pixels. Measure usable end-to-end bandwidth, latency, parser service and radio/acquisition contention. Keep reviewed pins/baud unchanged until a separate board-validated change is justified.

Bulk map delivery needs provider/data-rights approval, bounded geometry/chunks, region/version/route identity, integrity, credits/backpressure, resumable transfer, cancellation, atomic activation and measured caches. Wi-Fi remains a proposal: authenticate/bind it to the trusted peer, validate BLE/Wi-Fi coexistence and phone cellular internet behavior, then evaluate actual UART/display throughput. PSRAM capacity alone is not feasibility evidence.

## Joint acceptance B1: locked-screen continuity

Use actual Android plus both boards, with the debugger/USB phone tether removed. Board USB power is acceptable for desk checks; it does not verify bike power.

| Scenario | Required observation |
| --- | --- |
| Baseline and 60-minute locked/pocketed soak | Actual metadata delivery/age observed through the run; no unexplained 90-second expiry, false connected claim or growing queues/heap loss |
| Home/another app/lock/unlock, repeated Activity recreation | One authenticated link; no pairing prompt or input/display interruption caused by UI changes |
| Out of range and return; vehicle and display independent reboot | App recovery policy observed, current metadata re-established, old session rejected; display reboot does not imply vehicle reboot |
| BLE connected but metadata paused >90 seconds | Name/battery become unavailable; BLE truth remains separate; last valid display clock may keep ticking |
| UART unplug/recovery with BLE healthy | UART-derived state expires and recovers truthfully; pending pairing fails closed; do not falsely attribute UART loss to Android |
| Explicit Disconnect/notification Stop; app/cluster Forget | No unwanted reconnect or repeated pairing; trust retention/removal follows the current contract |
| Permissions/radio revoked, force-stop or OS Stop | Honest loss/expiry; no firmware wait for a dead phone; recovery only through supported explicit/OS paths |
| Concurrent inputs, Settings/NVS, warnings and lifecycle | Acquisition stays bounded; warnings remain live; both layouts/themes and non-touch controls preserved; artwork remains 2.00 s each |

Record app/firmware revisions, phone/OS, duration, power/debug state, numeric link/write outcomes, metadata gaps, reconnect timing, UART counters, heap/stack/flush observations and unresolved failures. B1 is joint evidence; host renders and builds cannot substitute for it. Extended B2 telemetry and B3 navigation acceptance are gated on their separately frozen contracts.

## Handoff to the mobile developer

Supply both image hashes/configurations, current protocol version, known hardware limitations, how to collect numeric diagnostics and B1 results. For F2/F3 supply the agreed protocol revision, supported capabilities and measured resource/rate limits. Preserve existing pairing/metadata while introducing compatible new services. This plan authorizes preparation and incremental work within an explicitly requested implementation task; it does not itself mark future features implemented.
