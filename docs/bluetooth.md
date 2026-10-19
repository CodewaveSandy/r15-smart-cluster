# Bluetooth settings UI

Updated: 2026-10-19. Host UI and timed demo implemented; no Bluetooth stack, phone pairing keys or UART command transport implemented.

## Screen and ownership

Open Settings -> Bluetooth. The persistent gear/speed strip and tell-tales remain visible. Settings home reflects the latest Bluetooth status. The page shows a bounded current-device name, confirmed status and context-dependent actions. Both themes and Street/Track contexts are supported.

BluetoothState is a portable normalized radio observation alongside telemetry. Quality is shared through shared/model/Quality.h. Phase, failure, paired/connected flags and a bounded 32-byte terminated name are validated before presentation. Non-current quality produces Status unavailable and only Back, never a false disconnected state or an actionable Forget. Names render at most 24 supported ASCII characters; unsupported display text uses an explicit fallback instead of missing glyphs.

BluetoothMenu and SettingsNavigation produce typed request effects. Widgets format and render observations; they do not own radios, mutate bond/connection observations, access storage or decode UART. The vehicle MCU remains the proposed radio owner. Requests do not themselves prove success: Disconnect retains the displayed live connection until a confirming observation, and Forget retains the bond until completion.

## Actions and states

| Observed state | Actions |
| --- | --- |
| No paired device | Pair new device |
| Paired, disconnected | Connect, Pair new device, Forget device |
| Connected | Pair new device, Disconnect, Forget device if bonded |
| Discoverable / pairing / connecting | Cancel pending pairing/connection |
| Disconnecting / forgetting | Back while the accepted operation finishes |
| Failed request | Retry relevant connection/pairing, plus valid current-device actions |
| Unavailable / stale / unsupported / invalid | Back |

Forget opens a confirmation page with Cancel selected by default. Back cancels confirmation; selecting Forget there emits the request. An accepted Forget operation is not undone by subsequently leaving the page. Disconnect preserves pairing; successful Forget clears both connection and saved bond in the observation.

Back from Bluetooth returns to Settings and requests cancellation of a pending pair/connect, without disconnecting an established link. Motion at/above 20 km/h, stale speed, urgent warnings or lifecycle changes close Settings and issue the same pending-operation cancellation. The 20 km/h threshold remains user-selected UI behavior, not a verified safe riding speed.

## Host demo

The page is marked SIMULATED PHONE SERVICE. It starts with the generic Rider phone connected, matching the existing host phone/battery sample. Pairing, reconnect, disconnect and Forget are timed fixture transitions under the host's monotonic clock. There are no real radio calls, discovered-device scans, pairing secrets or network transmissions.

- P sets the demo stationary; S opens Settings; Right focuses Bluetooth; Enter opens it.
- Right/Down and Left/Up cycle actions; Enter selects; Backspace/Esc goes back.
- Pair new device progresses through Ready to pair and Pairing before Connected.
- F3 makes the next Bluetooth operation time out; use the displayed retry action afterward.
- F4 toggles unavailable service status for review.
- Space resets demo telemetry/radio state and clears pending demo work. Saved Appearance preferences are unaffected.

Only one generic phone fixture is represented. Demo bond/connection state resets on restart; it is not part of the display preference file. A cancelled pair/connect never completes from an old timer. A failed disconnect/Forget preserves the last confirmed link/bond rather than reporting optimistic success.

## Required before real integration

Confirm the vehicle MCU, supported Bluetooth roles/profiles, discovery direction, authentication/prompt requirements, number of paired devices and phone application requirements. This UI does not establish Classic Bluetooth, audio, calls, projection or Google Maps capability. Pairing instructions are generic pending that review.

Define device/session identity, command request IDs, acknowledgements/rejection, cancellation/idempotence, timeouts, source freshness and reconnect behavior before binding these effects to UART/radio operations. In particular, real Forget must be bound to the confirmed device identity; a display name is not an identifier. Pairing keys and bond persistence belong with the reviewed radio service, not the display theme/design file. No wire packet format or hardware facts are invented by this UI slice.

## Verification

Portable checks cover state/name validation, unavailable quality, no optimistic state mutation, cancellation and late completion, reconnect, confirmed Forget, failure/retry and preserving confirmed connection on failed disconnect. Deterministic 800 x 480 captures cover 14 Bluetooth states plus warning/stale-speed interruption in both modes/themes. Settled Bluetooth, Settings, Appearance and dashboard frames produce zero redundant flushes. Actual pairing and target timing/resource use require hardware tests.
