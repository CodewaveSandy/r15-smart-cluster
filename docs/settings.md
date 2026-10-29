# Settings and connectivity

Updated: 2026-10-29. Settings home, Appearance and unified Phone UI implemented; Navigation and ESP32 adapters pending.

## Visual direction

The user selected the category-grid concept in [settings-concept.png](reference/settings-concept.png). The image is an AI-generated design reference, not evidence of connected radios. Actual host screens retain all dashboard tell-tales below the persistent gear/speed/clock strip. The four categories are Appearance, Phone, Navigation and Clock & Date, with a clear theme-accent focus outline. Both Midnight and Daylight are implemented.

## Navigation and access

SettingsHome owns a persistent four-tile tree. SettingsNavigation reduces semantic actions without LVGL, SDL, storage or radio dependencies. The host maps S to Open, Right/Down to next, Left/Up to previous, I to next, Enter to Select, and Backspace/Esc to Back. Focus wraps. Enter opens Appearance, Phone or Clock & Date. Navigation remains pending. Phone status follows normalized phone service observations.

Prototype Settings access is unrestricted by speed, data quality, UART status or warnings at the user's request (2026-10-29). Tapping the gear opens it after boot even without a speed observation. Warnings remain live in the persistent tell-tale row; opening Settings does not fabricate stationary speed, neutral or lamps and does not change the UART/demo source. Lifecycle shutdown/off still owns screen visibility and cancels unconfirmed previews. Physical Trip/Info inputs already route over UART; radio commands remain a future integration. This temporary prototype access policy is not a reviewed riding-use policy; restrictions can be added later.

## Touch navigation

A 44 × 52 px gear target at the top-right beside the clock/date opens Settings through the same access reducer as physical/keyboard controls. It returns Back when Settings is already open. Touch release selects category tiles, Appearance choices/Apply/Cancel, Phone action rows and Back; content taps are ignored during the transition. Speed, stale/unknown data and warnings do not block entry in this prototype; boot/shutdown still controls visibility. Non-touch navigation remains supported. Native display serial controls are `s` open, `q` Back, `[`/`]` previous/next and Enter select. Display-owned Appearance persistence uses NVS; real radios remain unavailable. Apply closes only after confirmed save.

## Instrument transition

The same persistent speed label travels between the dashboard and top strip over 280 ms, easing position and scaling the Street numerals to approximately the existing 46 px secondary size. Track speed retains its secondary size and moves to the same strip. The unit follows the visible glyph edge. Reversing the request preserves the current transition position. Normal dashboard geometry is restored exactly. Gear and tell-tales remain live; current/best lap return with Track instrumentation.

A monotonic portable transition runs under the existing LVGL owner. No widget timer, task or per-update application allocation is added. Hidden dashboard instruments defer updates until shown, then consume the latest observation. The instrument group is hidden without a full-screen alpha layer; category tiles appear at the settled endpoint to avoid a full-menu opacity redraw during every movement frame. Transformed speed/leading-zero labels use tight text bounds to reduce temporary layer work. LVGL may allocate bounded intermediate layers for text transforms. Actual target heap, render/scanout timing and DMA/PSRAM behavior remain hardware gates.

## Appearance

Sequential focus order: Midnight, Daylight, Instrument design, Cancel, Apply. Enter on a theme previews it immediately across the interface. Only Instrument is offered as the current design; there are no fake alternate designs. The three-column composition stacks Midnight/Daylight choices in the first column and spans the remaining two columns with one live preview. The preview composes the actual Header, TellTaleRow, Tachometer, PrimaryReadout, RideInfoPanel and FuelGauge with fixed illustrative samples, following the selected theme, Street/Track layout and units. It has no service, timer or nested menus. The only design remains Instrument.

Cancel or Back restores the theme from entry and returns to Settings home. Apply emits a typed SavePreferences effect to the host adapter. Success closes Appearance and retains the choice; failure keeps the preview open with a visible retry message. Retrying Apply, selecting a different theme or cancelling remains possible. Back/Cancel or a lifecycle power transition cancels an unconfirmed preview; motion/data-quality/warnings do not close the prototype menu.

## Confirmed preferences

Preferences.h contains confirmed theme/design plus clock format, date format and timezone preferences. Menu focus, current screen, preview state, telemetry, trips and vehicle counters are not persisted. Actual time is not persisted as if it could keep ticking while unpowered.

The host/native adapters load before the first dashboard update and save through Apply, skipping unchanged choices. Version 2 uses 16 explicit bytes: R15P magic, version, theme, design, twelve-hour flag, date-format enum, automatic-timezone flag, signed little-endian UTC offset minutes, three zero reserved bytes and an XOR integrity byte. Loading accepts validated v2 records or the legacy eight-byte v1 theme/design record; legacy records retain their theme and receive default clock preferences. Missing/malformed data falls back without automatic overwrite or erase. No native struct memory is stored.

Mac path: `~/Library/Application Support/R15 Smart Cluster/preferences.bin`. `R15_SETTINGS_PATH` overrides it for an isolated preview/test. On other hosts, use XDG_CONFIG_HOME or `~/.config/r15-smart-cluster/`. Render tests do not load or modify a user's settings.

PreferenceFile performs host filesystem operations outside LVGL rendering, using a flushed/closed temporary sibling followed by atomic replacement. Failed writes report failure and preserve the confirmed file. This does not claim hardware power-loss durability. The native display loads the same validated versioned record from NVS before its first frame. Apply queues a single request to a dedicated bounded storage worker; only confirmed commit updates saved preferences and closes the requesting Appearance/Clock page. Repeated Apply is suppressed while Saving is visible; unchanged confirmed choices skip writes. Failed initialization/open/write/commit stays a visible retryable failure and never erases NVS automatically. Cancelling while a write is pending does not cancel a commit already underway or close a subsequently reopened screen. The UI never calls NVS. Actual panel restart, power-cut durability and flash/render coexistence remain hardware tests.

## Navigation and future services

Navigation remains pending. The selected direction is a custom moving map with an Android-first Flutter companion app and future iOS support; Android Auto/CarPlay are not required. The unified Phone category replaces rider-facing Bluetooth/Wi-Fi setup. See the [phone journey](../mobile/docs/user-journey.md). GPS position alone does not provide map rendering. USB clock synchronization and clock-format/timezone editing are implemented; a retained RTC, phone time and vehicle network time remain pending. Submenus share Next/Previous/Select/Back; touch can be another input adapter, while physical-switch navigation remains a requirement. Text entry needs a separate provisioning decision.

## Verification

Portable checks cover unrestricted access across speed/quality/warnings, lifecycle closure, four-category focus wrapping, Apply/Cancel, save effects/failure, clock/date/timezone boundaries and transition reversal. Host storage checks cover restart round trips, malformed sizes, invalid record fields/checksum, atomic replacement, failed replacement preserving the confirmed file and storage failure. Tests use a separate temporary directory under build output.

Render scenarios cover Settings/Appearance, theme previews, focused design, save failure, entry/exit, urgent warnings and stale speed for both layouts/themes, exact dashboard restoration, and zero idle redraws on Appearance, Settings and dashboard. Host rendering and persistence are not board or bike verification.

## Phone connection

Settings → Phone retains gear, compact speed, clock and the full tell-tale row in both layouts/themes. The page shows unavailable, ready to pair, first-pair request/code, awaiting phone completion, saved/disconnected and connected states. Prototype UI pairing actions no longer require stationary speed; service availability, authentication and explicit pairing approval still apply. A pending request defaults to Reject. Forget confirmation defaults to Cancel. Unknown service state exposes Back only. Lifecycle transitions restore instruments and reject pending pairing; warnings/stale speed leave the prototype menu open; a request already accepted cannot be undone by a later Back.

The app owns discovery and chooses the transport. One cluster Phone page replaces both radio menus; there is no rider-facing SSID scan or Bluetooth device list. PhonePair/Approve/Reject/Disconnect/Forget are semantic effects; they never mutate confirmed observations optimistically. Display widgets do not parse packets, manage bonds, store secrets or call radios.

The [nearby connection guide](../mobile/docs/mac-preview-connection.md) documents actual Android BLE discovery, Mac BLE advertising, first-time approval and saved automatic reconnect. The opt-in Mac preview uses BLE for public discovery and Wi-Fi TLS for the session, with protected Keychain/Android Keystore persistence. Independent ESP32 BLE/Wi-Fi adapters and access-point provisioning remain pending. The plain host preview has an unavailable Phone service; no fake phone is discovered. Legacy portable radio adapter fixtures remain tests, not Settings categories.

Portable Phone tests cover state validation, focus, prototype pairing access, no optimistic connection changes, confirmation default and interruption. Deterministic 800 x 480 render scenarios exercise unpaired, saved, connected, forget confirmation, unavailable/stale and pending/approved pairing across both layouts/themes, plus warnings/stale-speed visibility and lifecycle interruption and idle redraw checks.

In native firmware, tapping the gear opens Settings without replacing live observations. Explicit serial `p` remains an optional stationary demo fixture, with only speed synthesized; it is never required for menu access. `v` restores live UART.

## Clock, date and connection indicator

The clock/date area belongs to real wall time. The gear icon communicates the vehicle UART state: theme foreground for fresh accepted packets, amber before a packet, red after the existing one-second expiry. This indicates link freshness, not validity of every vehicle source. UART counters remain in the console. An independent BENCH DEMO header label identifies explicit synthetic mode without occupying the date.

Clock & Date offers 12/24-hour display (separate AM/PM in 12-hour mode), DD/MM/YYYY, MM/DD/YYYY or YYYY-MM-DD, and either the Mac-supplied UTC offset or a manual offset from UTC-12:00 to UTC+14:00 in 15-minute steps. Changes preview immediately; Apply saves through the bounded NVS worker, Cancel/Back restores entry preferences. Manual offsets do not implement a geographic timezone/DST rule database. Automatic mode follows the Mac's current offset, including its current daylight-saving setting, on each sync.

For this bench phase, run `bash scripts/sync-clock.sh /dev/cu.usbserial-10` after flashing the display (replace the port as needed). Keep it running. It sends the current UTC epoch and local offset over USB UART0 every 30 seconds and in response to `@CLOCK REQUEST`, and prints confirmation only after `@CLOCK OK <epoch>`. It requests exclusive serial access where supported; stop it before flashing or opening another console. Opening a USB serial port may reset some boards; the helper responds to the new boot request. No Android build or network connection is needed.

The bounded console frame is `@TIME <UTC epoch seconds> <signed offset minutes>\n`; accepted epochs are 2020–2099 UTC, offsets -720 through +840 minutes. Invalid/oversized frames are discarded without turning their contents into UI keys. Clock durations use monotonic time; wall-time correction never changes boot, shutdown or button timing. Display formatting is minute-driven and separate from signal freshness.

The display keeps ticking in RAM while powered after sync. Cold power loss invalidates time; it requests sync and shows CLOCK NOT SYNCED until a real source arrives. Saving the last date to flash cannot account for time spent unpowered. Automatic cold-start time without the Mac helper requires future phone/network time or a validated battery-backed RTC. No retained-clock claim is made.
