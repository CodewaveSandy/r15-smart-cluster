# Google Maps notification guidance — replacement plan

**Removal progress — 2026-11-05:** Custom map/navigation source, menu/tab, providers/GPS and c016–c01b are removed in the working tree. Replacement turn guidance remains unimplemented; candidate protocol IDs below remain proposals. User explicitly requested removal first.

Status: **planning only**, 2026-11-05. Requested replacement for the entire custom map/navigation implementation. No removals, firmware/app implementation, permission changes, build, flashing or commit are authorized by this document alone. The user will ask to start implementation separately. The existing uncommitted stack/UART fixes are separate work and must be preserved.

This is the proposed product and delivery authority for the replacement. [Firmware handoff](turn-by-turn-firmware.md) and [Android handoff](../mobile/docs/turn-by-turn.md) divide ownership. Earlier map Phase1/Phase2 documents become historical once migration is implemented; they are not evidence of this feature working.

## 1. Outcome and explicit boundary

Use **the installed Google Maps Android app** for destination search, route selection, routing/rerouting, GPS, maps, audio and ride completion. The companion observes eligible Google Maps navigation notifications and sends small normalized guidance snapshots through the existing authenticated phone → vehicle → display connection. LVGL presents a bounded turn card automatically on the dashboard. There is **no custom map, route search, tile downloader, route/corridor database, own GPS tracker, API key, account configuration screen, Navigate tab, full-screen navigation page or Navigation Settings tile** in the companion/cluster.

This is notification-derived guidance, not Google Navigation SDK integration, an official Google Maps data feed, Android Auto, screen mirroring or a full map projection. No Google/Geoapify/OpenFreeMap SDK/API is needed by the replacement. Normal Maps connectivity/account/application behavior remains Google Maps' responsibility; no promise of offline search, offline rerouting or provider feature parity is made. Network/audio from Google Maps stays on the phone/headset.

**Feasibility is a hard acceptance gate, not an implementation assumption.** Android documents notification access and standard extras, but does not specify which navigation fields Google Maps publishes. Your installed Maps version, device, language and notification configuration must be observed. Text mirroring may be feasible even when a confident maneuver arrow, roundabout exit, ETA or remaining distance is unavailable. No private extras/hidden API, Accessibility scraping, screenshots, OCR, resource-ID guessing or invented values will silently fill gaps. If useful navigation text is inaccessible, stop and report that result before destructive removal; the official Navigation SDK would be a separate user decision, not an automatic fallback with billing.

## 2. Rider journey

1. Open the companion and connect/pair through the existing real radar and Android numeric approval. Fix/bench-confirm the current vehicle restart regression first. Pairing, connected metadata and input telemetry must remain usable when this feature is disabled.
2. On My bike, a compact **Google Maps guidance** card provides Enable, notification-access status, connection status and Open Google Maps. Remove Where to next/Explore navigation and the Navigate bottom tab; with only My bike remaining, remove the redundant bottom tab bar. Existing connection/settings access and the approved Phone/radar artwork stay intact. This is a feature status/control card, not a new route/navigation menu or map screen.
3. First Enable explains that Android grants broad notification-listener access, while this app filters to Google Maps and keeps only transient guidance. Open the OS notification-access settings. A normal POST_NOTIFICATIONS prompt does not grant listener access. Recheck actual grant and listener connection on return; grant alone is not evidence of working callbacks.
4. Enable sharing explicitly while the app is visible and BLE sync is active. The durable preference is consent/configuration, not a persisted live ride. The service notification offers Stop guidance and Stop sync. A successful first connection can remember the user's guidance-enabled preference for subsequent already-active sync sessions; never launch hidden BLE/foreground service work merely because a Maps notification appeared.
5. Tap Open Google Maps or open it normally, choose destination/motorcycle mode where available, and start navigation **in Google Maps**. We do not select a driving profile or destination on the user's behalf. Google Maps mode support is not inferred from notification text.
6. An eligible navigation notification produces the dashboard turn card automatically. Google Maps owns recalculation; a recognized rerouting notification shows Rerouting with no old turn arrow. No map package or transfer-progress screen is involved.
7. Changes to the actual eligible notification replace the guidance. Only explicitly present, confidently recognized fields are shown. For example, Left +200 m can be displayed together only if that notification supports both; unknown distance is a dash, never0 m. ETA is optional reported text, not recomputed from our GPS/clock or an old provider route.
8. Ending Maps navigation/removing its selected notification clears the card. A short replacement grace handles the same Maps session replacing its notification; it must not become a lingering old turn. Arrival is shown only for a supported arrival observation, briefly, then normal instrument information returns.
9. Stop guidance suppresses forwarding and clears the cluster card, retaining BLE/name/battery/clock/telemetry and Google Maps navigation. A still-present notification cannot re-enable it until an explicit Enable. Stop sync disconnects BLE and clears remote guidance without stopping Google Maps. Forget retains its current explicit trust-cleanup semantics. No new control dismisses, snoozes or stops a Google Maps notification/navigation session.

## 3. Display behavior with no Navigation menu

Remove Navigation from Settings completely; reindex touch/focus/non-touch Next/Previous/Select/Back over the three remaining tiles: Appearance, Phone, Clock & Date. Clock must retain its function even when its index changes. No disabled/empty Navigation placeholder remains.

Add one auto-presented turn card to both Street/Track layouts, Midnight/Daylight. Proposed bounds: the existing lower **trip/info area**, x28..530 and y392..478, preserving the fuel/pump region to its right. Confirm exact bounds against `Geometry.h`/Metrics before freezing visuals. It temporarily replaces noncritical trip/info text only while guidance is present. Gear, tell-tales, tachometer, main speed/lap, fuel/reserve, time and warning ownership stay unchanged. No map camera, pan, zoom, Follow or gesture screen remains.

Card hierarchy: locally drawn maneuver glyph or neutral information icon; explicitly supplied next-turn distance; bounded instruction/road text; optional remaining/ETA summary and source-state indicator. Never draw Google Maps' generic app icon as a maneuver. No unverified bitmap/icon transfer is part of v1. Keep distance units explicit and unknown values visible. Source attribution label is `From Google Maps notification`, not an official certification claim.

Settings/modal pairing, critical warning/lifecycle overlays take precedence; opening Settings does not stop observation. During boot/shutdown, guidance cannot extend or interrupt the existing2.00 s lifecycle. Return to the dashboard shows only the current eligible observation; it does not replay queued turns. Trip/Info input semantics stay unchanged, even while their ordinary information rows are temporarily replaced. No required touchscreen action while riding. An inactive feature produces no permanent empty card; unavailable/stale status can show one bounded message before restoring ordinary trip/info.

## 4. Source selection, extraction and limitations

Android adapter filters **package `com.google.android.apps.maps` and the same enabled user/profile** before reading content. Package name is an allowlist for selection, not BLE authentication. Reject group summaries, ordinary traffic/review/marketing notifications and ambiguous multiple candidates. Channel ID, ongoing/category, key and standard fields can help identify candidates but no channel/resource ID is assumed stable. Selection requires the versioned parser profile proved during the feasibility stage; otherwise report Unsupported Maps notification, not navigation-ready.

Inspect standard `Notification.extras`: title, text, big text, subtext and text lines, plus field presence/type metadata. Do not inflate arbitrary RemoteViews or execute actions. Normalize control characters/whitespace, bound input to a few KiB and output to the proposed wire limits. Malformed/custom/redacted/empty content is explicitly unavailable. Do not log raw extras, addresses, destinations or road/instruction text. The user may view the actual current notification and normalized preview on the phone; any private diagnostic export is an explicit one-time choice, not logging enabled by default.

First parser profile targets **English notification instructions** (including en-IN and explicitly recognized metric/imperial units), subject to actual device observation. Supported maneuver vocabulary: Continue, Left/Right, Straight, Keep left/right, U-turn, Roundabout (exit number only when explicit), Arrive. Unknown is a real enum value. Text-only guidance is allowed; a speculative arrow is not. Preserve both action and distance validity independently. Do not infer lane guidance, street geometry, coordinates, speed, remaining distance or route mode from a text fragment. Do not interpret `left` inside a street/place name as a left-turn command.

Numeric parsing requires a recognized field/phrase/unit and locale-specific decimal rules. Bound values; retain whether a number is approximate/rounded. No distance countdown/dead reckoning between notification updates, no ETA countdown, no route matching or reroute engine in the companion. Multi-stage instructions must not bind the second maneuver to the first distance. A notification replacement is atomically selected before its normalized snapshot is published.

Display text support is explicit. The current firmware font subsets do not establish arbitrary Bengali/Hindi/Unicode rendering. Proposed v1 display capability is the verified printable Latin/ASCII repertoire. Native code retains a UTF-8 bounded representation; unsupported scripts yield a clear unsupported-language/text state and neutral glyph rather than silent transliteration, tofu, stripped road names or incorrect arrows. Broader fonts/shaping are a separately scoped addition. Feasibility records Maps version, Android/device, language, units, field availability and observed update cadence; version changes can invalidate a parser profile.

## 5. Age, liveness and removal semantics

Keep these separate: notification access granted; listener bound; eligible Maps notification present; parsable content; time since **meaningful source content change**; BLE authenticated; vehicle/display session; cluster acknowledgement.

A BLE heartbeat or `getActiveNotifications()` poll can confirm transport/presence, but cannot turn old instruction content into a new maneuver observation. Core instruction/direction/next-turn-distance/road evidence has its own change timestamp: an ETA/summary-only update or a generic app-status change must not refresh an old turn. Command sequence can advance while the core source-content revision/time stays unchanged. Rebinding or rereading a notification does not reset content age. Ignore duplicates/reordered callbacks by selected notification key/generation and normalized fingerprint. On initial baseline after process death/listener rebind, source age is Unknown unless justified by a supported observation; do not convert Android wall-clock notification timestamps directly into trusted elapsed freshness. The next verified meaningful source change establishes new evidence.

Proposed thresholds for bench validation, not demonstrated Google Maps guarantees:

| Condition | Proposed handling |
| --- | --- |
| New supported meaningful source update | Replace snapshot atomically; show only available fields |
| Same notification remains, text unchanged | Retain content age; do not manufacture a new instruction |
| Content unchanged>30 s | Label `Last Maps update`; neutralize directional emphasis, retain bounded text with age |
| Content unchanged>120 s | Hide maneuver/action fields; `Guidance update pending — check Google Maps` while presence remains |
| Presence refresh | At most every2 s while sharing; check listener/selected notification without refreshing content age |
| No valid end-to-end transport observation for5 s | Clear active turn/ETA; indicate connection unavailable briefly; no stale bright arrow |
| Selected notification removed | Immediately withdraw directional emphasis; up to750 ms replacement grace, then Clear if no validated replacement |
| Rerouting/GPS lost/paused/unsupported/redacted/listener disconnected/revoked | Clear old confident turn immediately; distinct state or return to normal info |
| Stop guidance/Stop sync/Forget/board boot | Clear promptly; no old stream packet can revive it |
| Arrived explicitly recognized | Arrival card up to5 s; then clear unless new validated source state supersedes it |

Long straight roads or waiting at lights may legitimately leave a Maps notification unchanged. Therefore these thresholds and UI treatment must be evaluated against actual Maps behavior in G0; do not claim a reliable continuous feed until that observation is made. Baseline/unknown-age text must be labelled as such, never falsely marked live. Exact constants freeze with the byte contract after feasibility. No source-availability claim is stronger than the notification evidence.

## 6. Android ownership, privacy and lifecycle

Create a concrete `MapsNotificationListener` OS-bound service, declared exported=false with `BIND_NOTIFICATION_LISTENER_SERVICE` and the notification-listener intent filter. Ensure filtering configuration does **not exclude ongoing/silent notifications**, which may contain Maps guidance. Wait for `onListenerConnected` before baseline reads; disconnect invalidates observation immediately. Rebind only through documented bounded APIs and no repeated permission prompts. The listener receives the OS events; it owns no GATT, route engine or Flutter state.

A native `TurnGuidanceSession` inside the existing `ClusterSyncService` owns the latest normalized observation, explicit forwarding intent and expiry. Connect through an in-process, lifecycle-safe native binder/coordinator; no Activity, EventSink, Flutter widget or strong reference to an obsolete service is retained. Use service-instance generations and a bounded latest-value handoff. Listener callbacks copy bounded data and return promptly; conversion/parsing does not block UI/ATT or run arbitrary provider operations.

Keep `BleBond` as the sole serialized ATT owner. Existing foreground connected-device service and notification remain; no location foreground service, second radio owner, GPS subscription or permanent wake lock is added. Maps callbacks must not bypass Android foreground-start restrictions. When no BLE sync is active, the OS listener can observe locally while enabled but does not silently connect or start an FGS from background. User foreground Connect establishes the existing authorized lifetime; the listener may hand over the current baseline with appropriate age uncertainty.

Locking the screen or disposing Flutter does not stop an already-active native sharing session. Service is START_NOT_STICKY with no boot receiver or automatic restoration of live guidance. After process death/Active apps Stop/force-stop/reboot, no cached turn becomes live; foreground reopen/Connect is the recovery path. Listener OS rebind is not permission to resume a cancelled ride. Permission revoke, explicit Stop and listener disconnect clear native observations/forwarding state appropriately; user preference alone does not prove availability.

Notification access is broader than Maps: explain this honestly before the OS grant. Ignore other packages before examining their extras; retain no notifications/history, no cloud forwarding/analytics, and only one active Maps candidate in memory. Android's own notification permission is distinct. No Accessibility, background location, contacts, SMS, microphone or screen-capture permissions are requested.

## 7. Firmware and lightweight protocol proposal

Preserve c011 enrollment, c012 metadata, c013–c015 telemetry/health and all legacy R15G/P/N/M semantics. Preserve Secure Connections/numeric approval, encryption/key requirements, one phone/GATT, connection/boot identity validation, current return heartbeat and actual source expiry. Vehicle continues acquisition/inputs/radio/validation; display alone owns LVGL.

**Remove old map GATT c016–c01b**, map/v2 queues/stores/object arenas/timing/camera/bulk handling. Do not repurpose their UUIDs or reinterpret their packets. Retired endpoints are absent; old apps/firmware report feature unavailable and continue pairing/metadata where compatible. Stale old map callbacks cannot activate guidance. Reflash/update both firmware and APK for the new feature; never erase NVS/bonds as a migration shortcut.

Provisional new suffixes under the existing service UUID prefix (unused in the inspected source; verify again at implementation freeze): c01c `GuidanceCapabilities` READ, c01d `GuidanceCommand` WRITE WITH RESPONSE, c01e `GuidanceStatus` READ/NOTIFY. These are **proposed, not implemented or final normative bytes**. Capability includes protocol/version/length/CRC, real readiness, vehicle boot, secure BLE connection token, display boot, max packet/text/MTU, supported maneuvers/glyph profile and TTL/clock limits. No forced ready=true.

Request MTU247 using the existing owner. Proposed maximum command value224 bytes implies actual MTU≥227. Below that, guidance is Unsupported without breaking pairing/metadata; no new text-chunk transport is introduced simply to accommodate an unmeasured lower MTU. G0/joint bench verifies negotiated MTU before finalizing this choice. If required support cannot negotiate it, explicitly revise the contract instead of sending truncated text or assuming247.

Commands: Open stream, Time request/confirm, Guidance snapshot, Presence (no content refresh), Clear/Stop. One outstanding control/ACK barrier; one latest replaceable guidance snapshot plus priority Clear, not an accumulating turn history or map pipeline. Message sequences and source revision are separate. Reconnect/new stream invalidates partial/in-flight data and source timing. Full CRC/version/range/UTF-8/reserved/length checks precede atomic state publication.

Proposed common command envelope40 + body + CRC4 (max224): magic `R15U`, version1, type, header40/reserved; vehicle/secure-connection/display/stream IDs at8/12/16/20 (u32); command seq at24; content revision at28; source-age claim at32 (u32, unknown sentinel); body length at36 (u16), reserved38. Fixed snapshot body180 is detailed in the firmware handoff. Proposed capability magic `R15F`, status `R15W`; confirm uniqueness at freeze. Explicit field sentinels separate0 m from Unknown.

Use a fresh monotonic end-to-end timing exchange: actual phone dispatch T0, display receive time DB, phone callback T1; accept RTT≤500 ms, refresh25 s, expire30 s, bound drift/transport and source age. This small clock helper can be extracted from previous validated concepts, but old map stores/protocols are removed. No location permission/coordinates are needed. Snapshot source-change and dispatch elapsed values must agree with age claims; Presence never updates content revision/age. Unknown source age remains Unknown even with a fresh transport anchor.

UART retains the existing **generic R-safe framed transport**, decoupled from map/projection headers and renamed for its transport responsibility. Preserve raw R15G/P/N/M demux,48-byte fragments, CRC, direction/sequence/offset bounds and escaped framing. Accept only new guidance message kinds, not retired map payloads. Total inner≤224 bytes; incomplete reassembly TTL500 ms; one control/snapshot assembly plus latest state. No PSRAM map arena, full-SHA cache, tile interest, route window, file transfer or multi-megabyte buffer remains. Target all fixed guidance state/queues/reassembly at≤4 KiB per MCU (excluding existing UART driver/task/LVGL/font storage), measured at implementation. Removing the previous~966,712-byte map arena is a source-derived saving target, not a runtime heap measurement.

Priority order: real acquisition/phone control/metadata first, Clear/Stop next, current guidance/presence/status next, diagnostics last. Snapshot changes coalesce to at most2/s; unchanged transport presence≤1/2 s. Budget≤1536 encoded forward UART B/s for guidance, including escaping/fragments; do not interpret useful payload bytes as wire bytes. Real measured throughput/latency must replace targets. A maximum-sized update is only a few hundred wire bytes, not a road-map upload.

Status ACK identifies current vehicle/connection/display/stream and consumed command, accepted source revision, normalized state, error and timing readiness, plus display receipt/render publication. Android ATT success is admission only; report Shared to cluster only after display ACK. ACK timeout1 s → one Status read → up to2 identical retries with bounded backoff, then Unavailable and existing-owner reconnect only if justified. Stop/Clear supersedes queued observations. Invalid, stale, old-stream or delayed packets cannot restore an old turn after Clear. Keep a cleared-stream tombstone/order bound until a forward Open; no reset-to-zero acceptance loophole.

## 8. Removal inventory and dependency cleanup

Before deleting: make a verified ignored archive of the current necessary sources/assets/docs and a tracked deletion manifest; Git history alone does not replace the repository's backup instruction. Preserve uncommitted stack/UART fixes and verified factory backups. No blanket git reset, dependency-cache purge, erase-flash or re-pairing as cleanup.

| Area | Remove / replace during implementation | Preserve |
| --- | --- | --- |
| Flutter | `features/navigation` map page/DTOs; Navigate tab; Where to next card; route/search/API-key/download/map controls | My bike readings, connection/Phone/radar/theme/artwork |
| Kotlin | NavigationRoute/Session/Sync/Wire, VectorMaps, MapSync/MapWire, PhoneLocation; map/location commands/permission flows/timers/queues/method-channel DTOs | MainActivity lifecycle-safe bridge, ClusterSyncService, BleBond/ATT serialization, metadata/telemetry/security/private Mac trust |
| Android/dependencies | MapLibre dependency/lock entries/native plugin generated references; GMS location21.4.0 if no consumer; map assets/font sprite generation; location FGS permission/type | Locked unrelated dependencies; INTERNET/Wi-Fi access if needed by existing Mac TLS bridge, Nearby permissions, notification/connected-device FGS |
| Permissions | App GPS coarse/fine and location explanation; retain ACCESS_FINE_LOCATION with maxSdkVersion30 only if still needed by supported legacy BLE scanning | Latest Android Nearby devices flow; no location prompt for guidance |
| Private storage migration | Delete only `noBackupFilesDir/navigation` cache/source/route/provider files and the exclusive `r15-navigation` Keystore alias, off UI thread | Bluetooth bonds/selection, Mac trust, display preferences, unrelated Keystore entries |
| Shared firmware | MapScene/Projection/Camera/Gesture/MapStore/NavigationStore/NavigationBridge/old map wire/SHA code when no live consumers remain | Generic UART framing/CRC helpers extracted with dependency-free normalized guidance types |
| Vehicle | c016–c01b handlers/subscriptions/bulk queues/announcements/map diagnostics; map-specific RecordWriter lanes | Acquisition, physical-input semantics, BLE trust/c011–c015, UART pins/baud, LED-off profile, ≥8192 main-stack guard and numeric health diagnostics |
| Display | MapMemory reservations, MapPanel/MapPage, map state/camera/gestures/commands/focus; map Settings tile; map-specific poll/status work | One LVGL owner, primary instruments/warnings/fuel, clock/Phone/Appearance,2 s lifecycle, proven UART fixes |
| Host/tools/build | Map-only preview executables/CLI branches/fixtures, `tools/maps` utilities, map-specific assets/fonts only if exclusively used, CMake map source lists | Standard dashboard/development preview, serial clock/ADB/guarded flashing workflows, generic icon/font assets used elsewhere |
| Documentation | Move map plans/protocols/provenance/ODbL data references to explicitly historical indexed archive outside active handoffs; replace README/status/command references | Necessary licences/source attribution with any retained archived data, actual stack-overflow evidence/verification limitations |

Do not remove `SettingsNavigation.h`, `TouchNavigation.h` or general widget navigation merely because their filenames include navigation. Remove only map-specific branches. Do not delete generic font/icon files if other screens use their glyphs. Old normative map vectors are archived with the retired protocol, not silently repurposed as new-contract vectors.

GATT removal must trigger normal service discovery/Service Changed handling, not hidden Android cache refresh or automatic bond deletion. Mixed old/new APK/firmware explicitly show guidance unsupported and keep basic features stable. Stale BLE handles/callbacks are fenced by current owner generation; unsupported old commands never reach the new guidance parser.

## 9. Delivery order and completion gates

| Gate | Deliverable / acceptance before advancing |
| --- | --- |
| G0 baseline + source feasibility | Bench-confirm fixed vehicle uptime/stack margin/pairing with current images. Build a small Android notification-observation path in Actions, inspect **your** real Maps notifications for start/turn/long straight/reroute/end/locked screen/language; show local-only normalized preview. No destructive map removal yet. Record supported fields/update cadence and unsupported cases. If inaccessible, stop and review the product choice. |
| G1 contract + visual freeze | Freeze normative bytes/static vectors, source-age policy, icon/text repertoire, actual MTU and no-menu dashboard placement. Manually inspect800×480 both layouts/themes, warnings, unavailable and lifecycle states using host preview. No code-only feasibility claim. |
| G2 firmware replacement first | Back up, remove old map runtime/menu/arenas and implement shared model/new channel/vehicle relay/display card. Build both firmware images; manual host scenarios (clearly labelled simulation) and bounded resource/stack review. Bench-confirm legacy pairing/metadata/inputs and new ACK/clear/timing behavior before mobile full integration. |
| G3 mobile replacement/integration | Back up, remove old Dart/Kotlin/provider/location/assets/dependencies/storage flows, integrate proven listener/parser/session into the one BLE owner, add consent/controls/My bike status and upgrade migration. Format/analyze only locally; APK/native compilation in Actions. |
| G4 joint target-device acceptance | Install matched APK/images; actual Maps → phone preview → authenticated BLE → vehicle → UART → LVGL. Prove supported arrows/text/unknowns, manual Stop/removal/revoke, reconnect/no stale revival, frame latency/resources and60 min untethered locked-screen coexistence. |
| G5 cleanup/handoff | Confirm no live map menus/endpoints/jobs/cache/provider-key/own-GPS dependencies; update active docs and exact build hashes, parser compatibility record and known limitations. User controls commits/push/flashing unless explicitly authorized. |

These are implementation/acceptance gates, not a promise that notifications will expose every desired field. Full completion means actual observation succeeds on the target phone and cluster, not simply that sources compile or a sample arrow renders. No automated suites may be created/restored/run. Firmware builds/static analysis/manual host/bench checks are allowed. Android builds stay **GitHub Actions only**; modern device normal text light/dark; no small-screen/2× work.

## 10. Manual acceptance matrix

- Baseline: vehicle runs≥5 min over many diagnostic intervals without panic/reset; main/TX stack margins recorded; pairing approval, saved reconnect, name/battery/time and physical input telemetry unaffected.
- Permissions: first enable/decline/revoke/re-enable; granted-but-unbound listener; POST_NOTIFICATIONS denied; Maps missing/disabled/notifications disabled/work-profile restriction; unrelated phone functions remain available.
- Selection: navigation vs ordinary Maps notifications, group summary, multiple keys, replace/remove races, locale/unit/version changes, empty/redacted/custom extras, identical callbacks and listener baseline. Other apps' messages must never appear or be retained.
- Guidance: explicit left/right/keep/U-turn/roundabout/arrive, multi-stage wording, unrecognized maneuver/units, zero/unknown/large distance, long/unrenderable text, missing ETA; compare each exposed value against the actual notification. No fabricated glyph/countdown or stale bright instruction.
- Age: long straight, stationary lights, delayed callbacks, duplicate/reordered updates, reconnect while source unchanged, missing Presence/anchor, phone clock changes; content age is not refreshed by polls or transport retries.
- Transport: MTU below/at/above minimum, missing endpoints, CRC/version/length/reserved/bounds errors, dropped/duplicate ACK, partial/interleaved UART frames, busy metadata/telemetry, Clear preemption, late old stream/session, board reset one at a time.
- UI: three Settings tiles and correct Clock index; touch/non-touch actions, Street/Track light/dark, warning/gear/fuel priority, pairing modal and2 s lifecycle; no Navigation menu, no full map screen, no stale card replay.
- Lifecycle: lock/unlock/page dispose/Activity recreation, out-of-range/BT off, listener unbind/permission loss, process death/Active apps Stop/force-stop/reboot, Stop guidance/Stop sync/Forget. No extra GATT/idle GPS/work resurrection.
- Performance: observed Maps callback→phone publish→vehicle admission→display ACK latency (with stated observation uncertainty), two-way UART source gaps/queue depth, bounded memory/map-arena removal, both task stack margins,60 min energy/stability. Target≤500 ms from a newly received listener event to visible guidance under healthy links is a **target**, not demonstrated provider latency.

## 11. Primary references and evidence limits

- [Android NotificationListenerService](https://developer.android.com/reference/android/service/notification/NotificationListenerService): OS callbacks/access/service binding and baseline-read lifecycle; this does not document Google Maps' notification schema.
- [Android Notification extras](https://developer.android.com/reference/android/app/Notification): title/text/big-text public fields; none guarantees a maneuver/ETA field for Maps.
- [Notification access settings](https://developer.android.com/reference/android/provider/Settings#ACTION_NOTIFICATION_LISTENER_SETTINGS): user-controlled access flow.
- [Google Maps intents](https://developer.android.com/guide/components/google-maps-intents): opening Maps; launching the app does not authorize extracting a structured route.
- [Official Navigation SDK turn-by-turn feed](https://developers.google.com/maps/documentation/navigation/android-sdk/tbt-feed): separate supported structured-data alternative; it is not the notification approach selected here and is not added silently.

Repository/source inspection and primary documentation support this plan. No real Google Maps notification payload, successful extraction, external-display frame, compatibility range, battery/latency result or post-stack-fix stability result has been obtained in this planning turn.
