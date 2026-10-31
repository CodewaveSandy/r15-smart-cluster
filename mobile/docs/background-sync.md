# Android background-sync strategy and work plan

Updated: 2026-10-31. Status: planned, not implemented. Start here for mobile implementation independently of firmware work. Read mobile/AGENTS.md, [architecture](architecture.md#companion-architecture-status-and-approved-ui), [current protocol](protocol.md) and [development](development.md). Firmware work starts at [the firmware plan](../../docs/background-sync.md). The shared first acceptance gate is B1 in that plan.

## Outcome and current gap

The rider connects while the app is visible, later starts a route, locks the screen and pockets the phone. Active device sync and eventually location/navigation must continue independently of Flutter pages. Explicit Disconnect/Stop must still win.

Current app has real pairing, one GATT owner, saved-peer foreground reconnect and authenticated name/battery/UTC updates. `BleBond` takes an Activity; `MainActivity.onDestroy` disposes it. `BleConnection` cancels background retry scheduling and backgrounded saved-connect attempts. Native Handler metadata refreshes every 30 seconds while alive, but no foreground service or process recovery owner exists. Navigation, vehicle telemetry notifications, modern GPS permissions and maps are absent. Current device success is not locked-screen acceptance.

## M1 implement background BLE continuity first

M1 uses existing firmware and c011/c012 exactly as specified in [protocol](protocol.md). No new UUID, heartbeat, firmware edit, map SDK or GPS permission is required for this milestone.

### Connection ownership

Create a concrete Android started foreground service for the active companion connection, using application/service context for GATT and metadata observations. Retain one serialized GATT owner; adapt the existing `BleBond` rather than adding a second connection or duplicating its operation queue. Keep discovery, enrollment, MTU and writes ordered through actual callbacks; preserve three-second enrollment timing and the existing bounded security retries.

`MainActivity` becomes a lifecycle-safe command/state bridge. Flutter binds/observes current service state on attach/resume, obtains an immediate full snapshot and sends semantic Connect/Disconnect/Forget-selection intents. Unbinding, navigation, widget disposal or Activity destruction must not disconnect an active user-authorized sync session. Remove the current lifecycle cancellation only after service ownership exists. No stale Activity, engine, EventSink or MethodChannel result may be retained by the service. Preserve explicit cancellation and guard late callbacks with current GATT/session generation.

Separate device-connection intent from future route intent: End ride stops GPS/route transfer; explicit Disconnect/Stop sync closes BLE and cancels recovery. Do not erase trust for either. Forget remains confirmed app-selection cleanup plus instructions for Android/cluster bond removal; no remote Forget or hidden Android API is introduced.

### Android configuration

Declare an unexported service with `android:foregroundServiceType="connectedDevice"`, `FOREGROUND_SERVICE` and `FOREGROUND_SERVICE_CONNECTED_DEVICE` for applicable API levels. Keep version-compatible behavior and verify actual min/target SDK from the pinned build. Require current Nearby/Bluetooth access before starting; a saved grant is not authority. Start/promote the service promptly while the app is visible on deliberate Connect or the existing foreground saved-reconnect flow. Do not launch a hidden service unconditionally at boot.

Show an ongoing connection notification with accurate Connecting/Connected/Reconnecting state, a return-to-app action and explicit Disconnect/Stop sync. Configure a notification channel and declare/request Android 13+ `POST_NOTIFICATIONS` with an explanation; permission denial does not itself prohibit starting a foreground service, but changes notification visibility. Foreground service declarations/permissions are platform requirements, not a guarantee against termination. Android supports a `connectedDevice` service for sustained BLE communication. [Android BLE background guidance](https://developer.android.com/develop/connectivity/bluetooth/ble/background), [service types](https://developer.android.com/develop/background-work/services/fgs/service-types), [notification permission](https://developer.android.com/develop/ui/views/notifications/notification-permission).

Keep M1 native scheduling alive without requiring a running Flutter UI/isolate. WorkManager is for deferrable work; it is not the owner of continuous ride telemetry. Do not use a permanent CPU wake lock or blanket battery-optimization exemption as the initial design. Measure locked-screen gaps and energy first; any narrowly scoped wake mechanism or Samsung battery-setting guidance requires evidence and truthful limitations.

### Recovery, limits and state

Move retries out of the widget/foreground-only Dart lifecycle into the service. A concrete initial policy to implement and validate: the existing four attempts with 1/2/4/8-second delays, then a 30-second cooldown before another bounded round while an active connection intent, permissions, radio and saved bond remain valid. Only one timer/attempt may exist. Preserve current authentication/remote-termination stop decisions; numeric GATT status alone is not a diagnosis. Explicit Stop/Disconnect, selection change, stale/removed bond and permission loss cancel work. Do not auto-pair a refused peer or open enrollment in the background.

Persist only minimal non-secret connection intent/selected-peer hints needed for defined recovery. A persisted “connected” flag never establishes a link. Document behavior for OS process death, recents swipe, Android Active apps Stop, Settings force-stop and phone reboot separately. Do not promise recovery after force-stop or silently defeat OS/user Stop. If Android recreates an eligible service, rebuild native state from real observations under launch restrictions; otherwise the app must explain the required foreground restart. Background-start exemptions/CompanionDeviceManager presence are a later evaluated option, not an assumed entitlement. [Launch restrictions](https://developer.android.com/develop/background-work/services/fgs/restrictions-bg-start), [user stopping behavior](https://developer.android.com/develop/background-work/services/fgs/handle-user-stopping).

Retain separate observations: saved bond, live GATT, accepted/fresh metadata, service running, reconnecting/stopped. Keep 44-byte snapshots current at dispatch, 30-second normal cadence, event coalescing, minimum 1.1-second spacing and 90-second success expiry. An optional GATT operation timeout currently blocks further optional work until reconnect; preserve non-overlap and define an observable bounded recovery action without issuing competing GATT calls or erasing trust. Older firmware remains connected with metadata unsupported.

### Files and checks

Inspect `android/app/src/main/kotlin/dev/r15cluster/r15_companion/{MainActivity,BleBond,PhoneMetadataSource}.kt`, `AndroidManifest.xml`, `lib/features/phone/application/ble_connection.dart`, `lib/features/phone/data/ble_pairing.dart`, `lib/app/companion_app.dart` and `companion_home.dart` before editing. Keep first-time radar, OS pairing approval, bottom navigation, Mac-preview adapter and current UI intact. Add service/channel code only when consumed; avoid a generic background framework.

Format/analyze and build using [development commands](development.md). No automated suites may be created/restored/run in this phase. Complete joint B1 from [the firmware plan](../../docs/background-sync.md#joint-acceptance-b1-locked-screen-continuity), including 60 minutes locked, Activity recreation, cluster restart/out-of-range recovery and explicit Stop. Run without the debugger/phone USB; record battery setting, permissions, Android/device, metadata gap/age and service/link events. Inspect approved dark/light/large-text/reduced-motion UI and notification actions. Pass B1 before calling background continuity complete.

## M2 bidirectional telemetry, after B1

Implement only against a jointly frozen [future contract](protocol.md#future-telemetry-and-navigation-contract-gate). Firmware must provide authenticated capabilities and a real vehicle-to-phone stream; currently it does not. Subscribe once per current connection, restore subscriptions/full snapshot after reconnect, validate boot/session/sequence/source age and publish immutable quality-bearing `BikeTelemetry`. Keep unavailable inputs unavailable; phone GPS speed and phone battery are never substituted for bike readings.

Bound/coalesce incoming data and UI updates; a hidden page must not own subscriptions. Keep service/transport callbacks independent of widgets. Agree feature rates, stale deadlines, limits and recovery behavior with firmware before implementation. B2 acceptance requires actual bidirectional delivery, old-session rejection, independent field expiry, reconnection and acquisition/display responsiveness under sustained load.

## M3 active navigation and pocket operation

After provider selection and licensed embedded-delivery feasibility, keep GPS/routing/rerouting and route progress in an owner that lives as long as the foreground ride service. If using Dart/provider SDK logic, explicitly design its background engine/isolate lifetime; it cannot depend on a disposed UI engine. Flutter pages show snapshots and issue Start/End/recenter intents. Rendering a phone map while visible is separate from maintaining route computation while locked.

Add the `location` foreground-service type and `FOREGROUND_SERVICE_LOCATION` permission where required, modern `ACCESS_COARSE_LOCATION`/`ACCESS_FINE_LOCATION`, runtime precise/approximate handling and current Location services checks. Start location service work from visible Start ride with granted permissions, before pocketing. Foreground location service access can continue with the screen off; do not automatically require “Allow all the time” merely for this flow. Evaluate `ACCESS_BACKGROUND_LOCATION` separately if the chosen restart/background-start/provider requirements need it. Degrade GPS/navigation independently from BLE when denied/revoked. [Location permission guidance](https://developer.android.com/develop/sensors-and-location/location/permissions).

Add main-manifest `INTERNET` for real provider/network operations; it currently exists only in debug/profile manifests. Review provider keys, restrictions, attribution, quotas, offline/reroute behavior and permitted embedded data transfer before adding dependencies. Keep audio with phone/headset. iOS needs its own native BLE/location background assessment and device verification; Android configuration is not transferable.

Use BLE for reviewed compact navigation data only within measured limits. Wi-Fi bulk maps remain a coordinated firmware dependency; verify cellular internet routing, authentication, resume/backpressure and route cancellation. Do not assume Wi-Fi avoids the 115200 UART bottleneck or that full moving maps are already feasible. B3 acceptance covers locked-screen GPS quality/expiry, reroute/route generations, data loss/recovery, offline/provider failures, real display navigation and foreground-service energy over a representative ride. Initial bench checks precede any bike observation.

## Handoff and definition of done

Give the firmware developer app revision/APK build, phone/OS, service lifecycle policy, supported protocol revision and B1/B2/B3 results with numeric diagnostics. Never log keys, names/addresses, route history or payloads. Both sides record the same gate result; “app stayed connected” alone is insufficient. M1 can start independently today; M2 and M3 depend on separately reviewed firmware contracts, not invented UUIDs or fake telemetry.
