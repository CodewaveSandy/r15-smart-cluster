# Firmware background-sync strategy and work plan

Updated: 2026-11-01. Status: firmware continuity diagnostics and authenticated discrete-input telemetry are implemented/build-verified; mobile integration and physical B1/B2 remain pending. Start here for firmware work. App developers start at [the mobile plan](../mobile/docs/background-sync.md). [Current BLE protocol](../mobile/docs/protocol.md) is the single wire-contract authority; [bench wiring](development.md#vehicle-ble-uart-bench) remains authoritative for physical connections.

## Outcome and current baseline

The rider opens the app, connects/selects a route, then locks the phone and puts it in a pocket. Phone work continues independently of its UI. Vehicle acquisition, warnings and display lifecycle remain independent of phone availability.

Working baseline: authenticated numeric-comparison BLE pairing, one saved bond/connection, saved-peer reconnect, enrollment name and authenticated name/battery/UTC metadata. Core phone/board behavior is user-confirmed; extended fault/load checks remain open. Firmware has no Android foreground/background setting. Android still owns continuity scheduling; existing c011/c012 behavior is preserved and new optional read/notify endpoints provide bidirectional observability without a faster metadata cadence.

Current boundaries: vehicle owns NimBLE, bond NVS and validation; display owns clock/presentation under one LVGL owner. R15G discrete-input, R15P status/control, R15N enrollment identity and R15M metadata are implemented. Vehicle-to-phone discrete-input telemetry, authenticated capability discovery and health reads are now implemented through c013–c015. Navigation messages, Wi-Fi map delivery and display maps remain absent. BLE connected does not prove fresh app data. Metadata expires after 90 seconds; display clock continues ticking after a valid sync, with no cold-power retention claim.

## Work independently: F1 continuity baseline

1. Read RULES.md, [architecture](architecture.md#system-architecture-status-and-roadmap), [implementation status](architecture.md#current-status-and-roadmap), this plan and [protocol](../mobile/docs/protocol.md). Inspect `firmware/vehicle/main/BlePhone.{h,cpp}`, `main.cpp`, `shared/protocol/PhoneMetadata.h`, `PhoneLink.h`, `PairingIdentity.h`, `GearPacket.h`, `firmware/display/main/VehicleUart.h` and display `main.cpp`.
2. Build both firmware targets with the [build-only commands](development.md#vehicle-ble-pairing-bench). Do not change runtime code merely to create a firmware diff. Preserve security, saved bonds, storage-failure handling, callbacks and all transport validation.
3. Establish a baseline with the current app: authenticated connection, normal 30-second metadata delivery, correct clock/name/battery, independent UART input updates, explicit Disconnect/Forget and restart. Record revision/toolchain and image hashes rather than assuming installed images match HEAD.
4. Prepare diagnostics for the mobile M1 run. Existing numeric `@BLE` link/security/fault messages and display UART/heap/flush counters are the starting point. Add bounded counters or rate-limited numeric diagnostics only if needed to observe accepted metadata age, parser rejection, overflow or loop starvation. Never log names, addresses, pairing codes, keys or payloads. Keep observation separate from success claims.
5. Run the joint B1 checks below with the app's new service. No wire-format change is needed. If failures occur, distinguish app scheduling, radio loss, stale metadata, UART loss and display rendering before changing timeouts.

Firmware F1 preparation is implemented: metadata accept/rejection/expiry, link/notification counts and maximum metadata/host/acquisition gaps are available through authenticated c015; c014 exposes current source/metadata age and display heartbeat. The read-only USB collector supports up to 7200 seconds. Physical F1/B1 completes only when firmware remains responsive and the app's background path passes B1. It does not establish bidirectional telemetry or navigation. No radio/clock task belongs on display; no BLE callback may call LVGL or block input sampling.

## F2 reviewed bidirectional data contract

The user authorized implementing this firmware foundation together with F1 before the app handoff. Implementation is ready; run B1 before physical B2 acceptance. The frozen exact specification in [protocol](../mobile/docs/protocol.md#background-sync-capabilities-and-telemetry-c013c015) now defines c013 capabilities, c014 full discrete-input telemetry and c015 health. Firmware codec, security/CCCD/MTU checks and bounded 200ms latest-state notification scheduling are implemented. Existing c011/c012 and all UART frames are unchanged. App developer implements the exact payloads and native service sequence in the mobile plan.

Vehicle responsibilities: authenticated telemetry notifications or indications as the reviewed contract requires, feature/version discovery, session/boot identity, current full snapshot after reconnect, independent source age and bounded subscription state. Only forward real available observations; unsupported speed/RPM/fuel/calculations stay unavailable. No phone command may control bike functions.

Navigation input needs distinct per-stream sequence and route generation, current position quality/age, maneuvers/progress, acknowledgement where necessary and explicit end/cancel behavior. A heartbeat cannot refresh old location. Define stale/frozen navigation presentation before enabling it. An authenticated BLE link and a metadata write prove neither route readiness nor display receipt.

Keep bounded latest-state mailboxes for replaceable observations and bounded ordered queues for semantic actions. Separate urgent status/input work from optional bulk transfer. Handle reconnect, reboot, duplicates, malformed frames, missing callbacks, overflow and backpressure without corrupting state. Retain CRC for UART corruption detection; CRC is not authentication. New protocol vectors may be normative data; automated test suites remain deferred per user instruction.

Firmware F2 build gate is passed; physical F2 exit remains pending: app implements the supplied contract and compatible bounded path; actual phone receives real quality-bearing telemetry, source expiry is visible, subscriptions restore after reconnect and input/display behavior survives concurrent load. Agree numeric rates and deadlines from measurement, not assumptions.

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


## Supplied firmware implementation and app handoff

`shared/protocol/PhoneTelemetry.h` defines the fixed 20-byte R15C capability, 64-byte R15T telemetry and 80-byte R15H health values; `phone-telemetry-v1-vectors.txt` preserves normative non-executable examples. `BlePhone` owns encrypted reads, current-peer authorization, subscription/MTU enforcement, one latest input mailbox and at most one notification attempt. Native acquisition publishes the existing real gear/lamp/counter sample; no new hardware readings, task, console log flood or UART traffic was added. Numeric expiration emits one bounded `@BLE SYNC metadata-expired` event per expiry; health polling carries the detailed counters. NimBLE attempt callbacks are synchronous in the pinned source and are not phone ACKs.

Only vehicle runtime source changes; the existing display image remains wire-compatible and was rebuilt for compatibility. Do not flash without the normal backup/chip/partition guards. Preserve existing NVS/bonds where supported by the user-operated update procedure; combined factory images may overwrite NVS, so saved reconnect across reflashing is not promised. App checks characteristics/capabilities and gracefully retains old firmware's metadata-only path. c014 needs MTU 67, c015 needs83; request 96+ in the app. No navigation/Wi-Fi capability is advertised.

Give the app developer [the updated mobile handoff](../mobile/docs/background-sync.md#m2-integrate-the-supplied-telemetry-firmware) and [the exact protocol](../mobile/docs/protocol.md#background-sync-capabilities-and-telemetry-c013c015). Complete the app service plus decoder/subscription/health integration, then perform B1/B2 with both boards. F3 remains provider/rights/bandwidth/render feasibility work, not a claimed implemented map service. Implementation/build evidence is not bench/ride acceptance.


## Implementation verification — 2026-11-01

Vehicle firmware build/package passed with ESP-IDF 5.5.4/C++17 and the pinned NimBLE stack; static report: 20,072 bytes RAM and 469,205 bytes application flash (not dynamic heap/stack/radio-load measurements). Display compatibility build and partition/application-fit packaging passed with unchanged runtime source. Host build passed (`ninja: no work to do`); no UI changes required fresh visual captures. The portable telemetry header/vector generator compiled under C++17 with `-Wall -Wextra -Werror -pedantic`; normative byte examples were generated and manually inspected. Python diagnostic collector syntax and `--help` passed without opening a port. Repository Markdown links/anchors and whitespace were checked. No automated test files/suites were created/restored/run.

Current local combined image artifacts (rebuilding may change hashes):

| Artifact | SHA-256 |
| --- | --- |
| `build/vehicle/sevensemi-s3/firmware.factory.bin` | `20402cf5fff0582d44c53e27a9bd176c233fbd433a5359010fe7b6d46d713d63` |
| `build/display/crowpanel-v3/r15-display.factory.bin` | `4bbbb695e87fbf8ab9fa93f1906858b473c503690376ce936c6baa0ac5d7d8c3` |

No boards were accessed/flashed, no actual GATT/CCCD/MTU exchange or Android background service was exercised, and no 60-minute soak/radio/UART/heap/stack/panel/bike measurements were performed. Existing physical pairing/metadata evidence is historical baseline only; new endpoints and regression checks require the app/bench B1/B2 run. The vehicle image is the only changed runtime target; an already-compatible display image need not be reflashed for these optional BLE endpoints. Keep the eight-guide organization; the mobile handoff and protocol are updated in place.
