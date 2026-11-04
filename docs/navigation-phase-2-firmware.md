# Phase 2 firmware implementation work packages

Updated: 2026-11-04. **Firmware implementation complete; built/manual-host-render evidence and physical acceptance are distinct.** The [exact v2 contract](../mobile/docs/protocol.md#phase-2-navigation-extension--implemented-firmware-contract) is implemented. Old APK/v1/metadata/telemetry security and bytes, pins, baud and partitions are preserved. Android v2 is the next independent target.

## Implementation status and next dependency

| Work package | Implemented result |
| --- | --- |
| NAV-F0 | Numeric observed-MTU/admission/backpressure/downstream-ACK, UART encoded/bulk/framing/CRC/timeout/queue/critical-gap, heap/stack/PSRAM, source age/accuracy/anchor/coverage and UI/touch diagnostics. No coordinate/payload/identity logs. Actual board baseline/rate/resource measurements remain bench acceptance. |
| NAV-F1 | 7Semi user identity plus official maker pinout establishes RGB GPIO48. The vehicle profile sends an application-start zero frame with hardware RMT, then holds low; pin ownership is compile-time guarded. No status colors/blinks. Physical reset/power-cycle timing still needs observation; no ROM/hardwired-power-LED guarantee. |
| NAV-F2 | Phaseful touch/drag, independent camera, explicit Follow/Area, v1 three/v2 six zoom levels, physical Pan controls, retained geographic marker, lifecycle/Settings/warning precedence. No assumed multi-touch/pinch. |
| NAV-F3 | Web-Mercator extent4096 semantic cells, source/full-content identity, bounded labels/geometry, twelve complete raw objects+staging, two shared render snapshots, overview/current detail windows and route revisions. Combined map buffers966,712 bytes; allocation failure disables v2 cleanly. |
| NAV-F4 | Authenticated c019/c01a/c01b; four pending DATA controls with downstream-only cumulative ACK/derived byte credit; latest LOCATION/PROGRESS lanes; incremental SHA/CRC/bounds verification; identical retry/conflict/gap/cancel/Stop/reset rules. Preserved R-safe UART isolation and critical priority; no giant queue, speculative ACK or baud increase. |
| NAV-F5 | Coalesced missing visible cells/HAVE/halo requests, periodic unfulfilled retries, full-hash reuse, route-window requests near/outside active range, atomic revision+minimum-context activation, monotonic revision rejection, quality-bearing source/coverage/guidance and offline cached browsing. Approximate and previously eligible last-known markers remain visibly distinct. |
| NAV-F6 | Exact implemented protocol, static normative positive/negative examples, bounded regional converter, explicit already-paired desktop sender and full host integration preview. [Commands/evidence/limits](development.md#phase-2-firmware-integration) distinguish implementation from real-radio/GPS/panel acceptance. |

The previous cache-proof firmware gate is superseded by the user's explicit firmware-first instruction. Firmware now accepts the documented compact cell schema without assuming any MapLibre cache internals. Android must prove its canonical source/cache/renderer bridge, encode those cells/routes and implement the scheduler/requests against the frozen channel. That remaining phone work does not imply an unimplemented firmware endpoint. Physical BLE speed, actual board touch/LED behavior and locked-screen Android continuity are not claimed by host rendering or builds.

## NAV-F0 — Evidence before optimization

Preserve existing instrument/pairing/clock behavior and collect a comparable 30,066-byte transfer at actual MTU 96 and negotiated≥247 where available. Add bounded numeric diagnostic hooks during implementation, not private packet logging:

| Stage | Measure |
| --- | --- |
| Android sender | Requested/actual MTU, payload size, ATT dispatch→callback, waiting for display ACK, pacing/backoff and retransmitted bytes |
| Vehicle ingress | Accepted/queued/rejected counts, high-water and age of each bounded queue |
| UART TX/RX | Encoded bytes/sec vs application bytes/sec, fragment counts, pump idle time, max critical-frame delay, framing/CRC drops |
| Display | Accepted offsets, ACK dispatch gap, CRC/SHA/decode/publication time, heap/min/largest internal block, PSRAM reservations and stack high-water |
| Existing functions | Max acquisition/source gap, metadata/health/return-control delay, warning/frame/scanout delay vs no-map baseline |
| Position | Precise/coarse observation, reported accuracy metres, monotonic source age, anchor RTT/age/uncertainty, coverage state and explicit rejection reason |

Never log actual coordinates, route/destination, payloads, pairing codes, keys or phone identity. Code currently requests MTU 247 but actual callback is authoritative; do not assume a slow run used 247. Record both board revisions/images, app commit/APK, actual phone/OS and cache state with the run.

Original v1 baseline costs (before this increment): c017 one outstanding display-ACK control; chunk payload up to 200 bytes/53 at MTU 247/96; vehicle sends at most one map fragment per 20 ms sampling iteration; sender delays after each accepted DATA. The implemented TX worker now drains fragments independently of that acquisition loop, and v2 pipelines four downstream-pending DATA commands. The4,096 B/s encoded cap remains; the old v1 APK still has its original sender pacing. Measure actual before/after speed and critical gaps. A Wi-Fi ingress change does not remove the downstream UART cost.

## NAV-F1 — Verified LED-off board profile

Identify the illuminated green component using board model/revision and schematic/marking/bench observation. The original vehicle source had no LED control; the implemented board adapter/profile now performs startup OFF on the verified 7Semi pin. Record `not controllable / simple GPIO / addressable`, verified pin/protocol/active level and power-cycle latch behavior. Do not assume a common DevKit GPIO or repurpose any acquisition/UART/USB pin.

For a controllable LED: minimal board adapter sets OFF early and holds it off across BLE connection, transfer and failures. Include pin-ownership guards. It must not add a new status-light feature or alter external instrument tell-tales. For a power-wired LED: document the hardware limit; no firmware-only promise or physical modification without user authorization. Validate ROM/reset/boot behavior separately from application control.

## NAV-F2 — Camera and touch independent of radio

Affected boundaries: `CrowPanel` touch adapter → display application camera/input reducer → immutable render snapshot → MapPanel draw owner. The original release-only adapter has been replaced by bounded `readTouch()` press/move/release/cancel observations; dragging is handled in the application reducer.

- Expose bounded press/move/release/cancel observations, measured contact coordinates and touch-read errors; no hardware polarity/register assumptions beyond the verified driver.
- Single owner maintains geographic camera center, zoom and Follow/Browse independently of pose. A drag selects Browse, applies screen-to-world deltas and cancels on touch loss; incoming GPS never snaps it back. Follow is explicit. Preserve world/camera wrap and tile seams in the shared projection model.
- Restrict map gestures to its content bounds and use a small movement threshold. Header, warning/instrument row, footer and Settings retain their existing hit/semantic behavior. Do not feed a map drag to the current bench demo toggle.
- Implement +/− and bounded zoom first. Pinch is gated on verified multi-contact support; no assumed GT 911 capability is enough without adapter evidence. Keep physical focus/select/Back and an explicit directional Pan mode; no invented holds/combos from click-only telemetry.
- At first use the existing complete phase 1 scene to inspect camera behavior; this isolates touch/camera correctness from the future bulk protocol. No firmware-only alteration of v1 location truth is implied.

Manual preview/panel cases: dot remains fixed in geographic space while camera moves; dot may leave viewport; Follow restores it; current route/pose still update during Browse; zoom/Back/Settings/lifecycle/warnings and both themes/layouts remain usable. Cold/no fix and cached map still browse.

## NAV-F3 — Shared map/route model and memory gate

Replace a single-region assumption with a versioned cell identity/projection, content identity, bounded interest queue and route revision. Model/protocol code remains free of LVGL/SDK/RTOS.

Implemented object responsibilities (exact bytes remain in the protocol authority):

| Object | Required content |
| --- | --- |
| Source/style descriptor | Schema version, dataset/snapshot/style/attribution IDs, supported geometry/label/LOD profiles |
| Cell descriptor | Source z/x/y, detail/subdivision, bounds/projection identity, content hash, geometry/label counts, encoded/decoded ceilings |
| Cell blob | Clipped/simplified road polylines + bounded names/classes; optional water/land-use geometry only under advertised capability and measured draw budget |
| Route overview | Route ID/revision/profile, simplified full-route geometry with explicit fidelity/limits, endpoints and estimates/quality |
| Route window | Current detailed route geometry/maneuvers, along-route range and revision; split chunks rather than truncate long polylines |
| Position/progress | Real quality-bearing fix and age/anchor context; separate route progress/matching/guidance quality, not camera coordinates |

Candidate tile-local extent 4096 with Web-Mercator z/x/y requires exact transform/wrap/seam/lat-limit definitions and language-neutral examples. The bytes are frozen firmware-first. The mobile source/cache spike still must prove extraction and encode this independently specified cell geometry. Use regional fixture data for deterministic projection inspection, not synthetic physical GPS.

Start from≤32 KiB encoded object/cell,≤4 queued interest requests and≤1 MiB total map/route arena including active/staging/decoded working sets. **These are bounded implementation ceilings**, not observed board heap/FPS evidence. Include LVGL/fonts/framebuffers/queues and current v1 reservations in the accounting; share/reuse or release arenas safely when changing protocol mode, never reserve both maxima blindly. Vehicle buffers fragments/credits only, not a full route/city. Failure advertises lower capability/unavailable maps while instrumentation remains operational.

Cache eviction never touches visible/pinned/current-route objects or an object being rendered; publish through explicit owner-thread lifetime/retirement. Validate each complete cell independently before publication; a partial cell is never visible. Keep compatible complete old cells on stalled/rejected replacements. A route revision publishes with its minimum matching route/current-area context, not new instructions over old geometry.

## NAV-F4 — Versioned channel and prioritized pipeline

The exact extension is implemented in the existing wire authority before Android transfer work. Contract requirements implemented:

- New discoverable/versioned endpoint set, backward compatibility, strict reserved fields and operation sizes; c011–c015 and v1 remain intact.
- Vehicle/display boot + current secured BLE token + navigation session + route revision + object/content identity + transfer sequence/offset. Route revision and independent camera interest are not one reused generation counter.
- Explicit byte/object credits, negotiated window 1..4, cumulative acceptance/missing-range ACK, duplicate/conflicting/out-of-order handling and credit replenishment only from downstream capacity. A BLE response is not a display commit.
- HAVE/NEED by full content identity, per-cell bounded hash/geometry verification, staged activation, object cancellation, route replacement, stalled-transfer timeout and reconnect/reset/resume rules. No CRC-only cross-session content reuse.
- v2 source observations may carry labelled low-accuracy coordinates; permission, accuracy, age and coverage stay separate. Keep live 2 s/end-to-end monotonic gating and wrong-session rejection; never silently change v1's unknown-field rules.
- Safety of stream framing under corruption: map bytes cannot impersonate legacy input/pairing/metadata/control frames. Preserve the current central demultiplexer; optimize encoding only with a clear resynchronization/injection analysis.

Move fragment draining out of the 50 Hz acquisition loop into the single TX owner with bounded credit/pacing. Priorities: critical acquisition/status/return actions, latest location/time/navigation control, current viewport/route object, halo, remote browse/prefetch. Only a bounded low-priority fragment may precede a critical record. Size every queue and expose backpressure; no silent drops, callbacks that wait for UART, or giant DMA/FIFO backlog.

Retain 115200/current pins and the bulk ceiling first. Target a≤5 s prepared bootstrap≤8 KiB and the global plan's30 KB benchmarks; these are measured exit gates, not source-code claims. Raise rate/baud or add Wi-Fi only under a separate reviewed measured decision. Keep authenticated SC pairing, bond NVS, c012 cadence, telemetry quality, freshness, warnings and lifecycle unchanged.

## NAV-F5 — Working-set interaction and navigation presentation

Display publishes a bounded camera-interest/HAVE/NEED request for visible missing cells + halo. Coalesce camera generations; obsolete pan requests cannot flood the phone or evict needed cells. When the phone is unavailable, pan/zoom cached areas and show missing detail, not fabricated roads. Recenter can request the current-position area independently of the phone's own camera.

Draw surrounding roads and route together once complete cells are ready. Show coverage, source quality, transfer and link independently. Add neutral/hollow/gray-last-known marker styles from the global plan without presenting an approximate fix as confident guidance. Use actual accuracy bounds; no pointer anchored at screen center during Browse. Phone-provided maneuver/progress estimates remain quality-bearing and cannot populate vehicle speed/odometer fields.

The full route corridor stays on the phone. Display keeps an overview plus current detailed window/halo and handles remote browse requests through the phone cache, not an entire arbitrary route in RAM. No NVS writes for moving map/GPS state or assumed SD storage. Cold display reset reloads current area first; new content/session checks precede reuse.

## NAV-F6 — Firmware handoff to Android

Build/package board targets through existing scripts; preserve all flash/backup/partition guards and physical wiring. Supply implemented version/UUIDs, normative examples, actual advertised limits, source dataset/fixture hashes, observed throughput/critical gaps, memory/stack/render results and LED/touch facts. A paired desktop sender may exercise the same authenticated path; no secret USB injection or auto-approval backdoor.

User bench checks cover old/new app compatibility, MTU 96/247, source/coverage independent states, partial/malformed/duplicate/missing cells, cancellation, reroute revision swap, rapid camera changes, link/reset/reconnect and known/unknown resources. Inspect 800×480 Street/Track and both themes with warnings/lifecycle. No automated suites are created/run. Mark live Android GPS/pocket acceptance pending until the app integration is installed by the user.
