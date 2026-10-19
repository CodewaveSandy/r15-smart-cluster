# Design decisions

Baseline: 2026-10-18. Accepted direction is distinguished from implementation proposals.

| ID | Status | Decision and reason |
| --- | --- | --- |
| D01 | Accepted | Target the user's Yamaha R15 V4; personal prototype before commercialization |
| D02 | Accepted | Two ESP32 MCUs: vehicle acquisition/calculations/Wi-Fi/BLE and separate display rendering |
| D03 | Accepted | UART exchanges normalized observations; presentation remains independent of vehicle protocols |
| D04 | Accepted | LVGL on the selected Elecrow 7-inch 800 x 480 display family; exact PCB revision pending |
| D05 | Accepted | Preserve the specified Street/Track UI, Inter/Barlow typography, themes, RPM gradient, icons and fuel behavior |
| D06 | Accepted | Non-touch navigation through semantic physical actions |
| D07 | Accepted | Full Maps/Android Auto/CarPlay remains an unresolved requirement to investigate |
| D08 | Superseded 2026-10-19 by D14 | Key-on boot animation and key-off shutdown animation each last 1–1.5 s; shutdown depends on retained hardware power |
| D09 | Proposed | ESP-IDF, FreeRTOS, C++17, LVGL 9 candidate and SDL host preview; pin compatible versions after board validation |
| D10 | Proposed | Single LVGL owner, bounded state mailbox/event queues, RGB565; buffering selected by measured driver behavior |
| D11 | Proposed | Framed binary UART with integrity checks; finalize wire specification and golden vectors before coding |
| D12 | Accepted | No unverified Yamaha facts; initial real-bike work passive/read-only; critical failure behavior needs separate review |
| D13 | Accepted | Git operations stay under user control; generated builds/captures are not source |
| D14 | Timing retained; artwork superseded by D15 | Boot/shutdown each last exactly 2.00 s; preserve the existing dashboard and immediate urgent warnings |
| D15 | Accepted | R15 Light Signature, concept 02: mirrored blue slashes, small white R15, controlled reveal/retract, black shutdown endpoint |

Changes to consequential decisions must record date, rationale, implications and unresolved questions. Supersede decisions explicitly when direction changes.

## Implementation selections — 2026-10-18

The host preview uses LVGL 9.2.2, SDL2, C++17 and CMake. LVGL source is hash-pinned and Inter/Barlow conversion uses locked lv_font_conv 1.5.3. Firmware framework compatibility remains pending. At this initial selection, lifecycle duration was 1250 ms with progress-line artwork. This artwork/timing is superseded by D14; urgent-warning exposure remains.

## Typography selection — 2026-10-18

Accepted by the user: Inter for labels/supporting information and Barlow Semi Condensed for instrument numerals. Use Medium/SemiBold weights, larger 16 px RPM labels, reusable typography roles and locally bundled OFL assets. Pin Inter release 4.1 and Barlow's google/fonts source revision as recorded with the assets. Keep only the required source weights/subsets. Preserve layout and instrument behavior; font/icon redesign are independent work. The host uses linear Retina interpolation while LVGL's logical framebuffer remains 800 x 480.

## Dashboard layout and information lists — 2026-10-18

User-selected: remove the footer/help bar and GEAR caption; center speed horizontally; keep independent ODO/Trip A/Trip B/Trip F and coolant/battery/TCS/Track lists; minimize fuel-segment gaps. Host controls: Enter cycles trip, I cycles info, T toggles selected simulated TCS or display Track. No vehicle control is implemented. Simulation provenance remains in the window title/terminal; unavailable status moves into the header subtitle.

## Track lap priority — 2026-10-18

User-selected: lap time is the large central Track readout; move speed to the previous right-hand lap location. Use the existing 79 px instrument font for lap and 46 px for secondary speed. Street keeps its existing speed presentation. Lap time comes through normalized millisecond observations; the host currently supplies a fixed sample, not live timing.

## Track best lap and common height — 2026-10-18

User-selected: Track replaces ODO/trip pages with BEST LAP and uses Street’s vertical section geometry. This supersedes the mode-dependent RPM/lower-panel heights and Track trip cycling. Street retains its selection across mode changes; best lap comes from normalized state rather than UI calculation.

## Automotive icon system — 2026-10-18

Use selected Pictogrammers MDI 7.4.47 glyphs for automotive and communication status. Replace rough paths and text stand-ins with native-size antialiased assets; neutral stays N and QS/VVA stay badges. Compile only 12 symbols, preserve source/license/hash records, and reuse the locked converter. This supersedes the draft icon artwork. Generic symbols do not establish OEM or regulatory conformity.

## Dashboard component ownership — 2026-10-19

Use fixed-size value components for the seven dashboard responsibilities. Own top-level LVGL objects with a standard unique pointer/deleter, retain private borrowed descendant handles and let LVGL delete descendants recursively. Destroy components before their dashboard parent; keep stored gradient descriptors alive until their objects are deleted. This gives independently reusable components without extra grouping objects, wrapper allocations, virtual interfaces or component tasks. Consolidate layout bounds in Geometry.h and preserve the approved render baseline. Portable presentation extraction follows separately.

## Portable display behavior and input effects — 2026-10-19

Keep bounded text formatting, quality/lamp decisions and demonstration thresholds in embedded/presentation, independent of rendering/platform libraries. Keep display-local options/actions in embedded/application and key translation in the host adapter. Vehicle-affecting selections return typed requests instead of mutating observations in application/UI code. Only the simulator currently handles a TCS-toggle request; production commands still require a reviewed capability/protocol contract. Do not create unused settings navigation actions before their screens are implemented.

## Host optimization closure — 2026-10-19

Preserve 4 bpp font quality and subset the large fonts by their actual roles. Reuse bundled Inter for LVGL's default font and explicitly disable unused widget/layout features while keeping current theme styles. Share this feature profile without pretending it configures a board allocator, RTOS or DMA. Use small last-input keys for numeric formatting instead of a second full telemetry/text cache; defer instrumentation behind opaque lifecycle artwork but reveal latest state immediately for valid urgent warnings. Report the measured host memory tradeoff and asset sizes separately from future firmware measurements. UART/persistence implementation follows its reviewed contracts, rather than introducing unused optimization scaffolding.

## Neon lifecycle storyboard — superseded by D15

The earlier draft used two RGB565+A8 neon textures (336 KiB), radial streaks, flashes and a forward zoom. D15 replaces that artwork and renderer. Its source assets, prompts and implementation were preserved in a verified external backup before removal; they are no longer active repository resources. The two-second duration, retained-power requirement and immediate urgent-warning behavior remain in force.

## R15 Light Signature — concept 02

The user selected the headlight-inspired concept board. Retain the two-second timing and urgent-warning/cancellation behavior, replace the neon emblem/rays/flashes/zoom with procedural mirrored blades and one small A8 white wordmark. Remove the superseded image sources, compiled textures, timeline and tests after verified backup. Keep the approved dashboard unchanged. Record the chosen concept under docs/reference. The animation payload falls from 336 KiB to 12 KiB; target performance and shutdown power remain unverified.

## D16 — Angular instrument numerals (2026-10-19)

User selected the R1M-inspired angular speed/gear concept, retained the existing continuous 44 px RPM bar, requested speed units beside the digits, a bold RPM value and red scale labels 11–13. Implemented with bundled OFL Chakra Petch Bold Italic subsets; this is an approximation of the AI artwork, not Yamaha’s factory font. Current lap also uses Chakra Petch Bold Italic at 74 px so the maximum `99:59.99` fits the 320 px field; clock retains Barlow and labels retain Inter. Visible glyph bounds keep speed and lap centered despite italic side bearings. The final RPM readout restores the original small 16 px size: only the grouped number `8,500` uses Inter SemiBold, while the `RPM` suffix retains Inter Medium to match the caption; red labels do not change shift logic or assert a measured engine limit. Previous sources/docs were archived in `build/backups/before-racing-typography.tar.gz` before replacement.

User subsequently requested heavier Chakra numerals. The generator now applies a half-pixel coverage expansion inside the existing glyph bounds, with no runtime effects or new font dependency. Current lap is 74 px to preserve the gap next to speed for the widest timer strings.

## D17 — Fractional fuel gauge and reserve cue (2026-10-19)

User requested a slight italic fuel gauge and depletion within each piece. Keep eight pieces with a 4 px skew and partial coverage at the normalized signal’s 1% resolution. Supplied capacities are 11 L total and approximately 1.76 L reserve, so the display marks 16% and shows steady `RESERVE` at/below that threshold. This is a nominal percentage cue, not a verified volume calculation or sensor calibration. Unknown/stale fuel does not assert reserve. Existing warning colors/blinking and demo Trip F behavior remain separate. Callback state is bounded and retained; changed coverage/colors invalidate only affected segment objects.
