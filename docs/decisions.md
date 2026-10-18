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
| D08 | Accepted | Key-on boot animation and key-off shutdown animation each last 1–1.5 s; shutdown depends on retained hardware power |
| D09 | Proposed | ESP-IDF, FreeRTOS, C++17, LVGL 9 candidate and SDL host preview; pin compatible versions after board validation |
| D10 | Proposed | Single LVGL owner, bounded state mailbox/event queues, RGB565; buffering selected by measured driver behavior |
| D11 | Proposed | Framed binary UART with integrity checks; finalize wire specification and golden vectors before coding |
| D12 | Accepted | No unverified Yamaha facts; initial real-bike work passive/read-only; critical failure behavior needs separate review |
| D13 | Accepted | Git operations stay under user control; generated builds/captures are not source |

Changes to consequential decisions must record date, rationale, implications and unresolved questions. Supersede decisions explicitly when direction changes.

## Implementation selections — 2026-10-18

The host preview uses LVGL 9.2.2, SDL2, C++17 and CMake. LVGL source is hash-pinned and Inter/Barlow conversion uses locked lv_font_conv 1.5.3. Firmware framework compatibility remains pending. Lifecycle animation duration is 1250 ms; artwork is a lightweight initial progress-line design, with urgent-warning exposure during animations.

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
