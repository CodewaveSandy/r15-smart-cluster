# Android implementation handoff: connect to the real R15 cluster

Date: 2026-10-31. Audience: the developer working on the second laptop. This is the next implementation task after the approved app UI, BLE searching and radar results. Work on the Android companion in `mobile/`; coordinate firmware changes with the firmware developer instead of changing its contract locally.

## Requested outcome

The rider can select their real cluster in the companion, see their phone name and the actual pairing code on the display, approve the matching code on Android and the cluster, and keep a live BLE connection. After both boards restart, the app can reconnect to the saved cluster without another code. Disconnect retains the bond; Forget removes trust through an explicit user flow.

Deliver this real BLE connection milestone first. Wi-Fi provisioning, application authentication/capability negotiation, telemetry, navigation/maps, notifications and music are later work. A saved Android bond does not establish an application session or prove those services work.

Read `mobile/AGENTS.md` and its referenced rules before implementation. Preserve the approved UI, existing Mac-preview flow and other app work. The current mobile instruction is manual testing with formatting/static analysis and affected builds; do not add new automated tests in this phase. Do not install or build the mobile toolchain on the firmware Mac.

## Firmware evidence and limitations

The physical vehicle ESP advertises **R15 Cluster**. User bench tests confirm numeric-comparison bonding, encrypted saved-bond reconnect after powering both boards off/on, and the new large pairing modal with nRF Connect. The latest screenshot shows CONNECTED / BONDED, AES encryption and the name service below. Automatic companion reconnect and phone-name delivery from the companion have not yet been verified.

Both ESPs must have the firmware containing the phone-name addition. A later vehicle-only fix preserves an already-running Android security procedure instead of disconnecting on NimBLE `BLE_HS_EALREADY`; use that corrected firmware too. The firmware developer supplies these images and the wiring/flash steps. This handoff does not authorize mobile-side firmware edits.

Vehicle owns BLE and bond NVS; display owns the modal and communicates over UART. There is one BLE connection and one saved phone bond. Disconnect nRF Connect before testing the companion, otherwise it occupies that slot. The phone connects to the vehicle, not the display.

Canonical contracts:

- [Phone-name UUIDs, payload and UART boundary](../../docs/phone-name-integration.md).
- [Bench wiring, pairing, reconnect, Forget and diagnostics](../../docs/phone-uart-bench.md).
- [Mobile architecture](architecture.md), [journey](user-journey.md), [security](security.md) and [transport plan](protocol.md).

Do not use the Mac preview's `…c001/…c002` service, TLS bootstrap or HTTP session endpoints to connect this physical firmware. The `…c010/…c011` service is only for an optional display label; it is not a telemetry/authentication/session service.

## Start with these existing app boundaries

Inspect these files before editing; adapt to newer app changes if present:

| File | Current role / needed change |
| --- | --- |
| `mobile/android/app/src/main/kotlin/dev/r15cluster/r15_companion/BleBond.kt` | Starts LE GATT and immediately calls `createBond`; finishes by disconnecting/closing, even on success. It must support name delivery and hand a successful live connection to a real session owner. |
| `mobile/android/app/src/main/kotlin/dev/r15cluster/r15_companion/MainActivity.kt` | Existing `dev.r15cluster/nearby` channel, scan/read/bond/access handlers and operation ownership. Extend with bounded native connection events/actions; preserve preview behavior. |
| `mobile/lib/features/phone/data/nearby_platform.dart` | Native platform boundary. Keep Android GATT objects in Kotlin; expose semantic results/events to Dart. |
| `mobile/lib/features/phone/data/ble_pairing.dart` | Existing bonding adapter. Separate a saved bond from a live link. |
| `mobile/lib/features/phone/domain/phone_status.dart` | Existing presentation/readiness model. Do not reuse a Mac application-session success flag for an OS bond. |
| `mobile/lib/features/phone/application/` | Own user flow and connection state outside widgets; continue the existing dependency direction. |

The temporary bonding operation must release receivers/timers when finished, but successful completion must not close the GATT connection if ownership transfers to the active connection controller. Failed/cancelled attempts must close their resources once. No activity, popup or native callback may outlive its owner accidentally.

## Implement in this order

### 1. Preserve actual discovery and select the intended device

Keep the exact case-sensitive advertised-name prefix rule `R15 Cluster`. Preserve real results, full two-line names, stable decorative radar positions, bounds, permissions and cancellation. Do not require our custom service UUID in advertising: firmware currently advertises its name, not that service UUID.

Stop scanning before connecting. Open a single LE GATT connection to the selected real result. Guard against double taps, late scan results and callbacks from superseded GATT objects. A prefix/name match is discovery only, not authentication. Do not automatically select another similarly named cluster during retry.

### 2. Send this phone's label before requesting a new bond

On successful GATT connection, discover services. For a new/unbonded connection, find:

| Item | Exact value |
| --- | --- |
| Service UUID | `813b1240-8fd9-4d67-a53b-9f0a2531c010` |
| Writable characteristic UUID | `813b1240-8fd9-4d67-a53b-9f0a2531c011` |
| Write type | Write with response |
| Payload | 1–32 printable ASCII bytes, at least one non-space; no JSON/NUL/newline/length prefix |
| Example string | `Samsung S25 Ultra` |
| Example hex | `53616d73756e672053323520556c747261` |

Use the phone's local Bluetooth name when permitted and available, or a genuine user-visible model/friendly label. Never use the remote cluster's `BluetoothDevice.name` or its `0x2A00` Device Name as the phone's name. Keep the value within the supported ASCII profile; define an explicit fallback for unavailable/non-ASCII labels rather than silently inventing a Samsung name for every device.

Serialize Android GATT operations: service discovery → name write → write callback → bonding request if still needed. Use the Android API appropriate to the app's SDK with a compatible older-OS path; wait for actual callbacks rather than treating an operation's initial return as completion.

The vehicle gives an unbonded connection **up to three seconds** for optional name discovery/write before initiating security itself. A valid name write advances security initiation to the next host turn (at least 20 ms). Therefore discovery/write must begin immediately. Android or firmware may already have initiated bonding: check the real bond state and do not start conflicting duplicate operations. If security/prompt comes first, preserve it; missing metadata must not abort an otherwise valid pairing. The display falls back to Phone name unavailable.

Writes are accepted only for the current unbonded connection, an open enrollment window and fresh display communication, before cluster approval. The first accepted label is fixed for that connection; identical retries are accepted, different replacements refused. Do not retry a refused metadata write indefinitely. Bonded reconnects do not need this write, and the current endpoint rejects bonded-peer name updates. Names are RAM-only enrollment labels, not retained phone profiles.

### 3. Complete real OS pairing and retain the live link

First tell the rider to open **Settings → Phone → Pair phone** on the cluster; that opens a 60-second enrollment window. Without it, a new/untrusted connection is intentionally refused.

Use Android's actual OS numeric-comparison flow. If the device is not already BONDING/BONDED after metadata handling, request bonding once and observe the real outcome. The display modal shows the six-digit code; its decision deadline is 30 seconds. The rider must compare the codes and approve on both devices. Preserve leading zeros. Never fabricate a code, intercept/auto-confirm the OS approval, or treat the phone name as proof of identity.

Observe bond changes for the selected device only, inspect actual OS state and ignore stale/spoofed/unrelated broadcasts. Handle permissions revocation, rejection, timeout and disconnect explicitly. The existing app operation has a 45-second bound; keep a finite overall operation bound and clear all timers/receivers on completion.

After success, maintain the same selected live GATT connection in one concrete owner. Expose connection changes to Flutter through a lifecycle-safe stream/channel. Do not keep the old `finish()` behavior that disconnects on BOND_BONDED, and do not create a second competing connection just to keep it alive.

### 4. Represent connection truth accurately

Keep these states distinct in domain/presentation state:

| Observation | Meaning |
| --- | --- |
| Device found | Advertisement observed; no trust/link established |
| Connecting / pairing | Operation in progress, cancellable where OS permits |
| Bond saved | Android reports BONDED; can still be physically disconnected |
| Live BLE link | Selected current GATT callback reports a successful live connection |
| Application services ready | Requires a future negotiated/authenticated application contract; unavailable today |

Use confirmed native observations for the physical BLE-connected UI. If showing Connected for this milestone, make its limited meaning clear (Bluetooth link) and keep Navigation/Notifications/Music readiness pending. Do not set the existing application-session Connected state solely because `bondState == BONDED`, a name write succeeded or public Generic Access reads succeeded. Those observations do not independently prove encryption or application authorization. The cluster already verifies encrypted/authenticated saved-bond state before showing its confirmed connected status; a matching app-side readiness/proof characteristic is not yet provided. Request a reviewed firmware contract when that stronger status is needed—do not invent a UUID or infer it from an unencrypted metadata write.

A failed new attempt must not erase an existing saved bond or claim success. Preserve actionable failure text instead of a generic ready message for every cause.

### 5. Saved-cluster reconnect and disconnect

Persist only the bounded selected-device information needed to find that same saved peer; use OS bond storage for Bluetooth keys. Do not store keys, pairing codes or phone data in logs/preferences. A cached address/name is a lookup hint, not a permanent authenticated identity guarantee; handle stale selections and later identity-contract changes explicitly.

On foreground app launch/resume or loss of a previously live link, recheck permissions/Bluetooth and the selected bond, then reconnect to that same cluster. Do not request the enrollment window, rewrite the phone name or prompt for a new code when the saved bond is valid. Firmware starts saved-peer security immediately.

For the initial implementation, keep retry finite and foreground-owned: for example four attempts with 1/2/4/8-second delays, then explicit Retry. Cancel scheduled/active work on user Disconnect, selection change or disposal. Do not start retries during an active first-pair flow. Background reconnect across process death needs a separate Android lifecycle/service decision and physical tests; do not promise it now.

Explicit Disconnect closes the live GATT link and cancels automatic retries for that user action; retains the bond. Selecting Connect again restores it without another code. A stored bond alone never displays a live connection after app restart.

### 6. Forget and asymmetric bonds

Cluster-side Forget is already a semantic display→vehicle action. It disconnects first, removes vehicle bond storage and returns to Pair phone / Back; it does not erase Android's copy. The phone must react to the real disconnect and stop showing Connected. Do not silently re-pair or repeatedly reconnect a stale Android bond to a now-untrusted cluster.

The firmware currently has **no companion-accessible remote Forget command**. Do not repurpose the name write for it or invent a control packet. If adding app-side Forget now, explain whether it only clears the app's selection and guide the rider to Android Bluetooth settings and cluster Forget for both sides. Do not use hidden/reflection Android bond-removal APIs or silently delete saved trust. Any future remote-Forget protocol requires explicit design with the firmware developer.

## Manual acceptance before calling this complete

Use the real Samsung/Android phone and both powered boards. Keep nRF Connect disconnected except when separately comparing behavior. Format/analyze and build the affected app/native target using the pinned toolchain. Do not create or restore automated tests in this phase; use manual acceptance.

| Check | Required result |
| --- | --- |
| First pair from radar | Real cluster selected; only one GATT connection; actual OS prompt and cluster modal |
| Phone name | Companion's transmitted label appears above the code; no text overlaps |
| Long/missing/unsupported name | Bounded/explicit fallback; no crash and no forced pairing failure |
| Approval | Same actual six digits including leading zeros; approve on both; display confirms authenticated link |
| Persistent live link | App remains BLE-connected after bonding popup closes; no immediate cleanup disconnect |
| Reject/Cancel/timeout | Clear failure; no success state or erased prior trust; retry possible |
| Closed enrollment | New device refused with instructions to enable Pair phone |
| Restart both boards | Foreground app reconnects to selected saved cluster without another code or tapping Connect |
| Relaunch app with saved bond | Readiness checked; real reconnect required before showing a live link |
| Explicit Disconnect/Connect | No unwanted auto-retry after Disconnect; saved-bond reconnection works |
| Cluster Forget while connected | Link closes; app updates; no reconnect loop or fake Connected; fresh pairing requires both-side cleanup as needed |
| Bluetooth/permissions loss | Native work cancels, truthful status and retry after restoring access |
| Other app occupies sole slot | Clear bounded failure; no second connection or misleading paired-success |
| Lifecycle/callback race | Dismissal/resume/rapid taps do not leak GATT/receivers or accept an obsolete callback |
| UI regression | Approved radar/home, dark/light, large text, reduced motion and unavailable states preserved |

Report app/firmware revisions, Android/device identity, exact steps, observed native results and any remaining failures. Do not include pairing codes, keys or phone contents in the report. Build-only checks are not physical acceptance.

## If an attempt disconnects

The firmware developer can run this on the Mac while reproducing the Android attempt (port may change):

```bash
/Users/codewavesandy/.platformio/penv/bin/python tools/vehicle/ble_log.py /dev/cu.usbmodem1101
```

Capture the app's numeric GATT status and bond transition plus relevant `@BLE` diagnostics. Firmware `@BLE SECURITY peer-procedure-active` means the existing Android pairing procedure is being preserved, not rejected. A missing `@BLE FAULT` line alone does not prove connection success. Do not fix an unknown failure by disabling code approval, removing MITM/security or erasing NVS/bonds.

## Completion handoff

Commit only the app implementation/docs in the app workstream, following the user's Git instructions. Give the firmware developer a short record of manual results and any required contract changes. This milestone is complete when actual first pairing/name delivery/live connection and foreground saved reconnect pass on hardware. Final-product Wi-Fi/data/navigation functionality remains separate work, not implied by BLE bonding.
