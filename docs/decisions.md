# Design decisions

Baseline: 2026-10-18. Accepted direction is distinguished from implementation proposals.

| ID | Status | Decision and reason |
| --- | --- | --- |
| D01 | Accepted | Target the user's Yamaha R15 V4; personal prototype before commercialization |
| D02 | Accepted | Two ESP32 MCUs: vehicle acquisition/calculations/Wi-Fi/BLE and separate display rendering |
| D03 | Accepted | UART exchanges normalized observations; presentation remains independent of vehicle protocols |
| D04 | Accepted | LVGL on the selected Elecrow 7-inch 800 x 480 display family; exact PCB revision pending |
| D05 | Accepted | Preserve the specified Street/Track UI, Bricolage font, themes, RPM gradient, icons and fuel behavior |
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

The host preview uses LVGL 9.2.2, SDL2, C++17 and CMake. LVGL source is hash-pinned and Bricolage conversion uses locked lv_font_conv 1.5.3. Firmware framework compatibility remains pending. Lifecycle animation duration is 1250 ms; artwork is a lightweight initial progress-line design, with urgent-warning exposure during animations.
