# Settings and connectivity

Updated: 2026-10-21. Settings home, Appearance and unified Phone UI implemented; Navigation and ESP32 adapters pending.

## Visual direction

The user selected the category-grid concept in [settings-concept.png](reference/settings-concept.png). The image is an AI-generated design reference, not evidence of connected radios. Actual host screens retain all dashboard tell-tales below the persistent gear/speed/clock strip. The three categories are Appearance, Phone and Navigation, with a clear theme-accent focus outline. Both Midnight and Daylight are implemented.

## Navigation and access

SettingsHome owns a persistent three-tile tree. SettingsNavigation reduces semantic actions without LVGL, SDL, storage or radio dependencies. The host maps S to Open, Right/Down to next, Left/Up to previous, I to next, Enter to Select, and Backspace/Esc to Back. Focus wraps. Enter opens Appearance or Phone. Navigation remains pending. Phone status follows normalized phone service observations.

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

Navigation remains pending. The selected direction is a custom moving map with an Android-first Flutter companion app and future iOS support; Android Auto/CarPlay are not required. The unified Phone category replaces rider-facing Bluetooth/Wi-Fi setup. See the [phone journey](../mobile/docs/user-journey.md). GPS position alone does not provide map rendering. Clock editing and actual time-source/retention also remain pending. Submenus share Next/Previous/Select/Back; touch can be another input adapter, while physical-switch navigation remains a requirement. Text entry needs a separate provisioning decision.

## Verification

Portable checks cover speed/quality/lifecycle/warning gates, focus wrapping, Apply/Cancel, save effects/failure, preview interruption, transition reversal and immediate interruption. Host storage checks cover restart round trips, malformed sizes, invalid record fields/checksum, atomic replacement, failed replacement preserving the confirmed file and storage failure. Tests use a separate temporary directory under build output.

Render scenarios cover Settings/Appearance, theme previews, focused design, save failure, entry/exit, urgent warnings and stale speed for both layouts/themes, exact dashboard restoration, and zero idle redraws on Appearance, Settings and dashboard. Host rendering and persistence are not board or bike verification.

## Phone connection

Settings → Phone retains gear, compact speed, clock and the full tell-tale row in both layouts/themes. The page shows unavailable, ready to pair, first-pair request/code, awaiting phone completion, saved/disconnected and connected states. Pairing access/approval requires valid stationary speed; existing Settings entry remains below 20 km/h. A pending request defaults to Reject. Forget confirmation defaults to Cancel. Unknown service state exposes Back only. Warnings, stale speed and lifecycle transitions restore instruments immediately and reject pending pairing; a request already accepted cannot be undone by a later Back.

The app owns discovery and chooses the transport. One cluster Phone page replaces both radio menus; there is no rider-facing SSID scan or Bluetooth device list. PhonePair/Approve/Reject/Disconnect/Forget are semantic effects; they never mutate confirmed observations optimistically. Display widgets do not parse packets, manage bonds, store secrets or call radios.

The [nearby connection guide](../mobile/docs/mac-preview-connection.md) documents actual Android BLE discovery, Mac BLE advertising, first-time approval and saved automatic reconnect. The opt-in Mac preview uses BLE for public discovery and Wi-Fi TLS for the session, with protected Keychain/Android Keystore persistence. Independent ESP32 BLE/Wi-Fi adapters and access-point provisioning remain pending. The plain host preview has an unavailable Phone service; no fake phone is discovered. Legacy portable radio adapter fixtures remain tests, not Settings categories.

Portable Phone tests cover state validation, focus, stationary pairing, no optimistic connection changes, confirmation default and interruption. Deterministic 800 x 480 render scenarios exercise unpaired, saved, connected, forget confirmation, unavailable/stale and pending/approved pairing across both layouts/themes, plus warnings/stale-speed interruption and idle redraw checks.
