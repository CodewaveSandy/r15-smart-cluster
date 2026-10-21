# Settings and connectivity

Updated: 2026-10-19. Settings home, Appearance, host preferences, Bluetooth and Wi-Fi UI/demo implemented; Navigation and ESP32 adapters pending.

## Visual direction

The user selected the category-grid concept in [settings-concept.png](reference/settings-concept.png). The image is an AI-generated design reference, not evidence of connected radios. Actual host screens retain all dashboard tell-tales below the persistent gear/speed/clock strip. The four categories are Appearance, Bluetooth, Wi-Fi and Navigation, with a clear theme-accent focus outline. Both Midnight and Daylight are implemented.

## Navigation and access

SettingsHome owns a persistent four-tile tree. SettingsNavigation reduces semantic actions without LVGL, SDL, storage or radio dependencies. The host maps S to Open, Right/Down to next, Left/Up to previous, I to next, Enter to Select, and Backspace/Esc to Back. Focus wraps. Enter opens Appearance, Bluetooth or Wi-Fi. Navigation remains pending. Radio home statuses follow service observations.

A valid speed observation from 0 up to (but excluding) 20 km/h, Running lifecycle and no valid urgent warning are required to open Settings. Speed at/above 20 km/h, unavailable/stale speed, urgent warning or power transition close it. Unconfirmed Appearance previews are cancelled on interruption. The renderer independently enforces instrument visibility so a stale view option cannot hide readings. Physical input detection and the vehicle command channel are unimplemented. The 20 km/h threshold is user-selected UI behavior, not a validated safe riding speed. The gate always uses normalized km/h even when display units are imperial; future pairing/text-entry restrictions require their own review.

## Instrument transition

The same persistent speed label travels between the dashboard and top strip over 280 ms, easing position and scaling the Street numerals to approximately the existing 46 px secondary size. Track speed retains its secondary size and moves to the same strip. The unit follows the visible glyph edge. Reversing the request preserves the current transition position. Normal dashboard geometry is restored exactly. Gear and tell-tales remain live; current/best lap return with Track instrumentation.

A monotonic portable transition runs under the existing LVGL owner. No widget timer, task or per-update application allocation is added. Hidden dashboard instruments defer updates until shown, then consume the latest observation. The instrument group is hidden without a full-screen alpha layer; category backgrounds/borders and bounded labels fade separately. LVGL may allocate bounded intermediate layers for text transforms. Actual target heap, render/scanout timing and DMA/PSRAM behavior remain hardware gates.

## Appearance

Sequential focus order: Midnight, Daylight, Instrument design, Cancel, Apply. Enter on a theme previews it immediately across the interface. Only Instrument is offered as the current design; there are no fake alternate designs. The previews use native widgets and bundled fonts rather than full-screen bitmap assets. Thumbnail numerals use a dedicated native 28 px Chakra subset, smaller than live instruments and rasterized directly rather than scaled.

Cancel or Back restores the theme from entry and returns to Settings home. Apply emits a typed SavePreferences effect to the host adapter. Success closes Appearance and retains the choice; failure keeps the preview open with a visible retry message. Retrying Apply, selecting a different theme or cancelling remains possible. Leaving all Settings through motion/data quality/warnings/power cancels an unconfirmed preview.

## Confirmed preferences

Preferences.h contains only display-owned confirmed theme and design identity. Menu focus, current screen, preview state, telemetry, trips and vehicle counters are not persisted. Other display options remain session-only in this slice.

The host adapter loads preferences before the first dashboard update and saves only through Apply. Unchanged confirmed choices skip the write. The version-1 record is an explicit eight-byte encoding: R15P magic, version, theme, design and XOR integrity byte. Loading requires exactly eight bytes and recognized values/version; missing or malformed data falls back to defaults without automatically overwriting it. No native C++ struct memory is written.

Mac path: `~/Library/Application Support/R15 Smart Cluster/preferences.bin`. `R15_SETTINGS_PATH` overrides it for an isolated preview/test. On other hosts, use XDG_CONFIG_HOME or `~/.config/r15-smart-cluster/`. Render tests do not load or modify a user's settings.

PreferenceFile performs host filesystem operations outside LVGL rendering, using a flushed/closed temporary sibling followed by atomic replacement. Failed writes report failure and preserve the confirmed file. This does not claim hardware power-loss durability. Display-owned ESP-IDF NVS, target storage scheduling and hardware persistence tests remain firmware work; the UI never calls a filesystem or NVS API.

## Navigation and future services

Navigation remains pending. The selected direction is a custom moving map with an Android-first Flutter companion app and future iOS support; Android Auto/CarPlay are not required. The planned unified Phone category replaces rider-facing Bluetooth/Wi-Fi setup after implementation and visual review; existing simulated pages below remain the current implementation. See the [phone journey](../mobile/docs/user-journey.md). GPS position alone does not provide map rendering. Clock editing and actual time-source/retention also remain pending. Submenus share Next/Previous/Select/Back; touch can be another input adapter, while physical-switch navigation remains a requirement. Text entry needs a separate provisioning decision.

## Verification

Portable checks cover speed/quality/lifecycle/warning gates, focus wrapping, Apply/Cancel, save effects/failure, preview interruption, transition reversal and immediate interruption. Host storage checks cover restart round trips, malformed sizes, invalid record fields/checksum, atomic replacement, failed replacement preserving the confirmed file and storage failure. Tests use a separate temporary directory under build output.

Render scenarios cover Settings/Appearance, theme previews, focused design, save failure, entry/exit, urgent warnings and stale speed for both layouts/themes, exact dashboard restoration, and zero idle redraws on Appearance, Settings and dashboard. Host rendering and persistence are not board or bike verification.

## Bluetooth

Host UI and timed demo implemented; no Bluetooth stack, phone pairing keys or UART command transport implemented.

### Screen and ownership

Open Settings -> Bluetooth. The persistent gear/speed strip and tell-tales remain visible. Settings home reflects the latest Bluetooth status. The page shows a bounded current-device name, confirmed status and context-dependent actions. Both themes and Street/Track contexts are supported.

BluetoothState is a portable normalized radio observation alongside telemetry. Quality is shared through shared/model/Quality.h. Phase, failure, paired/connected flags and a bounded 32-byte terminated name are validated before presentation. Non-current quality produces Status unavailable and only Back, never a false disconnected state or an actionable Forget. Names render at most 24 supported ASCII characters; unsupported display text uses an explicit fallback instead of missing glyphs.

BluetoothMenu and SettingsNavigation produce typed request effects. Widgets format and render observations; they do not own radios, mutate bond/connection observations, access storage or decode UART. The vehicle MCU remains the proposed radio owner. Requests do not themselves prove success: Disconnect retains the displayed live connection until a confirming observation, and Forget retains the bond until completion.

### Actions and states

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

### Host demo

The page is marked SIMULATED PHONE SERVICE. It starts with the generic Rider phone connected, matching the existing host phone/battery sample. Pairing, reconnect, disconnect and Forget are timed fixture transitions under the host's monotonic clock. There are no real radio calls, discovered-device scans, pairing secrets or network transmissions.

- P sets the demo stationary; S opens Settings; Right focuses Bluetooth; Enter opens it.
- Right/Down and Left/Up cycle actions; Enter selects; Backspace/Esc goes back.
- Pair new device progresses through Ready to pair and Pairing before Connected.
- F3 makes the next Bluetooth operation time out; use the displayed retry action afterward.
- F4 toggles unavailable service status for review.
- Space resets demo telemetry/radio state and clears pending demo work. Saved Appearance preferences are unaffected.

Only one generic phone fixture is represented. Demo bond/connection state resets on restart; it is not part of the display preference file. A cancelled pair/connect never completes from an old timer. A failed disconnect/Forget preserves the last confirmed link/bond rather than reporting optimistic success.

### Required before real integration

Confirm the vehicle MCU, supported Bluetooth roles/profiles, discovery direction, authentication/prompt requirements, number of paired devices and phone application requirements. This UI does not establish Classic Bluetooth, audio, calls, projection or Google Maps capability. Pairing instructions are generic pending that review.

Define device/session identity, command request IDs, acknowledgements/rejection, cancellation/idempotence, timeouts, source freshness and reconnect behavior before binding these effects to UART/radio operations. In particular, real Forget must be bound to the confirmed device identity; a display name is not an identifier. Pairing keys and bond persistence belong with the reviewed radio service, not the display theme/design file. No wire packet format or hardware facts are invented by this UI slice.

### Verification

Portable checks cover state/name validation, unavailable quality, no optimistic state mutation, cancellation and late completion, reconnect, confirmed Forget, failure/retry and preserving confirmed connection on failed disconnect. Deterministic 800 x 480 captures cover 14 Bluetooth states plus warning/stale-speed interruption in both modes/themes. Settled Bluetooth, Settings, Appearance and dashboard frames produce zero redundant flushes. Actual pairing and target timing/resource use require hardware tests.

## Wi-Fi

Radio behavior is simulated on the native LVGL host. Vehicle MCU Wi-Fi drivers, provisioning, credential storage and UART commands are pending.

### Screens and controls

Settings → Wi-Fi retains live gear, compact speed, clock and all tell-tales. Midnight and Daylight share the same geometry in Street and Track. The list contains at most three fixture networks plus Search again, with Saved / Protected / Open labels. This bounded host list is not a claim of production scan capacity; paging or a selected scan window needs a later protocol design.

Select a network to request connection. The saved Rider hotspot fixture and open Guest network connect after a simulated delay. The protected, unsaved Garage Wi-Fi first shows “Credentials pending” and an explicit Simulate connection action. Back cancels that notice. No password is collected, sent, logged or persisted; this is a visual placeholder for provisioning, not authentication. The real credential-entry method and phone hotspot/peer role remain unresolved.

Connected status shows the network name, Disconnect, Forget network and Search again. Disconnect preserves the demo's saved network. Forget requires confirmation with Cancel initially focused; completion removes the connected fixture's saved flag and disconnects. Failed requests preserve confirmed connection state. Scanning/connecting/disconnecting/forgetting, failed, empty, unavailable and stale states are distinct. Unknown status offers Back only, never a false disconnected claim.

Arrow keys cycle actions sequentially; Enter selects; Backspace/Esc goes back. Pending scan/connect is cancelled on leaving. Accepted disconnect/forget can finish after leaving; cancellation does not undo them. Speed at/above 20 km/h, stale/unknown speed, urgent warning or leaving Running closes Settings under the existing policy. The threshold is user-selected UI behavior, not a validated safe riding speed.

### Preview

Run `bash scripts/preview.sh`, then **P → S → Right → Right → Enter**. F5 fails the next Wi-Fi request, F6 toggles unavailable status, Space resets fixtures. Network choices are session-only and never enter the Appearance preference file. Search repeats the fixed host fixture; it does not discover real networks.

### Ownership and integration gates

`WifiState` is a bounded, quality-bearing portable observation. It validates list size, enum bounds and terminated 32-byte SSID storage. Unsupported font bytes or names over 24 characters use a visible fallback; names are not stable network identifiers. `WifiMenu` and SettingsNavigation emit semantic request effects; WifiPanel only renders. The single LVGL owner remains unchanged. WifiDemo handles host-only timing and outcomes outside rendering.

Before real connection support, define stable network identifiers (including same-name networks), scan generation/paging, capabilities/security, request correlation, acknowledgement/timeout/cancellation and session recovery. Bind confirmation and network selection to those identities rather than mutable list positions. Define and review provisioning and vehicle-owned credential protection/storage. No native structs are a wire format. Wi-Fi connection does not establish internet, Maps or projection support.

### Verification

Wi-Fi tests cover unknown status, model bounds, requested versus observed state, cancellation, connection completion, confirmation default, failure retaining an established link, forgetting and access interruption. Wi-Fi render coverage includes 14 states/interruption captures in each layout/theme; the current suite summary is in the [implementation plan](implementation-plan.md). Stable Wi-Fi fixtures produce zero flushes over 100 unchanged updates. Host success is not ESP32 resource or vehicle evidence.
