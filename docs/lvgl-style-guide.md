# LVGL visual and implementation style guide

Date: 2026-10-18. Status: UI specification; dashboard foundation implemented, remaining screens/artwork pending.

## Visual authority

Implement the geometry and behavior specified here. Capture deterministic 800 x 480 states in the LVGL host build for visual review. Match geometry and hierarchy; allow documented RGB565 quantization/font rasterizer differences after inspection. Never substitute different typefaces silently.

## Layout and typography

| Element | Specified baseline |
| --- | --- |
| Canvas and content width | 800 x 480; left/right margin 28; usable width 744 |
| Header | Gear first; mode, QS/VVA, phone/battery, clock/date; divider y=85 |
| Gear / mode / clock | 46 px bold / 22 px bold / 30 px semibold |
| Tell-tales | Row y=98; arrows at opposite edges |
| Tachometer | x=28, y=144, width=744; fill begins 20 px below component top |
| RPM fill height | Street 44 px, Track 52 px |
| Speed | Street 128 px bold at x=210,y=220; Track 79 px bold at x=28,y=231 |
| Lower divider | Street y=376; Track y=336 |
| Lower information | Primary page left, supporting bike/lap information middle, fuel right |
| Footer divider | y=439; status/hints below |

Use bundled Bricolage Grotesque Regular/SemiBold/Bold under the specified OFL license. Convert font sizes from observed rendered metrics; Validate nominal font sizes against actual rendered baselines. Centralize text styles, tracking, line heights and alignment. Reserve fixed bounds for changing digits and use bounded formatting. Small-font subsets need all actual menu/date/unit/error glyphs, degree sign and unavailable dash; large-font subsets need their actual digits/N/dash/punctuation. Keep font sources, converter version, command and output size reproducible.

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

Centralize palette, spacing, geometry, opacity, fonts and timing in typed tokens. Shared widget constructors use tokens; screens compose widgets rather than copying style logic. Theme changes never change telemetry or riding behavior. Preserve the specified red shift background gradient (#492731 -> #b34c48 -> #64303b) and readable warning palette; profile its redraw cost.

## Instrument behavior

- Continuous RPM gradient anchored to the entire 0–13,000 scale; clip fill, do not stretch colors with the current RPM. Green #4cdb88 through 3,500; blend to #f5d45b at 4,500; hold through 7,500; blend to #f06464 at 8,500, then hold.
- Shift background steady from 10,500 RPM; alternate red/normal every 300 ms from 11,400. Stop immediately below thresholds; unavailable RPM must not trigger it.
- Eight slanted fuel segments; ceil(percent/12.5) for valid fuel. Yellow at 10–20% inclusive, red below 10%; blink pump and filled segments every 500 ms below 20%. Exactly 20% is steady yellow; percentage stays readable; zero still blinks the pump.
- Tell-tales and phone pictogram: 12% inactive, 100% active, no fades. Specified simulated turn blink is 450 ms. A future already-flashing vehicle signal must not be blinked a second time: distinguish source phase from steady intent in the contract.
- Preserve QS/VVA lettering and icon shapes. Simulator VVA is strictly above 7,500; real VVA support requires a verified observation/rule. No prototype RPM/fuel threshold is a verified Yamaha specification.
- Fuel warning <20% and current reserve-trip calculation <=15% are distinct specified behaviors; do not accidentally merge them during implementation.

## Components and data flow

Create focused Header, Tachometer, SpeedReadout, GearReadout, TellTaleRow, FuelGauge and InfoPanel only as they are ported. Each exposes a small typed update API, owns its LVGL subtree and caches displayed values. Screens compose them; a presenter maps normalized state and quality to display values. Keep formatting testable without a display.

Screens: Dashboard (Street/Track layouts), Settings, Clock, VehicleInfo, developer Help. Hardware labels must describe actual available controls and time sources; host developer hints remain host-only.  Touch is optional and cannot become required navigation.

Navigation actions: up/down/left/right/select/back/next/previous. Host keyboard and vehicle input adapters produce the same actions; no keycodes, GPIOs or CAN identifiers in widgets. Display settings/focus remain local. Remote trip/lap actions require the defined command contract and visible failures.

## Unknown and warning presentation

Unavailable measurements use dashes and clear data status, not zero/empty/neutral. Unsupported fields stay unavailable rather than plausible samples. A stale warning is not automatically off; define unknown indication separately from known inactive. Use this quality treatment consistently on real hardware. Warnings remain visible on relevant menu screens as well as the dashboard. Simultaneous warnings must remain distinguishable.

Valid warnings update on the next UI service cycle with no animation delay. RPM smoothing, if added later, is bounded, cancellable and never applied to warning truth. Use a small shared monotonic animation scheduler; no per-widget FreeRTOS tasks.

## Rendering discipline and acceptance

Use persistent LVGL objects and changed-field updates. Prefer built-in primitives and small static icon paths/masks; profile custom drawing before adopting caches. Cache the invariant RPM ramp if useful. Avoid full-screen image assets, repeated object rebuilding, expensive shadows and large translucent overlays. Keep the exact visual baseline before optimizing; inspect every optimization for regressions.

Required comparisons: both layouts x both themes; neutral and gears; 0/max/unknown speed/RPM; 10,499/10,500/11,399/11,400 shift boundaries; fuel 0/9/10/19/20/21/100 plus both blink phases; simultaneous warnings; phone disconnected and battery unknown; each data quality; settings focus and full date save/cancel; link loss/recovery/reboot. Capture fixed clock and animation phase on host. Inspect native panel for tearing, color ordering, viewing angle and blink/readability at intended installation position.

## Boot and shutdown artwork

Each animation lasts 1–1.5 s; artwork is undecided. Reuse the theme/type system, use lightweight assets/primitives and support immediate urgent-warning interruption. See [power lifecycle](power-lifecycle.md) for state, power and test requirements.
