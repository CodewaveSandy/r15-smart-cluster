# Google Maps notification guidance — Android handoff

**Firmware ready for Android source integration — 2026-11-05:** Both firmware paths now implement [the normative v1 bytes](../../docs/turn-guidance-protocol.md) with [static examples](../../shared/protocol/turn-guidance-v1-vectors.txt). These supersede every proposal below. App listener/parser/transport are still unimplemented; actual Maps payload/profile/age cadence, MTU and physical acceptance must be observed. Implement this channel inside the existing BLE owner, request MTU247 and require actual≥227, then capability→status subscription→new Open→TimeRequest/Confirm→Snapshot/Presence with display publication ACK. Clear is priority; keep global command order across streams and do not refresh core content age on Presence/ETA/retry. Firmware-first ordering was explicitly requested by the user; source feasibility remains a gate for useful real phone guidance.

**Removal progress — 2026-11-05:** Custom map/navigation source, menu/tab, providers/GPS and c016–c01b are removed in the working tree. Firmware turn guidance is implemented; Android remains unimplemented. Normative v1 bytes supersede the candidate IDs/layouts below. User explicitly requested removal first.

Planning only, 2026-11-05. Read [the complete cross-system plan](../../docs/turn-by-turn-plan.md) and [firmware contract proposal](../../docs/turn-by-turn-firmware.md). This replaces custom maps/routes only when implemented. No source removal, feature implementation, new permission or build occurs in this planning turn.

## G0: prove the source before removing the old feature

First bench-confirm the vehicle stack correction and existing pairing. Then build a narrowly scoped native notification listener/phone-only preview in GitHub Actions. User explicitly grants notification access. Filter package `com.google.android.apps.maps` and current user/profile before reading fields. Observe actual navigation start, one clear turn, long unchanged instruction, rerouting, end/removal/replacement, locked screen, locale/units and Maps update behavior on the target device. Record field/type presence and numeric cadence/version metadata; no raw route/address/notification text in logs or Git. Show the private current raw/normalized preview locally if the user needs to compare it.

Do not assume a particular Maps channel, notification ID, small icon, custom extra or RemoteViews layout is a public contract. Standard extras are candidates, not guaranteed guidance. Ignore unrelated Maps alerts and other packages. If no safe useful text exists, report unsupported and stop before irreversible cleanup; do not quietly add OCR, Accessibility, screen capture or a billed SDK. If only text is available, the reviewed product capability is text-only; Unknown maneuver and missing distance/ETA remain supported states.

Freeze supported parser profile/device/Maps version/language and update-age policy with G1. English first, explicit units and exact instruction patterns; no NLP guess or parsing `left` in a place name. Multiple-step text must not attach the wrong distance to a turn. Validate and bound text/units/number ranges independently, preserve Unicode safely and flag unsupported display repertoire. No own GPS matching/countdown/ETA/reroute computation.

## Removal map once G0/G1 pass

Back up/hash the current sources/assets/docs first. Remove the old `features/navigation` screen/DTOs, Kotlin NavigationRoute/Session/Sync/Wire, VectorMaps, MapSync/MapWire, PhoneLocation and their service/Activity/GATT/method-channel integration. Remove map provider/API-key UI, searches, corridor/cache jobs, map sprites, map assets, MapLibre and the native GMS location dependency when no remaining consumer exists. Update committed dependency lock and generated plugin references through the normal Flutter tooling, without an Android build/SDK installation. Do not hand-maintain an inconsistent lockfile.

Remove Navigate tab and Home Where to next action. Keep My bike as the main screen, normal connection settings and approved Phone/radar. Replace the route entry with a small Google Maps guidance status/control card: Enable/Disable, access/listener/source/cluster status, current normalized preview, Open Google Maps. No duplicate search/navigation menu. There is no new full-screen map or keyboard destination flow.

Remove location-service FGS type/permission and GPS runtime prompts. Preserve legacy BLE scan location permission only for Android≤30 as needed, with its SDK bound; modern Nearby permissions remain. Keep connected-device FGS/POST_NOTIFICATIONS. INTERNET and Wi-Fi state remain only where still required by the independent existing Mac preview/TLS path, not for a hidden provider downloader. Preserve Mac trust and its Keystore alias.

Perform one versioned, idempotent native cleanup of old navigation source/tile/route manifests and encrypted Geoapify key/its exclusive alias. Do not enumerate/delete unrelated app files, bonds, selection, signing credentials or other Keystore entries. Do not briefly restore a persisted old route as guidance during upgrade. An interrupted cleanup can retry, with bounded failure reporting.

## Native implementation ownership

- `MapsNotificationListener`: exported=false, OS permission `BIND_NOTIFICATION_LISTENER_SERVICE`; proper action; accepts ongoing/silent navigation candidates. Wait for connected callback before active-notification reads; disconnect/revocation invalidates source immediately; bounded documented rebind only. No GATT or Activity/Flutter dependencies.
- `MapsNotificationParser`: pure bounded normalization/parsing from a copied candidate; actual proven profile and per-field validity. No routing/network/GPS/bitmap scraping. Input maximum, duplicate key/fingerprint and selection generation fence expensive or obsolete work.
- `TurnGuidanceSession`: lives in ClusterSyncService, keeps one current observation, meaningful-change revision/time, source age uncertainty, independent user sharing intent, removal grace and Stop tombstone. Listener service-instance/binder handoff is native and bounded. EventSink only observes snapshots; no Activity/result retained by workers.
- `TurnGuidanceSync`: integrate the new channel into BleBond's existing serialized ATT operation owner, after metadata/telemetry priority. Read/validate actual capability, subscribe/read Status, establish current source/clock stream, latest snapshot or Clear, bounded ACK retry/read fallback; never a second GATT or hidden bond repair.
- Flutter: immutable domain DTO/controller emits Enable/Stop/Open Maps/Open access settings/Connect intents and renders truthful states. It owns no background polling, notification listener, phone GPS or transport timer.

Names describe concrete consumed responsibilities; implementation may combine small adapters rather than create unused abstractions. Preserve existing trust, approved pairing, saved reconnect, c011–c015, telemetry freshness and user Stop behavior.

## Consent, lifecycle and confidence

Explain broad OS notification access vs our Maps-only filtering, and that no routes/history are stored. Grant/revoke flow is via Android settings and observed binding, not a fake success dialog or normal notification permission. No automatic dismissal/snoozing/clicking Maps actions. Open Maps through a user-initiated documented intent; handle not installed/disabled without crashing. User chooses destination and mode there.

Start/enable radio foreground work only while visible through the existing Connect intent. A notification appearing while no sync is active cannot bypass background-start restrictions. With existing sync active, native listener/session/foreground connection survive screen lock/Flutter disposal. Stop guidance clears remote state while retaining sync and Maps navigation; Stop sync clears/disconnects cluster only. Permission loss/listener disconnect cannot be healed by replaying a cached turn.

Keep START_NOT_STICKY/no boot restart. Persist user configuration only, never active guide/source age. OS listener rebind/process recreation may observe a baseline but does not restore a cancelled sharing intent or establish a fresh content timestamp. Foreground reopen/Connect restarts the existing authorized service; baseline age is unknown until justified. A connected listener/present notification is distinct from a newly changed instruction. Presence checks/retries/identical snapshots cannot keep old maneuvers bright indefinitely.

Exact content-age/display policy is in the main plan. Provisional30 s diminished/120 s action-hidden thresholds must be validated on long straights; do not conceal this limitation by resetting a timer on polling. ETA/remaining/roundabout exits exist only if exposed and parsed. Unsupported-language/custom notification states remain visible on the phone and safe on the cluster. User can disable this optional feature without losing ordinary pairing and telemetry.

## Transport/verification handoff

Proposed new c01c–c01e, max224-byte command/actual MTU≥227, authenticated context, new stream/sequence/content revision, monotonic clock and atomic display ACK are detailed in the firmware handoff. G1 must freeze final bytes and static examples before coding both encoders. c016–c01b are retired/absent, never repurposed; mismatch reports feature unavailable. Service Changed/current-generation discovery remains normal; never hidden refresh or automatic re-pairing.

Only format/static analysis/syntax parsing locally. **No local APK/Gradle/native compilation, Android SDK installation, automated suites or small-screen/2× work.** APKs and Android compilation stay in the existing Mobile APK GitHub Action. Modern target phone normal text, light/dark/unavailable states and physical firmware acceptance are manual.

Finish with matched image/APK/parser version evidence: actual notification text vs preview/display; field unknowns; start/reroute/end/removal/Stop; locale/unit/Maps version mismatch; revocation/baseline/process death/reconnect; latest-only queues/no stale revival;60 min locked-screen acquisition/metadata/telemetry coexistence, event→ACK latency and battery. Sample arrows/native compile alone are not a successful Google Maps integration.
