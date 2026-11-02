# UI, Settings and lifecycle

Updated: 2026-10-19. Dashboard, lifecycle, Settings, Appearance and unified Phone screens implemented. Navigation, retained clock sources and full hardware integration remain pending; phone/USB clock sync, persisted formats and connected phone metadata are implemented.

## Visual authority

Implement the geometry and behavior specified here. Capture deterministic 800 x 480 states in the LVGL host build for visual review. Match geometry and hierarchy; allow documented RGB565 quantization/font rasterizer differences after inspection. Never substitute different typefaces silently.

## Layout and typography

| Element | Specified baseline |
| --- | --- |
| Canvas and content width | 800 x 480; left/right margin 28; usable width 744 |
| Header | Gear number/N only (no GEAR caption); mode, QS/VVA, phone/battery, clock/date; divider y=85; top-right 44 × 52 px Settings gear, clock/date shifted left to leave an 8 px gap |
| Gear / mode / clock | Chakra Petch 72 px Bold Italic / Inter 23 px SemiBold / Barlow 30 px SemiBold |
| Tell-tales | Row y=98; arrows at opposite edges |
| Tachometer | x=28, y=144, width=744; fill begins 20 px below component top |
| RPM fill height | 44 px in both modes |
| Primary readout | Street: centered speed, 128 px; Track: centered current lap mm:ss.hh, 74 px |
| Track speed | Secondary readout on the right at x=560,y=269; 46 px with unit beside the digits |
| Lower divider | y=376 in both modes |
| Lower information | Street trip list / Track best lap left; info list middle; fuel right. Shared title y=399, value y=421, fuel segments y=440 |
| Footer | No help/status bar or bottom divider; diagnostics use the header subtitle/window/terminal |

Use bundled Inter Medium/SemiBold, Barlow Semi Condensed SemiBold and Chakra Petch Bold Italic under the specified OFL license. Choose font sizes from observed rendered metrics; validate nominal font sizes against actual rendered baselines. Centralize text styles, tracking, line heights and alignment. Reserve fixed bounds for changing digits and use bounded formatting. Small-font subsets need all actual menu/date/unit/error glyphs, degree sign and unavailable dash; large-font subsets need their actual digits/N/dash/punctuation. Keep font sources, converter version, command and output size reproducible.

## Theme tokens

| Token | Midnight | Daylight |
| --- | --- | --- |
| Background | #090e14 | #eff3f5 |
| Foreground | #ffffff | #000000 |
| Muted | #91a4b7 | #4d6577 |
| Panel | #14202c | #e0e8ec |
| Divider | #253544 | #c7d3db |
| Accent | #78e4c1 | #007757 |
| Amber | #f5d45b | #946000 |
| Red | #ff7980 | #bf263a |
| High beam | #77baff | #246bb0 |
| RPM bed | #14202c | #c2cfd6 |
| RPM green / amber / red | #4cdb88 / #f5d45b / #f06464 | #00774b / #946000 / #bf263a |

Information text (ODO/trip/best lap, coolant/consumption/average-speed captions, FUEL/RESERVE, normal fuel percentage, units, RPM ticks below 11, header date/phone text and lap/speed captions) uses pure white in Midnight and pure black in Daylight. Warning/unknown status, redline ticks, active indicators and low-fuel severity retain their semantic colors. Shift-background frames use white instrument text in both themes. Centralize palette, spacing, geometry, opacity, fonts and timing in typed tokens. Portable display thresholds are in embedded/presentation/PresentationPolicy.h; rendering Metrics reference them. Shared widget constructors use tokens; screens compose widgets rather than copying style logic. Theme changes never change telemetry or riding behavior. Preserve the specified red shift background gradient (#a51c30 -> #f04444 -> #c52636) and readable warning palette; profile its redraw cost.

## Instrument behavior

- Scale labels 11, 12 and 13 use the theme red as a user-selected redline cue. This does not change fill colors, shift thresholds or establish a verified vehicle limit.
- Continuous RPM gradient anchored to the entire 0–13,000 scale; clip fill, do not stretch colors with the current RPM. Midnight uses green #4cdb88 through 3,500; blends to #f5d45b at 4,500; holds through 7,500; blends to #f06464 at 8,500, then holds. Daylight uses the same fixed stops with darker green #00774b, amber #946000 and red #bf263a over a distinct #c2cfd6 bed, improving visible fill contrast on the delivered panel. Shift-background frames retain the bright Midnight ramp for contrast on red/dark artwork. This shared theme treatment applies to actual observations, host and bench demo alike.
- Shift background steady from 10,500 RPM; alternate red/normal every 300 ms from 11,400. Stop immediately below thresholds; unavailable RPM must not trigger it.
- Eight forward-slanted fuel pieces with a 4 px top/bottom offset. Each piece supports partial coverage: clamp(percent × 8 − pieceIndex × 100, 0, 100) coverage units, rather than rounding to a whole piece. Valid integer-percent observations move the fill in 1% steps; no invented intermediate measurements. Yellow at 10–20% inclusive, red below 10%; blink pump and filled segments every 500 ms below 20%. Exactly 20% is steady yellow; percentage stays readable; zero still blinks the pump.
- Tell-tales: 12% inactive, 100% active, no fades. Header phone pictogram and battery text are hidden unless a fresh connected observation is available (explicit icon-inspection mode is separate). Specified simulated turn blink is 450 ms. A future already-flashing vehicle signal must not be blinked a second time: distinguish source phase from steady intent in the contract.
- Preserve QS/VVA lettering and recognizable automotive symbol meanings. Simulator VVA is strictly above 7,500; real VVA support requires a verified observation/rule. No prototype RPM/fuel threshold is a verified Yamaha specification.
- User-supplied nominal tank capacity is 11 L and approximate reserve is 1.76 L, giving a 16% reserve cue. Show a fixed amber marker above the gauge at 16%; replace `FUEL` with steady `RESERVE` for valid readings at or below 16%. Unknown/stale data must not assert reserve. This percentage-based cue assumes normalized fuel percent represents remaining usable capacity; actual sensor/volume calibration remains vehicle-owned and unverified. Existing warning/blink thresholds remain separate from the reserve cue.

## Components and data flow

Dashboard composes Header, TellTaleRow, Tachometer, PrimaryReadout, RideInfoPanel, FuelGauge, SettingsHome, AppearancePanel, PhonePanel and LifecycleOverlay. Each exposes a small typed update API and owns its LVGL subtree; components apply changed values/styles only. Screens compose them; a presenter maps normalized state and quality to display values. Keep formatting testable without a display.

Implemented screens are Dashboard (Street/Track), Settings home, Appearance, Phone. Navigation remains pending; Clock & Date offers persisted formats/timezone and USB time sync. Developer help stays in Terminal. Hardware labels must describe actual available controls and time sources; touch is optional and cannot become required navigation.

Navigation actions: up/down/left/right/select/back/next/previous. Host keyboard and vehicle input adapters produce the same actions; no keycodes, GPIOs or CAN identifiers in widgets. Display settings/focus remain local. Remote trip/lap actions require the defined command contract and visible failures.

## Unknown and warning presentation

Unavailable measurements use dashes and clear data status, not zero/empty/neutral. Unsupported fields stay unavailable rather than plausible samples. A stale warning is not automatically off; define unknown indication separately from known inactive. Use this quality treatment consistently on real hardware. Warnings remain visible on relevant menu screens as well as the dashboard. Simultaneous warnings must remain distinguishable.

Valid warnings update on the next UI service cycle with no animation delay. RPM smoothing, if added later, is bounded, cancellable and never applied to warning truth. Use a small shared monotonic animation scheduler; no per-widget FreeRTOS tasks.

## Rendering discipline and acceptance

Use persistent LVGL objects and changed-field updates. Prefer built-in primitives and small static icon paths/masks; profile custom drawing before adopting caches. Cache the invariant RPM ramp if useful. Avoid full-screen image assets, repeated object rebuilding, expensive shadows and large translucent overlays. Keep the exact visual baseline before optimizing; inspect every optimization for regressions.

Required comparisons: both layouts x both themes; neutral and gears; 0/max/unknown speed/RPM; 10,499/10,500/11,399/11,400 shift boundaries; fuel 0/9/10/19/20/21/100 plus both blink phases; simultaneous warnings; phone disconnected and battery unknown; each data quality; settings focus and Appearance save/cancel; future clock editing needs date validation; link loss/recovery/reboot. Capture fixed clock and animation phase on host. Inspect native panel for tearing, color ordering, viewing angle and blink/readability at intended installation position.

## Boot and shutdown artwork

Boot lasts exactly 2.00 s and shutdown lasts exactly 2.00 s using the selected R15 Light Signature. Draw mirrored tapered blue slashes with crisp white cores around a small solid-white R15 wordmark; use a black backdrop, restrained glow and controlled opacity/retraction. Avoid flashes and oversized zooms. Preserve the dashboard at the reveal/fade endpoints and immediate urgent-warning interruption. See [power lifecycle](ui.md#power-lifecycle) for state, power and test requirements.

## Typography roles and preview quality

Inter Medium is the primary label font: 14 px general text, 16 px RPM label, 18 px speed units, and 12 px for compact date/ticks. Inter SemiBold at 23 px supplies mode/supporting values. The compact RPM readout uses 16 px Inter SemiBold digits with comma grouping (`8,500`) and a separate 16 px Inter Medium `RPM` suffix matching the `RPM × 1,000` caption. Both share a baseline at the original small-readout position; numerals use theme foreground (white in Midnight, dark in Daylight). Chakra Petch Bold Italic supplies Street speed (128 px), gear (72 px), Track speed (46 px) and current lap (74 px); Barlow Semi Condensed SemiBold retains clock (30 px). Chakra bitmaps receive a subtle half-pixel coverage expansion during generation, preserving advances/kerning; the 74 px timer fits the maximum value with separation from speed. The angular numeral treatment is R1M-inspired, not a factory-font claim. These choices favor restrained interface labels and distinctive instrument numerals.

Use embedded/ui/Typography.h roles; generate bitmap assets at their actual logical size using scripts/generate-fonts.sh. Maintain 4-bit antialiased coverage and full required glyph subsets. Bitmap generation does not enable OpenType features automatically; tabular-number behavior must be explicitly implemented/verified if needed. Keep changing values in stable bounds.

The SDL host preview uses linear texture filtering when enlarging the 800 x 480 framebuffer for Retina. This softens magnified pixel edges; it does not imply higher panel resolution or hardware text quality. Inspect native 800 x 480 captures and the actual panel separately. The icon artwork is a separate presentation responsibility.

## Independent lower lists

Street trip list: ODO -> Trip 1 -> Trip 2 -> ODO. Track replaces this entire panel with fixed BEST LAP, hides distance units, and does not cycle trip pages. Returning to Street restores its selected trip page. Info list: coolant -> Track mode -> real-time fuel consumption (L/100km) -> average fuel consumption (L/100km) -> average speed (km/h, or mph with imperial selected) -> TCS -> unidentified placeholder -> coolant. Each list keeps its own selection. Distance uses normalized km and display-unit conversion; unknown values remain dashes. TCS ON/OFF is an observation; the host preview may toggle a sample but real control requires a verified command contract. Track ON/OFF changes the UI layout only. All demo distances, consumption and average-speed values are fixed host samples, not functioning calculations or persistent vehicle data. Consumption and average speed are separate quality-bearing vehicle-owned observations; averaging windows and reset semantics remain unresolved. The unidentified page shows a dash and has no action until identified. Battery voltage is no longer an info-list page.

Available speed uses three digit positions in both layouts and the Settings strip: leading zeros use 16% theme-foreground opacity, while significant digits (including the final zero at rest) use full foreground. Unknown/stale speed remains a dash without placeholders.

Street speed remains visually centered on x=400 independently of digit count, including unknown data. Position its `km/h` or `mph` unit 10 px beside the visible right glyph edge, near the baseline; compensate italic side bearings when centering. Track puts current lap at x=400 in the 74 px Bold Italic font and moves speed to the right-hand secondary position. The lap caption is above its large readout; the speed unit sits beside the side speed. Fuel retains eight pieces over the existing width; a four-pixel skew creates the slight italic silhouette. Empty backgrounds stay visible behind the partially filled piece, and a fixed amber notch marks the approximate reserve threshold.

Current lap is an observation in milliseconds. Presentation formats mm:ss.hh with truncated hundredths; unknown/stale or durations beyond 99:59.99 show dashes rather than wrapping. The host provides a static 01:24.38 sample for layout review. This is not a functioning stopwatch.

Both modes share the same 44 px RPM height, y=376 lower divider and bottom-panel baselines. Track current-lap title/value are positioned at y=235/y=259 to balance the taller main section. Best lap is a separate normalized millisecond observation; the UI never calculates a record from displayed current-lap samples.

## Icon system

Use the selected Pictogrammers MDI automotive/status glyphs through typed Icon roles in embedded/ui/Icons.h. The symbol set includes proper high-beam rays, coolant thermometer/waves, engine, ABS ring, traction-control, message bubble, call handset, hazard triangle, solid turn arrows, phone and fuel pump. Neutral remains N. Use the 32 px native bitmap subset, with ABS at 30 px to avoid clipping its ring. Keep baseline placement and 12% inactive / 100% active tell-tale presentation consistent; the disconnected header phone indicator is hidden. No fades delay warning activation.

Bundle source version, hashes, Apache 2.0 license and notices. Only selected glyphs are linked; do not add a runtime SVG renderer or full icon font. The artwork is generic and requires separate OEM/vehicle validation before claiming exact conformity. F2 inspection covers both layouts/themes.

## Settings presentation

The implemented Settings flow follows [Settings behavior](ui.md#settings-behavior). Retain the header gear and full tell-tale row while moving speed to center x=200/y=22. Three category tiles use shared geometry and MDI glyphs; focus uses a 2 px accent border. A reversible 280 ms transition moves/scales the same speed label and follows with its unit. Normal layouts restore exactly. Prototype Settings has no speed/quality/warning access restriction per the user; warnings remain live and lifecycle transitions still own screen visibility. Appearance now offers Midnight/Daylight previews, one Instrument design, Apply/Cancel and host/native NVS saved preferences. Appearance stacks theme choices on the left with an actual-component live preview spanning the right two columns. Phone UI/demos follow the same focus/back navigation; Navigation and real radio provisioning remain pending; NVS power-loss/render coexistence still requires target testing.

Radio pages share the compact instrument strip, a status card and bounded context-sensitive action rows. Unknown service quality offers Back, and Forget confirmation defaults to Cancel. See [settings and connectivity](ui.md#settings-behavior) for both radio flows and provisioning boundaries.

Clock & Date is the fourth Settings tile. Keep real time/date in the header, with a separate AM/PM caption in 12-hour mode. Color the top-right gear foreground/amber/red for connected/waiting/stale UART; do not replace the date with UART text. See [Settings](ui.md#clock-date-and-connection-indicator) for source quality, supported formats and cold-power limitations.

## Settings behavior

Updated: 2026-10-29. Settings home, Appearance and unified Phone UI implemented; Navigation and ESP32 adapters pending.

### Visual direction

The user selected the category-grid concept in [settings-concept.png](reference/settings-concept.png). The image is an AI-generated design reference, not evidence of connected radios. Actual host screens retain all dashboard tell-tales below the persistent gear/speed/clock strip. The four categories are Appearance, Phone, Navigation and Clock & Date, with a clear theme-accent focus outline. Both Midnight and Daylight are implemented.

### Navigation and access

SettingsHome owns a persistent four-tile tree. SettingsNavigation reduces semantic actions without LVGL, SDL, storage or radio dependencies. The host maps S to Open, Right/Down to next, Left/Up to previous, I to next, Enter to Select, and Backspace/Esc to Back. Focus wraps. Enter opens Appearance, Phone or Clock & Date. Navigation remains pending. Phone status follows normalized phone service observations.

Prototype Settings access is unrestricted by speed, data quality, UART status or warnings at the user's request (2026-10-29). Tapping the gear opens it after boot even without a speed observation. Warnings remain live in the persistent tell-tale row; opening Settings does not fabricate stationary speed, neutral or lamps and does not change the UART/demo source. Lifecycle shutdown/off still owns screen visibility and cancels unconfirmed previews. Physical Trip/Info inputs already route over UART; radio commands remain a future integration. This temporary prototype access policy is not a reviewed riding-use policy; restrictions can be added later.

### Touch navigation

A 44 × 52 px gear target at the top-right beside the clock/date opens Settings through the same access reducer as physical/keyboard controls. It returns Back when Settings is already open. Touch release selects category tiles, Appearance choices/Apply/Cancel, Phone action rows and Back; content taps are ignored during the transition. Speed, stale/unknown data and warnings do not block entry in this prototype; boot/shutdown still controls visibility. Non-touch navigation remains supported. Native display serial controls are `s` open, `q` Back, `[`/`]` previous/next and Enter select. Display-owned Appearance persistence uses NVS; real radios remain unavailable. Apply closes only after confirmed save.

### Instrument transition

The same persistent speed label travels between the dashboard and top strip over 280 ms, easing position and scaling the Street numerals to approximately the existing 46 px secondary size. Track speed retains its secondary size and moves to the same strip. The unit follows the visible glyph edge. Reversing the request preserves the current transition position. Normal dashboard geometry is restored exactly. Gear and tell-tales remain live; current/best lap return with Track instrumentation.

A monotonic portable transition runs under the existing LVGL owner. No widget timer, task or per-update application allocation is added. Hidden dashboard instruments defer updates until shown, then consume the latest observation. The instrument group is hidden without a full-screen alpha layer; category tiles appear at the settled endpoint to avoid a full-menu opacity redraw during every movement frame. Transformed speed/leading-zero labels use tight text bounds to reduce temporary layer work. LVGL may allocate bounded intermediate layers for text transforms. Actual target heap, render/scanout timing and DMA/PSRAM behavior remain hardware gates.

### Appearance

Sequential focus order: Midnight, Daylight, Instrument design, Cancel, Apply. Enter on a theme previews it immediately across the interface. Only Instrument is offered as the current design; there are no fake alternate designs. The three-column composition stacks Midnight/Daylight choices in the first column and spans the remaining two columns with one live preview. The preview composes the actual Header, TellTaleRow, Tachometer, PrimaryReadout, RideInfoPanel and FuelGauge with fixed illustrative samples, following the selected theme, Street/Track layout and units. It has no service, timer or nested menus. The only design remains Instrument.

Cancel or Back restores the theme from entry and returns to Settings home. Apply emits a typed SavePreferences effect to the host adapter. Success closes Appearance and retains the choice; failure keeps the preview open with a visible retry message. Retrying Apply, selecting a different theme or cancelling remains possible. Back/Cancel or a lifecycle power transition cancels an unconfirmed preview; motion/data-quality/warnings do not close the prototype menu.

### Confirmed preferences

Preferences.h contains confirmed theme/design plus clock format, date format and timezone preferences. Menu focus, current screen, preview state, telemetry, trips and vehicle counters are not persisted. Actual time is not persisted as if it could keep ticking while unpowered.

The host/native adapters load before the first dashboard update and save through Apply, skipping unchanged choices. Version 2 uses 16 explicit bytes: R15P magic, version, theme, design, twelve-hour flag, date-format enum, automatic-timezone flag, signed little-endian UTC offset minutes, three zero reserved bytes and an XOR integrity byte. Loading accepts validated v2 records or the legacy eight-byte v1 theme/design record; legacy records retain their theme and receive default clock preferences. Missing/malformed data falls back without automatic overwrite or erase. No native struct memory is stored.

Mac path: `~/Library/Application Support/R15 Smart Cluster/preferences.bin`. `R15_SETTINGS_PATH` overrides it for an isolated preview/test. On other hosts, use XDG_CONFIG_HOME or `~/.config/r15-smart-cluster/`. Deterministic render previews do not load or modify a user's settings.

PreferenceFile performs host filesystem operations outside LVGL rendering, using a flushed/closed temporary sibling followed by atomic replacement. Failed writes report failure and preserve the confirmed file. This does not claim hardware power-loss durability. The native display loads the same validated versioned record from NVS before its first frame. Apply queues a single request to a dedicated bounded storage worker; only confirmed commit updates saved preferences and closes the requesting Appearance/Clock page. Repeated Apply is suppressed while Saving is visible; unchanged confirmed choices skip writes. Failed initialization/open/write/commit stays a visible retryable failure and never erases NVS automatically. Cancelling while a write is pending does not cancel a commit already underway or close a subsequently reopened screen. The UI never calls NVS. Actual panel restart, power-cut durability and flash/render coexistence remain hardware tests.

### Navigation and future services

A host-only 800 × 480 vector-map rendering prototype is now available through `bash scripts/preview.sh --map-preview`. It preserves the compact instrument strip, tell-tales and lifecycle overlay, and adds surrounding synthetic roads, a highlighted sample route, heading marker, maneuver card and sample remaining-distance/time footer. Midnight/Daylight and north-up follow/overview are provided. This is a rendering exploration, not an approved final navigation layout, live GPS/routing, provider projection or firmware feature. A separate `--kolkata-map` mode now loads a real offline central-Kolkata OSM road snapshot with attribution, pan/zoom and area overview; it shows no route, guidance/ETA or position marker. Synthetic movement remains exclusive to the sample mode. Stale position hides the marker and guidance; unavailable map hides the scene; warnings remain visible. See [controls and exact evidence](development.md#lvgl-map-render-preview).

Navigation remains pending. The selected direction is a custom moving map with an Android-first Flutter companion app and future iOS support; Android Auto/CarPlay are not required. The unified Phone category replaces rider-facing Bluetooth/Wi-Fi setup. See the [phone journey](../mobile/docs/architecture.md#rider-journey-and-truthful-state). GPS position alone does not provide map rendering. USB clock synchronization and clock-format/timezone editing are implemented; authenticated phone time is also implemented; retained RTC and vehicle network time remain pending. Submenus share Next/Previous/Select/Back; touch can be another input adapter, while physical-switch navigation remains a requirement. Text entry needs a separate provisioning decision.

### Verification

Portable checks cover unrestricted access across speed/quality/warnings, lifecycle closure, four-category focus wrapping, Apply/Cancel, save effects/failure, clock/date/timezone boundaries and transition reversal. Host storage checks cover restart round trips, malformed sizes, invalid record fields/checksum, atomic replacement, failed replacement preserving the confirmed file and storage failure. Tests use a separate temporary directory under build output.

Render scenarios cover Settings/Appearance, theme previews, focused design, save failure, entry/exit, urgent warnings and stale speed for both layouts/themes, exact dashboard restoration, and zero idle redraws on Appearance, Settings and dashboard. Host rendering and persistence are not board or bike verification.

### Phone connection

Settings → Phone retains gear, compact speed, clock and the full tell-tale row in both layouts/themes. The page shows unavailable, ready to pair, first-pair request/code, awaiting phone completion, saved/disconnected and connected states. Prototype UI pairing actions no longer require stationary speed; service availability, authentication and explicit pairing approval still apply. A pending request defaults to Reject. Forget confirmation defaults to Cancel. Unknown service state exposes Back only. Lifecycle transitions restore instruments and reject pending pairing; warnings/stale speed leave the prototype menu open; a request already accepted cannot be undone by a later Back.

The app owns discovery and chooses the transport. One cluster Phone page replaces both radio menus; there is no rider-facing SSID scan or Bluetooth device list. PhonePair/Approve/Reject/Disconnect/Forget are semantic effects; they never mutate confirmed observations optimistically. Display widgets do not parse packets, manage bonds, store secrets or call radios.

The [nearby connection guide](../mobile/docs/development.md#mac-preview-adapter) documents actual Android BLE discovery, Mac BLE advertising, first-time approval and saved automatic reconnect. The opt-in Mac preview uses BLE for public discovery and Wi-Fi TLS for the session, with protected Keychain/Android Keystore persistence. The physical ESP32 BLE pairing/metadata adapter is implemented independently; Wi-Fi/access-point provisioning and broader application services remain pending. The plain host preview has an unavailable Phone service; no fake phone is discovered. Legacy portable radio fixtures are development-only, not Settings categories.

Automated Phone suites were removed by user instruction. Manually preserve state validation, focus, unrestricted prototype pairing access, confirmed connection truth, confirmation default and interruption. Deterministic 800 x 480 render scenarios exercise unpaired, saved, connected, forget confirmation, unavailable/stale and pending/approved pairing across both layouts/themes, plus warnings/stale-speed visibility and lifecycle interruption and idle redraw checks.

In native firmware, tapping the gear opens Settings without replacing live observations. Explicit serial `p` remains an optional stationary demo fixture, with only speed synthesized; it is never required for menu access. `v` restores live UART.

### Clock, date and connection indicator

The clock/date area belongs to real wall time. The gear icon communicates the vehicle UART state: theme foreground for fresh accepted packets, amber before a packet, red after the existing one-second expiry. This indicates link freshness, not validity of every vehicle source. UART counters remain in the console. An independent BENCH DEMO header label identifies explicit synthetic mode without occupying the date.

Clock & Date offers 12/24-hour display (separate AM/PM in 12-hour mode), DD/MM/YYYY, MM/DD/YYYY or YYYY-MM-DD, and either the Mac-supplied UTC offset or a manual offset from UTC-12:00 to UTC+14:00 in 15-minute steps. Changes preview immediately; Apply saves through the bounded NVS worker, Cancel/Back restores entry preferences. Manual offsets do not implement a geographic timezone/DST rule database. Automatic mode follows the Mac's current offset, including its current daylight-saving setting, on each sync.

For this bench phase, run `bash scripts/sync-clock.sh /dev/cu.usbserial-10` after flashing the display (replace the port as needed). Keep it running. It sends the current UTC epoch and local offset over USB UART0 every 30 seconds and in response to `@CLOCK REQUEST`, and prints confirmation only after `@CLOCK OK <epoch>`. It requests exclusive serial access where supported; stop it before flashing or opening another console. Opening a USB serial port may reset some boards; the helper responds to the new boot request. No Android build or network connection is needed.

The bounded console frame is `@TIME <UTC epoch seconds> <signed offset minutes>\n`; accepted epochs are 2020–2099 UTC, offsets -720 through +840 minutes. Invalid/oversized frames are discarded without turning their contents into UI keys. Clock durations use monotonic time; wall-time correction never changes boot, shutdown or button timing. Display formatting is minute-driven and separate from signal freshness.

The display keeps ticking in RAM while powered after sync. Cold power loss invalidates time; it requests sync and shows CLOCK NOT SYNCED until a real source arrives. Saving the last date to flash cannot account for time spent unpowered. Automatic cold-start time without the Mac helper requires future phone/network time or a validated battery-backed RTC. No retained-clock claim is made.

Native BLE Forget confirmation remains in a pending state until vehicle status confirms removal, rather than immediately reopening connected-device actions. The vehicle disconnects before deleting its bond and stops advertising for NimBLE unpair. See [bench behavior and deadlines](development.md#forget-while-connected).

Pairing requests display a large code modal with bold speed-style digits, a single-line identity heading, solid Allow pair and outlined transparent Reject pair. The current code remains visible while waiting for phone confirmation. Header/warnings remain visible. Touch uses the modal buttons and consumes backdrop taps; non-touch defaults to Reject. Real phone labels are optional, untrusted and supplied through [the firmware name contract](../mobile/docs/protocol.md#enrollment-name-c011), with an explicit unavailable fallback.

Connected BLE phones now supply optional live name/battery and clock synchronization through the [authenticated metadata contract](../mobile/docs/protocol.md#connected-metadata-c012). Missing metadata stays unavailable; the header phone indicator is hidden unless connected. Both MCU updates and companion integration require physical acceptance.

## Power lifecycle

Updated: 2026-10-29. User-selected journey; hardware implementation unverified.

### Intended journey

```text
Key ON -> boot animation (2.00 s) -> live instrumentation
Key OFF -> shutdown animation (2.00 s) -> controlled power-off
```

Boot lasts exactly 2,000 ms and shutdown lasts exactly 2,000 ms under the monotonic lifecycle controller. The user selected [R15 Light Signature, concept 02](reference/r15-light-signature-concept.png). The phase allocation below implements that direction. The running dashboard remains the existing Street/Track UI. Actual panel startup and retained shutdown power still require hardware measurements.

### Implemented host sequence

| Boot interval | Behavior |
| --- | --- |
| 0–350 ms | Two slender blue light slashes ignite from black |
| 350–850 ms | Blades brighten and broaden; the white R15 wordmark appears |
| 850–1,200 ms | Full signature, with a short controlled highlight beneath the wordmark |
| 1,200–1,700 ms | Blades move outward into slim accents; wordmark fades |
| 1,700–2,000 ms | Current dashboard reveals beneath the fading accents |

| Shutdown interval | Behavior |
| --- | --- |
| 0–200 ms | Current dashboard fades to black |
| 200–400 ms | Light Signature appears |
| 400–1,250 ms | Slashes extinguish from outside inward; wordmark shrinks slightly |
| 1,250–1,900 ms | Lights finish extinguishing; small R15 wordmark fades away |
| 1,900–2,000 ms | Completely black |

AnimationFrame is a portable function of monotonic elapsed time. The renderer draws a fixed number of mirrored line/triangle primitives and one small A8 wordmark. There are no flashes, particle systems, queued callbacks, full-screen textures or separate animation tasks. The logo never grows beyond native size. Both themes use the same black/blue/white signature palette.

During dashboard reveal and initial shutdown fade, current instrumentation continues updating beneath the cover. Valid urgent warnings remove the overlay immediately in every animation stage. Key-on cancels shutdown. Off/ReadyForPowerOff remain black; keyboard help stays in Terminal.

### State model and ownership

The portable controller implements Off, Boot, Running, Shutdown and ReadyForPowerOff. Starting below describes future board initialization, not an implemented controller phase. BootAnimation/ShutdownAnimation are product names for Boot/Shutdown. No persistence/readiness handshake is implemented by the host timer.

| State | Behavior | Exit |
| --- | --- | --- |
| Off | No display execution assumed | Stable power and key-on |
| Starting | Minimal board initialization; start acquisition/UART concurrently; unknown values remain unknown | Display can render boot animation |
| BootAnimation | Play 2.00 s using monotonic timing while state acquisition continues | Animation completes -> Running; valid urgent warnings can interrupt/overlay immediately |
| Running | Actual speed/ODO/gear and other supported observations; dashes/status for missing data | Confirmed key-off -> ShutdownAnimation; independent faults remain explicit |
| ShutdownAnimation | Play 2.00 s; stop optional work, request bounded persistence and retain essential indication | Animation complete and required bounded shutdown work finishes -> ReadyForPowerOff |
| ReadyForPowerOff | Report readiness to the power owner if supported; no unverified power-control GPIO | Hardware safely removes retained supply |

The vehicle MCU owns validated ignition observation and supplies a defined lifecycle event/status. The display lifecycle controller owns navigation/animation transitions; widgets do not monitor power pins. The electrical hold-up/latch controller is a separate hardware responsibility. Display communications failure alone never means key-off. If ignition events are not yet available, bench simulation must be visibly identified.

### Timing and critical truth

Animation duration is measured from its first visible frame to its completion. It is distinct from total key-on latency: reset, bootloader and panel initialization add time that must be measured. No total boot-time guarantee is claimed. Boot animation must not wait for CAN, phone, Wi-Fi or BLE readiness. Data acquisition starts as early as practical; latest valid state is ready when instrumentation appears.

Implemented host behavior: a valid urgent warning interrupts the boot/shutdown artwork immediately. Never hide critical truth to finish decoration. This exception and which indications qualify must be reviewed before bike use. If animation resources fail, proceed to instrumentation or an explicit fault indication rather than holding a blank screen.

Running means actual received observations where supported. Do not show demo zero speed, neutral, fuel or odometer while waiting. Vehicle odometer is not derived from a demo baseline; unavailable ODO stays unavailable. Date/time needs an explicit source/quality after cold power loss.

### Agreed power topology

Each MCU receives a separate regulated supply branch; the display is not powered through the vehicle DevKit. Both branches may share a reviewed upstream protected supply. Dedicated power circuitry carries/switches load current; the vehicle MCU coordinates ignition and bounded shutdown, with a hardware timeout/fallback. Startup must not wait for phone connectivity or vehicle firmware readiness. Prevent signal-line back-powering when one board is off. Exact regulators, switching circuit and current ratings require measurements and electrical review. Bench development powers each board separately from Mac USB; the UART cable power conductor stays disconnected.

### Shutdown power contract

A switched supply that disappears at key-off cannot run a shutdown animation. Hardware must provide ignition sensing separately from a retained regulated supply for both MCUs as needed. Required retention covers detection, the 2.00 s animation, bounded pending writes, coordination and measured margin. Component values, capacitor sizing, latch circuit, thresholds and readiness wiring remain undecided. Firmware cannot promise animation under hard power removal or brownout.

Persist preferences on change through bounded/coalesced writes; do not depend exclusively on a final shutdown write. Vehicle counters require their own wear, retention and power-loss policy before being treated as reliable persistence. The display may report ready, but it must not cut shared power while vehicle-side required work is pending. Define an overall bounded shutdown deadline and hardware fallback before implementation; no indefinite wait for the other MCU or phone.

Key-on during shutdown cancels power-off and returns to Running if state is valid; discard any stale readiness request. The hardware handshake must prevent a previous readiness message from cutting power after key-on. Rapid key changes, reset during shutdown and loss of UART require explicit session/transition identifiers and idempotent handling. Debounce and ignition polarity come from measured electrical behavior, not guessed constants.

### Verification

Host tests use injected monotonic timing for lifecycle boundaries, repeated key events, key-on during shutdown and urgent-warning presentation; deterministic captures include missing observations and black endpoints. Firmware fault tests must additionally verify animation/driver failure fallback and that UART loss never becomes key-off. Bench tests must measure first visible frame, total startup, retained shutdown supply, persistence completion, brownout/hard cut, independent MCU resets and rapid off/on cycles. Final animation readiness requires actual-panel inspection and power-path verification.

### USB-powered display bench preview

The native V3.0 desk-test target exposes shutdown through serial `o` or a lower-right fuel-area tap. It freezes the synthetic snapshot and clears only test urgent-warning overrides before requesting the shared lifecycle transition. The 2,000 ms controller sequence finishes black without cutting USB power. Once black, any tap (or serial `b`) starts the 2,000 ms bench boot and clears synthetic observations. Auto-demo input is paused during shutdown; real-warning handling in the shared dashboard is unchanged. This control exercises artwork, not an ignition adapter or retained-power circuit. Removing USB still removes the supply immediately.
