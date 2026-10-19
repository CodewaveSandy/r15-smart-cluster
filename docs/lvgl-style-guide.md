# LVGL visual and implementation style guide

Date: 2026-10-18. Status: UI specification; dashboard foundation implemented, remaining screens/artwork pending.

## Visual authority

Implement the geometry and behavior specified here. Capture deterministic 800 x 480 states in the LVGL host build for visual review. Match geometry and hierarchy; allow documented RGB565 quantization/font rasterizer differences after inspection. Never substitute different typefaces silently.

## Layout and typography

| Element | Specified baseline |
| --- | --- |
| Canvas and content width | 800 x 480; left/right margin 28; usable width 744 |
| Header | Gear number/N only (no GEAR caption); mode, QS/VVA, phone/battery, clock/date; divider y=85 |
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
| Foreground | #edf4fb | #122333 |
| Muted | #91a4b7 | #4d6577 |
| Panel | #14202c | #e0e8ec |
| Divider | #253544 | #c7d3db |
| Accent | #78e4c1 | #007757 |
| Amber | #f5d45b | #946000 |
| Red | #ff7980 | #bf263a |
| High beam | #77baff | #246bb0 |

Centralize palette, spacing, geometry, opacity, fonts and timing in typed tokens. Portable display thresholds are in embedded/presentation/PresentationPolicy.h; rendering Metrics reference them. Shared widget constructors use tokens; screens compose widgets rather than copying style logic. Theme changes never change telemetry or riding behavior. Preserve the specified red shift background gradient (#492731 -> #b34c48 -> #64303b) and readable warning palette; profile its redraw cost.

## Instrument behavior

- Scale labels 11, 12 and 13 use the theme red as a user-selected redline cue. This does not change fill colors, shift thresholds or establish a verified vehicle limit.
- Continuous RPM gradient anchored to the entire 0–13,000 scale; clip fill, do not stretch colors with the current RPM. Green #4cdb88 through 3,500; blend to #f5d45b at 4,500; hold through 7,500; blend to #f06464 at 8,500, then hold.
- Shift background steady from 10,500 RPM; alternate red/normal every 300 ms from 11,400. Stop immediately below thresholds; unavailable RPM must not trigger it.
- Eight forward-slanted fuel pieces with a 4 px top/bottom offset. Each piece supports partial coverage: clamp(percent × 8 − pieceIndex × 100, 0, 100) coverage units, rather than rounding to a whole piece. Valid integer-percent observations move the fill in 1% steps; no invented intermediate measurements. Yellow at 10–20% inclusive, red below 10%; blink pump and filled segments every 500 ms below 20%. Exactly 20% is steady yellow; percentage stays readable; zero still blinks the pump.
- Tell-tales and phone pictogram: 12% inactive, 100% active, no fades. Specified simulated turn blink is 450 ms. A future already-flashing vehicle signal must not be blinked a second time: distinguish source phase from steady intent in the contract.
- Preserve QS/VVA lettering and recognizable automotive symbol meanings. Simulator VVA is strictly above 7,500; real VVA support requires a verified observation/rule. No prototype RPM/fuel threshold is a verified Yamaha specification.
- User-supplied nominal tank capacity is 11 L and approximate reserve is 1.76 L, giving a 16% reserve cue. Show a fixed amber marker above the gauge at 16%; replace `FUEL` with steady `RESERVE` for valid readings at or below 16%. Unknown/stale data must not assert reserve. This percentage-based cue assumes normalized fuel percent represents remaining usable capacity; actual sensor/volume calibration remains vehicle-owned and unverified. Existing warning/blink thresholds remain separate from the reserve cue.

## Components and data flow

Create focused Header, Tachometer, SpeedReadout, GearReadout, TellTaleRow, FuelGauge and InfoPanel as their responsibilities are implemented. Each exposes a small typed update API, owns its LVGL subtree and caches displayed values. Screens compose them; a presenter maps normalized state and quality to display values. Keep formatting testable without a display.

Screens: Dashboard (Street/Track layouts), Settings, Clock, VehicleInfo, developer Help. Hardware labels must describe actual available controls and time sources; host developer hints remain host-only.  Touch is optional and cannot become required navigation.

Navigation actions: up/down/left/right/select/back/next/previous. Host keyboard and vehicle input adapters produce the same actions; no keycodes, GPIOs or CAN identifiers in widgets. Display settings/focus remain local. Remote trip/lap actions require the defined command contract and visible failures.

## Unknown and warning presentation

Unavailable measurements use dashes and clear data status, not zero/empty/neutral. Unsupported fields stay unavailable rather than plausible samples. A stale warning is not automatically off; define unknown indication separately from known inactive. Use this quality treatment consistently on real hardware. Warnings remain visible on relevant menu screens as well as the dashboard. Simultaneous warnings must remain distinguishable.

Valid warnings update on the next UI service cycle with no animation delay. RPM smoothing, if added later, is bounded, cancellable and never applied to warning truth. Use a small shared monotonic animation scheduler; no per-widget FreeRTOS tasks.

## Rendering discipline and acceptance

Use persistent LVGL objects and changed-field updates. Prefer built-in primitives and small static icon paths/masks; profile custom drawing before adopting caches. Cache the invariant RPM ramp if useful. Avoid full-screen image assets, repeated object rebuilding, expensive shadows and large translucent overlays. Keep the exact visual baseline before optimizing; inspect every optimization for regressions.

Required comparisons: both layouts x both themes; neutral and gears; 0/max/unknown speed/RPM; 10,499/10,500/11,399/11,400 shift boundaries; fuel 0/9/10/19/20/21/100 plus both blink phases; simultaneous warnings; phone disconnected and battery unknown; each data quality; settings focus and full date save/cancel; link loss/recovery/reboot. Capture fixed clock and animation phase on host. Inspect native panel for tearing, color ordering, viewing angle and blink/readability at intended installation position.

## Boot and shutdown artwork

Boot lasts exactly 3.00 s and shutdown lasts exactly 2.00 s using the selected R15 Light Signature. Draw mirrored tapered blue slashes with crisp white cores around a small solid-white R15 wordmark; use a black backdrop, restrained glow and controlled opacity/retraction. Avoid flashes and oversized zooms. Preserve the dashboard at the reveal/fade endpoints and immediate urgent-warning interruption. See [power lifecycle](power-lifecycle.md) for state, power and test requirements.

## Typography roles and preview quality

Inter Medium is the primary label font: 14 px general text, 16 px RPM label, 18 px speed units, and 12 px for compact date/ticks. Inter SemiBold at 23 px supplies mode/supporting values. The compact RPM readout uses 16 px Inter SemiBold digits with comma grouping (`8,500`) and a separate 16 px Inter Medium `RPM` suffix matching the `RPM × 1,000` caption. Both share a baseline at the original small-readout position; numerals use theme foreground (white in Midnight, dark in Daylight). Chakra Petch Bold Italic supplies Street speed (128 px), gear (72 px), Track speed (46 px) and current lap (74 px); Barlow Semi Condensed SemiBold retains clock (30 px). Chakra bitmaps receive a subtle half-pixel coverage expansion during generation, preserving advances/kerning; the 74 px timer fits the maximum value with separation from speed. The angular numeral treatment is R1M-inspired, not a factory-font claim. These choices favor restrained interface labels and distinctive instrument numerals.

Use embedded/ui/Typography.h roles; generate bitmap assets at their actual logical size using scripts/generate-fonts.sh. Maintain 4-bit antialiased coverage and full required glyph subsets. Bitmap generation does not enable OpenType features automatically; tabular-number behavior must be explicitly implemented/verified if needed. Keep changing values in stable bounds.

The SDL host preview uses linear texture filtering when enlarging the 800 x 480 framebuffer for Retina. This softens magnified pixel edges; it does not imply higher panel resolution or hardware text quality. Inspect native 800 x 480 captures and the actual panel separately. The icon artwork is a separate presentation responsibility.

## Independent lower lists

Street trip list: ODO -> Trip 1 -> Trip 2 -> ODO. Track replaces this entire panel with fixed BEST LAP, hides distance units, and does not cycle trip pages. Returning to Street restores its selected trip page. Info list: coolant -> Track mode -> real-time fuel consumption (L/100km) -> average fuel consumption (L/100km) -> average speed (km/h, or mph with imperial selected) -> TCS -> unidentified placeholder -> coolant. Each list keeps its own selection. Distance uses normalized km and display-unit conversion; unknown values remain dashes. TCS ON/OFF is an observation; the host preview may toggle a sample but real control requires a verified command contract. Track ON/OFF changes the UI layout only. All demo distances, consumption and average-speed values are fixed host samples, not functioning calculations or persistent vehicle data. Consumption and average speed are separate quality-bearing vehicle-owned observations; averaging windows and reset semantics remain unresolved. The unidentified page shows a dash and has no action until identified. Battery voltage is no longer an info-list page.

Street speed remains visually centered on x=400 independently of digit count, including unknown data. Position its `km/h` or `mph` unit 10 px beside the visible right glyph edge, near the baseline; compensate italic side bearings when centering. Track puts current lap at x=400 in the 74 px Bold Italic font and moves speed to the right-hand secondary position. The lap caption is above its large readout; the speed unit sits beside the side speed. Fuel retains eight pieces over the existing width; a four-pixel skew creates the slight italic silhouette. Empty backgrounds stay visible behind the partially filled piece, and a fixed amber notch marks the approximate reserve threshold.

Current lap is an observation in milliseconds. Presentation formats mm:ss.hh with truncated hundredths; unknown/stale or durations beyond 99:59.99 show dashes rather than wrapping. The host provides a static 01:24.38 sample for layout review. This is not a functioning stopwatch.

Both modes share the same 44 px RPM height, y=376 lower divider and bottom-panel baselines. Track current-lap title/value are positioned at y=235/y=259 to balance the taller main section. Best lap is a separate normalized millisecond observation; the UI never calculates a record from displayed current-lap samples.

## Icon system

Use the selected Pictogrammers MDI automotive/status glyphs through typed Icon roles in embedded/ui/Icons.h. The symbol set includes proper high-beam rays, coolant thermometer/waves, engine, ABS ring, traction-control, message bubble, call handset, hazard triangle, solid turn arrows, phone and fuel pump. Neutral remains N. Use the 32 px native bitmap subset, with ABS at 30 px to avoid clipping its ring. Keep baseline placement and 12% inactive / 100% active presentation consistent. No fades delay warning activation.

Bundle source version, hashes, Apache 2.0 license and notices. Only selected glyphs are linked; do not add a runtime SVG renderer or full icon font. The artwork is generic and requires separate OEM/vehicle validation before claiming exact conformity. F2 inspection covers both layouts/themes.

## Settings home and instrument transition

The implemented first Settings slice follows [settings.md](settings.md). Retain the header gear and full tell-tale row while moving speed to center x=200/y=22. Four category tiles use shared geometry and MDI glyphs; focus uses a 2 px accent border. A reversible 280 ms transition moves/scales the same speed label and follows with its unit. Normal layouts restore exactly. Settings requires a valid speed below 20 km/h; urgent warnings, stale speed and lifecycle changes restore instruments immediately. Appearance now offers Midnight/Daylight previews, one Instrument design, Apply/Cancel and host saved preferences. Radio submenus and firmware NVS remain pending.
