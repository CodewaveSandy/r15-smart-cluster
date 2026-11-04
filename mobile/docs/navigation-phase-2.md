# Phase 2 mobile implementation handoff

Updated: 2026-11-04. **Android implementation pending; firmware v2 is now implemented and specified.** Read [the full journey, provider decisions and budgets](../../docs/navigation-phase-2.md), [firmware dependencies](../../docs/navigation-phase-2-firmware.md), mobile/AGENTS.md, current architecture/background-sync and exact protocol guides. Current phase 1 remains a bounded Kotlin/MapLibre/Kolkata prototype, not the new dynamic journey.

## NAV-M0 — Source/cache and lifecycle spike

The current app cannot acquire phone-map location until cluster sync/share is started, packages one central-Kolkata region and cannot browse independently before pairing. Replace these coupling assumptions deliberately while preserving real discovery/radar, explicit bond approval and ordinary telemetry.

The user requested firmware-first completion, so [v2 bytes are now frozen](protocol.md#phase-2-navigation-extension--implemented-firmware-contract). Before claiming mobile integration, prove on the installed target APK that the pinned native MapLibre integration and cluster converter can consume one canonical raw-MVT cache. Document the exact native resource/cache hook, network ownership, offline style/glyph/sprite handling and eviction behavior. Do not assume MapLibre Flutter exposes its native cache contents. A reviewed bounded adapter is necessary if it does not; see the main plan for the narrowly scoped loopback fallback and its security/lifecycle constraints. Do not introduce a globally exported tile server, cleartext exception or independent duplicate downloader by accident.

Use a small provider-legal area, record source/TileJSON snapshot and source zoom limits, capture cache-hit/miss/bytes counters and verify offline road/label rendering. No new account or user location is sent by this planning document. Configure provider credentials only after the user selects the recommended stack. App builds/spike APKs run **only in GitHub Actions**; user installs and checks them manually. No local Android builds/SDK installation or ad-hoc native compilation.

## NAV-M1 — Independent phone location and session intents

Evolve the existing native service to own independent visible-browse, navigation, route-sharing and BLE-sync intents. Keep one PhoneLocation/Fused Location Provider and one BleBond/ATT owner; do not add Flutter-owned location subscriptions or a second GATT. Connect is no longer a prerequisite for browsing/GPS on the phone.

- App visible + permission: request/observe real fixes and warm a bounded surrounding area. First-use denial/approximate/disabled/provider failures are explicit; manual map browsing remains available.
- No active navigation/share + app hidden: stop continuous location after a documented short grace; cached files remain and bounded immutable cache jobs can complete through the appropriate native worker. Do not equate background availability with idle continuous tracking.
- Explicit Start navigation/Share route: visible promotion to location foreground service, clear notification and independent Stop actions; current GPS survives UI/engine detachment and saved-peer radio recovery.
- Explicit Stop sharing retains phone navigation; End navigation ends route sharing/navigation; Disconnect/Stop sync ends cluster work; Stop all ends every intent. Permission/radio/process changes cannot resurrect cancelled work.
- Keep START_NOT_STICKY and no boot/hidden restart. Reopening can show a saved route as a preview, never a persisted live position/active-sharing flag.

Expose immutable location-quality, map-coverage, route, transfer, link and camera observations to Dart. Keep actual horizontal accuracy visible even when poor, with separate coverage text. A numeric developer panel can expose age/anchor/rate/rejection counters without actual coordinates, names, routes, URLs or keys. Phone speed remains a heading aid only.

## NAV-M2 — Dynamic nearby map and offline repository

Replace hardcoded runtime camera/data selection with fresh location and user camera interests, keeping the Kolkata bundle only as an explicit demo/fallback area. A file/cached viewport is not a default current location. Preserve the existing local style as fallback while wiring the selected vector source and proper attribution.

Proposed cohesive responsibilities (implement only as consumed): native tile repository/downloader and cache-job owner; source-MVT decoder/cluster encoder; native navigation session/progress owner; search/routing client; Flutter map/camera/controls. These are work packages, not a request to create empty abstraction folders.

Use app-private disk/cache metadata, one HTTP request per missing content key, bounded responses/decompression/protobuf geometry and IO/CPU workers. Apply the global 256 MiB LRU/current-area and provider download ceilings, source zoom clamping,2 parallel requests, conditional caching and latest-viewport cancellation. Decode source geometry and convert on bounded native workers, not the main/GATT thread or a Flutter draw callback. Validate content before publication; atomically finalize files/manifest references so partial downloads are never Offline-ready.

Cache style, required glyph ranges and sprite metadata/images alongside raw cells. Offline coverage reports which zoom/detail resources are complete. Revisit/pan reuses cache; zoom beyond source maximum overscales. Missing-network/quota/corrupt/disk-full states preserve usable cells and show the specific reason. No public Overpass per-pan queries, public Nominatim autocomplete or OSMF tile bulk prefetch.

The phone's rendered MapLibre viewport and the display converter must use identical source snapshot/cell identity; a separate phone tile provider and unrelated cluster regional extract cannot be called the same map. The phone may use richer cartography/Unicode while the display uses a documented bounded label/LOD profile. Licence/attribution survive offline use and derived geometry distribution.

## NAV-M3 — Search, motorcycle route preview and full corridor

Use the proposed Geoapify autocomplete/geocoding + Routing motorcycle profile. Missing key provides a setup state; no hidden default key, public key committed to Git or automatic paid upgrade. Debounce/cancel/bias/limit autocomplete; use explicit query generations so old suggestions cannot replace newer ones. Validate endpoint/result geometry/status/quota and retain provider attribution. Do not relabel `drive`/`scooter` as a motorcycle route.

Route selection creates a real phone preview, estimated distance/time and the immutable RouteManifest described in the main plan. Previewing/searching does not share location with the cluster. Share route promotes the explicit native session, pins the full planned corridor, estimates missing tiles/bytes/free disk, prioritizes the current area and continues downloading the tail while locked. Separate `phone preview`, `sharing local area`, `display ready`, `full offline-ready` and `partial/blocked`.

Apply the initial±500 m corridor/endpoint buffer and provider-bound job limits; pin the route geometry, route-band cells/LODs, style/glyph resources and manifest hashes. Never promise an arbitrary full route fits the display or the provider's offline allowance. A route above limits requires an explicit narrower/coarser choice or approved alternative service; do not silently reduce road coverage or evade provider limits by slicing jobs. Search/reroute without network remains limited: preserve the cached route and report service unavailable, not fabricated directions.

Implement progression as its own uncertainty-aware application responsibility: projection along route, remaining distance/time quality, step advancement, arrival and hysteretic off-route detection. Several consistent eligible observations precede rerouting; cancel superseded routes and rate-limit retries. No heavy spatial work in BleBond callbacks. Fresh approximate/poor-accuracy observations show a labelled hollow point but cannot drive confident snapping/turn progression. Future offline-routing engines are a separate project, not implied by offline map tiles.

## NAV-M4 — New firmware channel integration

Implement the [exact v2 contract/fixtures/limits](protocol.md#phase-2-navigation-extension--implemented-firmware-contract) now supplied by firmware: c019/c01a/c01b, twelve complete objects plus staging, four DATA commands in flight, full-SHA reuse, interest/window requests, strict route revision activation and source-quality/time rules. Existing c011–c015 and map v1 strict decoders remain unchanged; discover a separately versioned channel and truthfully downgrade on old firmware. Do not design an incompatible Android packet from this semantic plan alone.

INTEREST z/x/y are semantic display cells: clamp upstream downloads to provider/source max zoom and derive child cells from the canonical cached parent where overscaling is needed. Do not request nonexistent raw zooms or maintain a duplicate download cache. Geographic-partial cell flags remain partial coverage even after all their bytes arrive.

Integrate a single scheduler with independent current source/route-progress, object transfer and viewport-interest lanes. Honor downstream byte/object credit and negotiated pipeline window while retaining serialized ATT procedures. Measure actual payload/MTU/ACK/pacing; latest observations and metadata/telemetry outrank bulk. Have/need/content-hash negotiation avoids full resend; missing ranges only resume in the matching content/session rules. Location cadence does not depend on the user browsing Navigate.

Send a validated minimal current-area road/route working set first, then fulfill display camera/halo requests from the canonical phone cache. A display drag does not move the phone camera or reroute the journey. On-demand fetch for off-corridor display browsing is online/cache dependent and lower priority than the rider's current area. Coalesce and cancel obsolete interest generations.

Each cell is independently complete/verified, never a half-rendered blob. Route revision and tile generation are distinct. Changing route/destination invalidates old progress/instructions; activate the matching new minimum context on the display, then fill remaining cells. Source age/time anchors/boot identities and trusted saved-peer security survive neither a stale callback nor a random app-level connected flag.

## NAV-M5 — Phone experience and manual acceptance

Keep the approved Home/Phone/radar designs and expose phone map access without compulsory pairing. Navigate gets actual local point/accuracy, Free browse/Follow/area overview, real destination search/result/route preview, download/detail/offline status, Share route and precise Stop controls. Avoid false `ready` text based on ATT success. Show the display's reports separately from the phone's own fix/coverage.

Use the latest target Android devices at normal text size, light/dark and unavailable/permission states. No small-screen or 2× text checks unless requested. No automated suites/local app builds. Finish code/locks/workflow updates, use available formatting/static analysis without an Android SDK, then the user commits/pushes, installs the Actions APK and tests manually.

The physical matrix is in the main plan: no-cluster app open; permission/accuracy/coverage independently; pan/zoom/cache/offline; search/quota/key failure; route completeness/limits; fast initial display context; independent display camera; poor-fix off-route rejection; reroute revision swap; process/radio/lifecycle/explicit Stops;60 minute untethered run and measured coexistence. Capture source/APK/image/data hashes and bounded numeric counters. No current location, travel history or provider secrets in logs or the public repository.
