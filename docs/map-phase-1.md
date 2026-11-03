# Phase 1 map + current-location integration handoff

Updated: 2026-11-03. **Firmware implemented/build and host-preview verified; physical acceptance and Android integration pending.** User-requested handoff for firmware-first, then Android work. Baseline: commit `3710ed1` (real central-Kolkata LVGL host map) and Android service integration `ede6905`. The v1 wire extension is implemented and specified in [the protocol authority](../mobile/docs/protocol.md#phase-1-map-extension--implementation-specification); existing c011–c015/UART contracts remain unchanged. This document owns workflow, rendering behavior and acceptance; the protocol guide owns bytes/IDs/limits. Do not treat the host `.r15map` text file as a firmware/phone wire format.

## 1. Outcome and scope

The rider connects the already-paired Android phone, deliberately selects **Share map & location** while the app is visible, grants location access, and sees actual surrounding roads on the cluster with a current position marker. The marker follows real phone fixes while the screen is locked. The complete map viewport has roads/names and normal instruments/warnings; it is not a route line on an empty background. Map activation and position freshness are independent.

Phase 1 is a **north-up, 2D, surrounding-road map with current position**. Search, destination selection, route calculation, turn instructions, arrival/ETA, traffic, heading-up rotation, global downloads, Wi-Fi provisioning, iOS and bike-speed substitution are out of scope. The prior sample route/maneuver/ETA must never appear in this real mode. Do not derive phone GPS position or direction from vehicle discrete-input telemetry (c014), the saved bond, phone name/time or the Mac preview.

Initial physical acceptance uses the existing central-Kolkata region: south/west/north/east `22.546,88.337,22.574,88.365`. **This is not citywide coverage.** The current location must lie inside the supplied region to show a map-relative marker. The architecture accepts another explicitly prepared region with the same bounded format; outside available coverage, report that fact rather than stretching the map or inventing roads. Preparation of larger/additional regions is explicit and must pass the same capacity check. Full map means a populated viewport within an available region, not an unlimited world database.

## 2. Chosen map stack and ownership

| Part | Phase 1 decision |
| --- | --- |
| Road data | OpenStreetMap-derived regional geometry, including provenance/ODbL notices; start from the already-downloaded Kolkata extract |
| Display renderer | Existing LVGL `MapPanel`, extended for real position quality/accuracy, immutable active scenes and bounded staging; no raster screenshot streaming |
| Phone map | MapLibre Flutter (`maplibre_gl`), local GeoJSON road layers generated from the same regional source; a local style/background and selected road-name layers; no hosted basemap required for this phase |
| Phone location | Native Android Fused Location Provider in the foreground service; `Location.elapsedRealtimeNanos` for sample time, never wall-clock age |
| Phone → vehicle | New authenticated BLE c016–c018 extension through the existing single GATT owner; current pairing/security is retained |
| Vehicle → display | New bounded map tunnel alongside existing 115200 8N1 UART; existing physical wiring, MCU responsibilities and legacy frame bytes are preserved |
| Storage | Region/provenance cached on phone; active/staging maps in display RAM only; no map/GPS writes to NVS, no assumed SD card/extra flash capacity |

MapLibre is a phone renderer, not a routing provider or an ESP32 library. Google Maps/Routes content, Geoapify tiles and public OSM raster tiles are **not** sources for this phase. No Google Maps Platform/Geoapify billing key is needed for regional OSM geometry + native location. Preserve visible `© OpenStreetMap contributors` and an accessible `https://www.openstreetmap.org/copyright`/ODbL notice on both screens; distribute provenance/licence with any distributed derived map package. Map software and map-data licensing remain separate.

Pin a compatible `maplibre_gl`/native SDK and Google Play services location version during Android implementation, not an unreviewed floating latest dependency. Use a local style and bundled/licensed glyph resources for labels, with no hidden demo tile/glyph URL dependency. Confirm offline road/name rendering in airplane mode with Bluetooth re-enabled. Fused Location Provider needs functioning Google Play services; if unavailable, show location-provider unsupported rather than false GPS readiness. A non-Play-services Android provider is a later explicit adapter. References: [MapLibre Flutter GL](https://github.com/maplibre/flutter-maplibre-gl), [GeoJSON style sources](https://maplibre.org/maplibre-style-spec/sources/#geojson), [FusedLocationProviderClient](https://developers.google.com/android/reference/com/google/android/gms/location/FusedLocationProviderClient).

The current Overpass importer is a one-off development data-preparation tool. It must **not** become the app's per-location/per-pan backend. Public Overpass explicitly discourages relying on its public instances as an app backend; public OSM raster tiles prohibit offline bulk prefetch. Ship/import a prepared regional package for phase 1; any future hosted provider, automatic region download service or citywide extract needs a separate coverage/cache/licensing/bandwidth review. [Overpass service policy](https://dev.overpass-api.de/overpass-doc/en/preface/commons.html), [OSM tile policy](https://operations.osmfoundation.org/policies/tiles/), [OSM copyright](https://www.openstreetmap.org/copyright).

The committed host snapshot has 1,435 road chunks / 4,494 points. Under the implemented binary representation it is **30,066 bytes**, generated and SHA-checked through the host firmware preview, not physically transferred/bench-verified. Preserve the source timestamp/hash in [the development evidence](development.md#real-kolkata-openstreetmap-roads). Local equirectangular projection uses radius `6371008.8m` and `cos(originLatitude)` exactly as the importer; extent is limited by the wire format. This is a small-region map projection, not geodetic/sensor calibration. Both phone preview and cluster must use the package origin/bounds and identical integer-metre rounding.

### Responsibilities

- **Phone:** permissions/disclosure, current fix observations, region selection/preparation/cache, package/CRC/SHA validation, serialized BLE sending, end-to-end progress, visible sharing intent and Stop. Android, not a Flutter page, owns ongoing GPS/connection work.
- **Vehicle:** authenticate the current phone, reject malformed/wrong-session/oversized traffic, advertise the actual downstream capability, perform bounded forwarding/flow control and prioritize acquisition/legacy input/status/metadata. It does not render, route, acquire phone GPS or buffer a whole map.
- **Display application:** own region staging/validation/activation, end-to-end ACK/progress, location time anchor/expiry, coordinate projection and immutable presentation snapshots. All LVGL access remains on the existing owner; widgets never parse BLE/UART or read storage.
- **LVGL component:** render the active immutable region and quality-bearing position; preserve instrument/warning/lifecycle behavior. No network, decoding, SHA calculation, per-frame scene copies or map loading inside draw callbacks.

## 3. User flow and lifecycle

1. Pair/reconnect through the existing explicit numeric-comparison/saved-bond flow. Old firmware remains connected with **Map sharing unsupported**; never show a map-ready flag from a bond alone.
2. The app negotiates MTU247 after the secured current connection is ready (actual callback value is authoritative), reads c016, discovers c018 and subscribes/reads status in the existing serialized owner. MTU <96 means phase-1 map sharing unsupported; existing metadata/telemetry stay functional.
3. On visible **Share map & location**, explain location use; request coarse/fine together and handle precise/approximate/denied/permanently denied/GPS-disabled states. Grant denial stops map-location work, not existing BLE/phone metadata. Add main-manifest INTERNET only if the chosen app download path actually needs it; regional assets do not need an always-online map service.
4. Start/promote foreground location work while visible with current access, using `location` plus the existing `connectedDevice` type as applicable. Declare `FOREGROUND_SERVICE_LOCATION`, remove the old API-30-only limit from real location access, and preserve notification permission behavior. Existing `connectedDevice` service ownership alone is insufficient. [Android location FGS requirements](https://developer.android.com/develop/background-work/services/fgs/service-types#location).
5. Acquire fresh current fixes. A cached/last-known location may center the **phone's labelled preview**, but cannot be labelled live on the cluster until it passes monotonic age/accuracy checks. The ongoing provider, not `getLastLocation()` alone, supplies the stream. [Android location guidance](https://developer.android.com/develop/sensors-and-location/location/retrieve-current).
6. Validate/cache the regional package, verify it covers the fix, establish end-to-end timing, and transfer the map. Location messages may run during transfer; there is still no marker on an incomplete first map. On display-confirmed ACTIVE, reproject the newest eligible fix into the committed region and show the marker.
7. **Stop map sharing** cancels GPS subscriptions, map preparation/transfers/timing and pending location writes, sends STOP to the display, and retains the ordinary BLE metadata connection. **Disconnect/notification Stop sync** stops both sharing and BLE, retaining bonds. No resume from a forgotten selection or old transfer.
8. Background/locked screen, Flutter engine detachment and Activity recreation must not stop an active authorized sharing intent. Do not retain an Activity/EventSink/MethodChannel result in service-owned work. GPS/notification/platform observers are service/application scoped.

Maintain separate `clusterSyncIntent`, `mapShareIntent`, actual GATT, actual foreground-service types, active map and position quality. Automatic link loss hides the cluster marker; it does not mean user Stop. While map sharing is deliberately active, keep the native location owner alive across eligible saved-peer reconnects, with only a latest-fix slot; never replay a history. Radio/permission loss suspends the affected capability, with a truthful notification/action to restore it. Explicit Disconnect/Stop always cancels recovery. Adapt the current service's unconditional `stopSync()` radio-loss path so an active location intent is not accidentally coupled to an Activity/BLE callback.

Retain the current `START_NOT_STICKY` policy for phase 1: no boot receiver/hidden restart, no blanket battery exemption/permanent wake lock. After process death, Android Active apps Stop, Settings force-stop or reboot, reopening the app is required; **map/location sharing requires a new visible Start action**, even if ordinary saved-peer BLE reconnect is allowed. Do not restore a persisted “live” fix. Persist selection/preferences/region files only, not GPS history or a falsely connected flag.

## 4. Location semantics

| Observation | Required behavior |
| --- | --- |
| Cadence | Request high-accuracy updates around 500ms; send at most 2 location packets/second, coalescing to the newest sample; no guaranteed OS/GNSS cadence claim |
| Timestamp | `Location.elapsedRealtimeNanos / 1,000,000` and `SystemClock.elapsedRealtime()` are the same phone monotonic domain including sleep; UTC remains the separate c012 clock contract |
| Live eligibility | Valid lat/lon, provider fix not from the future, source not mock, precise access, horizontal accuracy ≤50m, and conservative end-to-end sample age <2,000ms |
| Poor accuracy/approximate grant | Show **Location accuracy low / Precise location needed**; retain roads, suppress the precise-position arrow; no fabricated 0m accuracy |
| Heading | Use OS course-over-ground only with `hasBearing`, valid bearing accuracy ≤45°, and measured speed ≥2m/s; show a direction arrow then. Otherwise show a direction-neutral position dot, not north as a fabricated heading |
| Stale/no fix | At 2,000ms total sample age or 2,000ms with no valid newer packet, hide the live marker/heading and show **Location stale / Waiting for location**; retained map does not expire from GPS loss |
| Invalid/mock/future/duplicate/old generation | Reject without refreshing freshness; non-Available updates clear the marker promptly; mock fixtures stay visibly SIMULATED and do not count as physical acceptance |
| Coverage | Current fix outside committed geographic bounds: no marker projected against the wrong map; show **Outside downloaded area** and tell the phone which capability/coverage is absent |

Latitude/longitude are sent as signed E7 degrees, not metre coordinates or text. Accuracy is centimetres; bearing is centidegrees clockwise from true north. Phone GPS speed is only a heading-quality aid in phase 1 and never replaces bike speed/odometer/RPM. Do not use a rotation-vector compass as a hidden substitute when the phone is in a pocket. Optional accuracy-ring rendering must use the actual accuracy bound and cannot make an ineligible fix look precise.

Display freshness uses a phone↔display timed exchange, not independent receipt timeouts at each hop. Refresh the anchor every 25 seconds; after 30 seconds without an accepted anchor, the marker is timing-uncertain and hidden. The phone's actual dispatch/callback RTT must be ≤500ms. The exact exchange/age formula is in the protocol extension. Initial drift allowance is provisional (2ms quantization plus 1000ppm and enlarged observed anchor uncertainty), and must be measured/accepted on hardware; lack of confidence never displays a false live pointer. No wall-clock subtraction.

## 5. Map transfer and display behavior

The phone sends **a compact binary complete regional snapshot**, not PNG/JPEG/full-screen images, raw Overpass JSON, a native C++ struct or the host text file. The package includes geographic origin/bounds, source time, road classes/names/points and integrity fields. Route fields are absent. Maximum package 65,536 bytes, up to 2,048 eight-point road chunks, ≤23 printable ASCII name bytes; the display advertises its actual lower resource limits if needed. Overflow is an explicit **Area too detailed** rejection, never silent road dropping. Chunk splitting is not removal of road geometry; service/path visibility may change with zoom as in the host renderer.

One transfer/generation and one outstanding map operation are permitted. BEGIN declares length/hash; DATA carries an explicit offset; COMMIT requests full validation; CANCEL abandons staging. Only display-originated c018 progress advances the phone's progress. A successful Android write/ATT response means at most accepted by the vehicle—it is **not** proof of UART delivery, validation or visibility. Native c012 writes, c014 notifications/baseline maintenance and map LOCATION/time-control operations remain serialized/prioritized; map DATA yields to them. Byte-identical retries are idempotent; overlapping/conflicting data is rejected.

| Display condition | What the rider sees and what data may be used |
| --- | --- |
| No active map; first BEGIN/DATA | Existing instrument strip/tell-tales and a neutral **Loading map · N%** area; no fictional roads, no pointer on an empty/unverified map |
| Old complete map; replacement receiving | Keep old complete roads visible with **Updating map · N%**. The marker can use the old map only if it is fresh, timed and inside old coverage. No fragments of the new region are drawn |
| 100% bytes, not validated | **Checking map…**; keep old map/neutral loading state. Never call this Ready/100% visible |
| Valid COMMIT, allocation/parse/hash good | Publish the immutable new scene at one LVGL-owner update; reproject the newest fix using the new origin, not old metre coordinates; then show **Map ready** if coverage/time/fix qualify |
| Malformed/chunk CRC/hash/count/name/bounds failure | Reject/cancel staging; retain old complete map, report **Map update failed · Retry**; no partial activation or bond erasure |
| Data stalls | After 15s of no newly accepted map bytes, discard staging and show retryable failure. Identical retries/invalid packets do not extend this inactivity deadline |
| Phone/BLE/UART loss | Abort staging, hide the live pointer immediately on known link loss (otherwise monotonic expiry), retain complete roads as cached data with disconnected status |
| Display restart | RAM map is absent; start empty/Loading and require new capability/timing/full map. Old session chunks/fixes cannot restore it |
| Vehicle restart | Invalidate session/timing/staging; display may retain its static RAM map, but no marker until the new secured session/anchor/fix is validated |
| Stop map sharing | Discard staging and GPS/heading; retain complete cached map for manual browsing with **Location sharing stopped**; no automatic sharing restart |
| Boot/shutdown/urgent warnings | Existing 2.00s lifecycle and warning precedence remain; transfers cannot hold power on or delay warnings. No last-minute flash/NVS write is required |

Keep Settings accessible under the existing prototype rule. Navigation opens the map page; Back returns to the previous page without ending the user's sharing intent. Provide semantic Follow/Recenter, Overview and bounded Zoom controls through physical navigation as well as optional touch. The compact gear/speed/clock strip and complete tell-tale row stay visible; preserve original Street/Track dashboards exactly outside the map page. Cached roads/old source timestamp must never be labelled real-time traffic. Attribution stays visible when map data is shown.

### RAM, fairness and timeout defaults

- **Display:** preflight active/staging space outside the UI stack; initial ceiling is two 64KiB binary slots plus two decoded scenes (524,824 bytes, about 513KiB at the current host maximum), excluding LVGL/framebuffers/fonts/queues. Do not instantiate or repeatedly copy the 196,876-byte host Scene on an ESP32 task stack. Adapt MapPanel to borrow an immutable active handle with explicit owner-thread lifetime/retirement. Full allocation must succeed before BEGIN is accepted; otherwise map capability is unavailable, while instrumentation still works.
- **Vehicle:** at most one pending map command and bounded fragment/reassembly slots; no full regional package. BLE callbacks enqueue/copy bounded messages and return; no waits for UART/display. Acquisition task never blocks behind map TX, a hash, rendering or filesystem work.
- **UART:** keep 115200/8N1 and current pins; add a single serialized TX owner with a bounded nonblocking priority queue. Current `uart_tx_chars()` one-shot behavior cannot carry large arbitrary map records. Existing 36-byte/20ms input frames, status/metadata and return controls retain priority; then LOCATION/time/control; map DATA last. Cap map bulk to 4,096 **encoded UART bytes/sec**, with bounded burst and at most one low-priority fragment ahead of critical traffic. Measure actual legacy/headroom/latency; do not infer spare FIFO capacity from total wire rate.
- **Phone:** request MTU 247, accept actual MTU ≥96 and compute chunks from it; one map write/command in flight, ≤15 DATA writes/sec and the display's one-credit ACK. End-to-end map-operation ACK timeout 2s; at most three byte-identical retries with 0.5/1/2s backoff, then fail only map sharing/transfer. The existing ATT missing-callback rule still closes the GATT before any recovery, never overlapping procedures.
- **Reconnection:** discard half maps/time anchors and resend from BEGIN after security/capability/subscription is restored. No cross-connection byte-offset resume. An identical complete map may be reused only when the display compares the full BEGIN SHA-256 against its already-validated active content and confirms ACTIVE in the new context; CRC or region ID alone is insufficient.

The current 30,066-byte Kolkata package has a nominal UART lower bound of about 7.4s at 4,096 bytes/sec **before** framing/ACK/priority/validation overhead. Target ≤60s initial activation at MTU≥247, ≤120s at MTU96 on the bench, with no starvation; these are acceptance targets, not measured promises. During this delay GPS/metadata keep operating and UI progress is honest. New-region transfer latency must be measured before adding automatic moving-area prefetch; phase 1 cannot promise seamless global riding coverage from a single static region.

## 6. Firmware-first implementation order

### F-MAP-0 — Resource and coexistence gate

Verify actual vehicle chip/flash/RAM, display PSRAM and current partition/buffer configuration. Record free/minimum/largest internal heap, PSRAM and task-stack high-water, baseline acquisition/metadata/UI timing and actual UART loads. Validate independent return UART and secure saved reconnect. Agree the advertised lower limits before allocation. If double staging/rendering cannot coexist with instrumentation, reduce advertised area capacity explicitly and fail unsupported regions; do not change hardware facts or silently flash a new partition layout. Existing B1/B2 checks remain required; a map prototype is not their replacement.

### F-MAP-1 — Shared contract and host producer

Implement the versioned package/command/status/time/fix codecs and bounded escaped UART tunnel from the protocol guide; C++/Dart consume the same byte definitions, never ABI structs. Produce language-neutral positive/negative normative vectors for later app use; these are non-executable fixtures, not restored automated suites. Extend the existing OSM importer to produce the binary package/manifest/GeoJSON from the cached snapshot with deterministic SHA-256. Inspect round trips, length/padding/bounds/CRC/wrap and malformed captures manually. Freeze the implemented contract/version and record any reviewed correction in the protocol guide before Android starts.

### F-MAP-2 — Display integration

Implement RAM active/staging MapStore, incremental bounded parsing/hash validation outside rendering, begin/data/commit/cancel/status, boot/session gating and time-anchor/position-quality application logic. Link MapPanel/copyright asset into the native display page, use immutable active scenes and add real-map settings/non-touch controls plus loading/failure/coverage states. LVGL remains single-owner; decoder/tasks cannot mutate objects. Build/inspect 800 × 480 themes/layouts and first-map/replacement/failure/stale/unsupported/lifecycle states. Bench with explicitly labelled development input, never an unmarked fake GPS source.

### F-MAP-3 — Vehicle BLE/UART bridge

Add c016–c018, the downstream-capability exchange, authenticated map ingress and display-originated status forwarding. Refactor the present independent byte-by-byte UART consumers through bounded demultiplexing/escaping; map payload bytes cannot masquerade as R15G/R15P/R15N/R15M/control frames. Update the existing small UART RX rings/`waiting>256` flush assumptions for bounded fragments without unbounded draining. Use a bounded TX owner/ring, never interleave bytes of two records or block GPIO sampling in a BLE callback. Keep the existing enrollment timing, c012 cadence/security, c014/health semantics and old-app compatibility. Advertise readiness only after real display support + RAM reservation are known.

### F-MAP-4 — Bench freeze/handoff

Build/package each board separately and use the existing guarded flashing workflow; never bypass backups/chip/partition guards. Use a paired desktop BLE bench sender obeying the same OS comparison/encryption and serialized contract to exercise transfers before the phone implementation, plus host/manual captures for invalid data. No unauthenticated firmware backdoor/USB injection endpoint is introduced. Verify package/image hashes and end-to-end ACK/state/load behavior; deliver the supported endpoint/version/actual limits, a binary Kolkata package + provenance/hash and observed diagnostic results to the Android developer. Mark fresh live phone GPS acceptance pending until the Android stage.

## 7. Android implementation after firmware freeze

1. Keep existing discovery/radar/enrollment and the single BleBond operation queue. Extend it with c016 reads, c017 control/data/location writes, c018 subscription/status reads and ordered MTU negotiation. c013–c015 are not expanded or repurposed; old strict decoders keep working. GATT/service-change/reconnect clears old refs and map timing/generations.
2. Add native FusedLocationProvider lifecycle and separate sharing intent to ClusterSyncService. Keep current Android permissions/notification/Stop behavior and actual foreground-type requirements. Observe location timestamps/accuracy/bearing/mock state natively, maintain a single latest sample and publish typed immutable observations to Dart. Do not place continuous GPS/navigation ownership in Flutter pages/geolocator widget listeners.
3. Package/import the same licensed regional OSM data and binary manifest; preflight full SHA/counts/bounds/size against c016. Phone MapLibre displays local GeoJSON from this data with attribution and current-location quality; it is not proof that the cluster has activated the map. Add Connect/Share/Stop sharing states and actual display-ACK progress/active coverage.
4. Send time handshake, BEGIN/DATA/COMMIT and latest eligible fixes through the native priority scheduler. Keep metadata/telemetry alive during transfer and locks. Handle unknown endpoints, low MTU, no credit, HASH/BOUNDS/RESOURCE errors, timeouts, cancel, old generation and cold boots truthfully. Do not acquire new map regions via repeated public Overpass calls.
5. Use GitHub Actions for actual Android APK builds/install on the Mac. Optional existing diagnostic flag may enable bounded numeric counters; never log coordinates/bearings/raw packets/keys/user paths. No default analytics, route history or persisted last-known “live” marker. No automated tests/suites until the user resumes that stage.

## 8. Phase 1 acceptance and evidence

| Gate | Required manual/physical evidence |
| --- | --- |
| Packaging/compatibility | Phone APK + both firmware builds/image hashes; c011–c015 unchanged; old app/new firmware and new app/old firmware degrade correctly; actual MTU96/247 and missing callback/CCCD/error behavior |
| Real-map identity | Kolkata package source timestamp/query/hash/licence match on host/phone/display; correct roads/labels in both themes; no synthetic route/maneuver/ETA; no road loss hidden by capacity truncation |
| Position truth | Outdoors within supplied region, compare real reported location to recognizable roads; stationary dot vs qualified movement bearing; check 0°/359.99°/unknown bearing, zero/unknown accuracy, approximate/denied/revoked permission, GPS off, fix age 1999/2000ms, lost/duplicate/backward/wrapped packets and timing-anchor loss |
| Incomplete transfer | First-map 0/middle/all-bytes-before-COMMIT; replacement leaves old map intact; invalid CRC/hash/length/class/point/name/bounds; duplicate/conflicting/out-of-order chunks; cancel; stalled15s; display/vehicle resets; BLE/UART loss; retry without partial activation or erased trust |
| Session/time | Fresh security/capability/subscription after reconnect; old BLE token/display boot/vehicle boot/map generation rejected; measured RTT≤500ms anchor, slow/delayed callbacks stale; no burst of buffered old fixes revives a marker; received timestamp alone is insufficient |
| UI/ownership | Map follows only qualified fixes, off-coverage is explicit; all warnings/settings/physical Back/Follow/Zoom remain usable; two-second lifecycle and urgent-warning interruption; no LVGL calls off owner and no UI-stack map copies |
| Sustained pocket operation | 60 minutes locked and USB/debugger unplugged with c012 + c014 + map/fix activity; at least one map replacement and radio/range recovery; explicit Stop sharing and Stop sync verified; Android Activity recreation/recents/process-stop policies recorded |
| Resources/fairness | Actual transfer time/progress, UART encoded bulk rate, max acquisition/metadata/host/UI gaps, queue high-water/drops, minimum/largest heaps, PSRAM and stacks; no unexplained growth or instrumentation deadline regression; compare against recorded no-map baseline |

Record Android/device/battery settings/permissions, revisions, build configuration, elapsed duration, power/USB state, map/file/firmware hashes and bounded numeric diagnostics. Manual bench runs precede any bike observation. Current firmware evidence includes both board builds and host simulation of the actual package/tunnel/store/page; see [development evidence](development.md#phase-1-map-firmware-integration). **No physical map-transfer or live phone-location acceptance has been performed.** Completion requires the full physical phase-1 chain, not a screenshot, a successful ATT write or a saved bond.

## 9. Firmware implementation handoff status

F-MAP-1/2/3 source implementation and affected board builds are complete; the developer preview exercises the real portable protocol/tunnel/store and Dashboard map page. F-MAP-0 physical measurements and F-MAP-4 paired-board transfer/load acceptance remain open because no board is connected in this session. No flash or hardware measurement is implied by a successful build. Use the [development commands/evidence](development.md#phase-1-map-firmware-integration) and record the measured gates before declaring hardware ready.

Two clarifications are frozen into the first implemented v1: negotiate MTU before reading c016, and LOCATION flag bit2 with canonical Invalid/unknown coordinates signals accuracy-limited/approximate access. The cluster hides its marker and shows low accuracy; the phone explains the precise permission/accuracy cause. The wire authority includes these details. Display Trip advances the five map controls and Info selects while the map page is open; Back is the first control. Outside the map page their existing instrument behavior is unchanged. Serial focus/select/Back and touch remain available.
