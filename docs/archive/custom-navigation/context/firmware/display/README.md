# R15 display desk-test target

ESP-IDF 5.5.4 / C++17 / LVGL 9.2.2, with PlatformIO as dependency/build runner. This is separate from the root Mac simulator and does not use Arduino. The component lock pins LVGL. The pioarduino platform and ESP-IDF archive URLs are exact versions.

From the repository root:

```sh
bash scripts/flash-display.sh --build-only
bash scripts/flash-display.sh
```

The second command is the user-operated flash step. It checks the saved, verified factory backup, identifies this delivered ESP32-S3/4 MB board, and writes the build-generated combined image at zero using esptool 4.8.1, 115200 baud and `--no-stub`. No full-chip erase command is used. Default port `/dev/cu.usbserial-110`; override with `R15_DISPLAY_PORT`. Outputs and isolated flash tools stay under ignored `build/`. A missing/moved backup can be located via `R15_FACTORY_BACKUP`; the expected SHA-256 remains checked.

Close any serial monitor before flashing. Never flash the Mac build or an application-only binary at address zero. The image packager reads IDF's generated offsets, checks partition/image bounds and records input hashes in `build/display/crowpanel-v3/r15-display-image.json`.

## Board provenance

Elecrow DIS08070H PCB V3.0 is visible in the delivered board photos. User-operated esptool identifies ESP32-S3 revision v0.2, 4 MB flash and 8 MB embedded PSRAM. The official `example/V3.0/PlatformIO70` was built, flashed with hash verification and reset tested by the user; display stable and touch responds. The board adapter derives RGB pins, 24 MHz timings, ten-line bounce buffers and PCA9557/GT911 reset/address behavior from that working example. This target replaces its Arduino/LovyanGFX wrapper with native ESP-IDF RGB/I2C APIs; those native adapters still require user panel validation.

SHA-256 of the vendor reference files inspected:

| Vendor relative file | SHA-256 |
| --- | --- |
| `src/main.cpp` | `f940b8c964120a225fb8bc5a4bcf02cc105a5c31f17cb90286be45660376d14e` |
| `include/touch.h` | `c7af7b1df3345570266def5056f34190d30ab566056cb3224270b4d805f55765` |
| `patches/LovyanGFX/esp32s3/Bus_RGB.cpp` | `4f335fb2cce56b886f214883208889aa97f11dabfa641438a6557eab389ed5ce` |

Source: [official V3.0 example](https://github.com/Elecrow-RD/CrowPanel-7.0-HMI-ESP32-Display-800x480/tree/master/example/V3.0/PlatformIO70). Native driver: [ESP-IDF 5.5.4 RGB documentation](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/lcd/rgb_lcd.html).

## Bench behavior and controls

One `app_main` task owns all LVGL operations and application state. The RGB ISR only signals a static semaphore. Two RGB565 scanout buffers use 1,536,000 PSRAM bytes, with two internal ten-line bounce buffers. LVGL DIRECT mode draws changed areas into the off-screen buffer and synchronizes changed areas between buffers. Only the final dirty-area callback presents a frame. Flush waits once (bounded to 100 ms) for the ESP-IDF bounce-buffer `on_frame_buf_complete` event before allowing the previous buffer to be reused. App/UI and driver interrupt allocation are pinned to CPU0. A short critical section discards an idle completion and selects the existing driver-owned buffer atomically relative to that core’s EOF handler. The pinned IDF 5.5.4 bounce-buffer code path performs no copy, allocation, cache sync or blocking I/O inside that critical section; no color-transfer callback is installed. This synchronization must be reviewed again if IDF, buffer mode, task affinity or callbacks change. A timeout turns off the backlight and aborts rather than handing a potentially active buffer to LVGL. I2C transactions are bounded to 10/25 ms. NVS commits run on a separate bounded storage worker, never inside UI/render callbacks. Flash/cache contention during saves still requires panel testing.

Startup uses the existing shared R15 artwork/controller, with a 2,000 ms artwork clock distinct from reset/panel startup. Default readings are unavailable until a real adapter supplies them; synthetic mode is explicitly labelled. Header time/date uses phone or USB sync when available, otherwise remains unavailable. Tapping top-left switches Street/Track, top-right outside the gear Midnight/Daylight, lower-left trip page, lower-middle info page. Tap the main center area to start/stop the moving bench demo. Tap the lower-right fuel area to preview shutdown; after it finishes black, tap anywhere to boot again. Touch during shutdown is ignored so the complete two-second preview can play. Touch is consumed on release, and multi-touch/invalid coordinates are rejected. No external GPIO control is enabled.

Serial console at 115200 provides the same non-touch actions: `l` layout, `t` theme, `n` trip, `i` info, `u` units. `d` (or a center tap) selects a repeating synthetic sweep: 10 seconds rising from 0 to 160 km/h and 0 to 13,000 RPM, two seconds held high, ten seconds falling, two seconds stopped. These independently chosen samples are not a vehicle speed/RPM/gear calibration. The existing UI shift tint starts at 10,500 RPM and flashes at/above 11,400 RPM (300 ms on/off phases); these are UI demonstration thresholds, not verified Yamaha limits. The demo is clearly marked `BENCH / SYNTHETIC DATA`; real ODO, phone and clock remain unavailable. Fuel repeats full-to-empty-to-full over 48 seconds, holding empty for four seconds to expose pump flashing. Automatic tell-tales advance every three seconds through left, right, high beam, ABS, engine, coolant, hazards, then an explicitly labelled all-icon flashing lamp test. The all-icon test also exposes unsupported icon artwork; it does not claim those features are connected. `w` toggles test warning/turn/beam observations, marked `BENCH / TEST WARNINGS`. `b` restarts bench boot and clears the demo. `o` (or a fuel-area tap) freezes the synthetic snapshot, clears synthetic urgent-warning overrides and runs the two-second bench shutdown, then stays black. This prevents auto-demo warnings from cancelling the requested artwork; shared real-warning priority remains unchanged. `b` or a tap after black restores operation through boot, initially with unavailable data. Reset discards every bench setting/sample. No retained-power or real ignition behavior is implemented.

Every ten seconds the console reports internal free/minimum/largest heap block, free PSRAM, main stack margin, flush count and maximum flush duration. These become measurements only after the user runs the image. Record the output and inspect both layouts/themes, unavailable values, warnings, boot and repeated reset. Touch Settings/Appearance NVS, discrete-input/return UART and authenticated phone pairing/metadata are implemented; full telemetry, Wi-Fi and maps remain later work. Current evidence is summarized in the system implementation plan.

## Recovery

The factory image was read completely and verified against flash before the vendor example replaced it. It is outside Git at `~/Desktop/R15-factory-backup/crowpanel-v3-factory-rom.bin`, SHA-256 `babb6b68f22ddaf221760a6321a655405cc0b74abfebab1912491a17e48a50ff`. Restoration has not been exercised. To restore this complete image, the original known-working esptool command format is:

```sh
build/display-tools/bin/python -m esptool --chip esp32s3 --port /dev/cu.usbserial-110 --baud 115200 --no-stub write_flash --flash_size 4MB 0x0 "$HOME/Desktop/R15-factory-backup/crowpanel-v3-factory-rom.bin"
```

This is explicitly a restore/write command, not a routine build step. A transfer failure needs diagnosis before retrying; never erase fuses or change pin assignments to troubleshoot it.

## First-panel observations and follow-up

The user flashed the initial R15 image and supplied photo/video evidence of artwork followed by Street; layout/theme touch controls and reset operation are user confirmed. The user reports smoother animation on the Mac. The initial conservative full-frame port waited for two VSYNCs per update; at the nominal vendor timing (928 × 525 clocks per frame, 24 MHz pixel clock request), that imposed a nominal ceiling near 24.6 updates/s before software rendering. Actual divider, render and scanout timings must be measured on the panel. The new log reports rendered updates/s over ten seconds, including idle time, rather than claiming panel refresh FPS. The follow-up replaces that full redraw/two-VSYNC path with changed-area DIRECT rendering and one true frame-buffer completion wait. Pixel-clock/panel timings are unchanged. Host comparison proves pixel equivalence across RPM/speed, theme/layout, warning, unavailable and lifecycle transitions; it does not prove hardware scanout synchronization or panel FPS. User panel inspection must check tearing/shift/flicker and compare rendered-update logs after this change.

Icon opacity/tone intentionally matches the shared host policy: available/active uses its role color at 255 opacity; available/inactive and unsupported use muted color at 31; unknown uses amber at 150. In the initial unavailable-data screen, amber/partial-brightness icons mean unknown, not active warnings. Camera appearance alone cannot establish RGB color accuracy. The sweep supplies known inactive warnings for comparison; `w` exercises active warnings.

## Text and redraw follow-up (2026-10-23)

Information labels and units now use pure black/white per theme; indicators and warning severity colors remain separate. Historical FULL/DIRECT comparison covered multiple dirty rectangles, buffer synchronization, theme/layout changes and lifecycle covers; its automated suite is now removed/deferred. A representative 30-update speed/RPM sequence redraws 1,853,490 versus 11,520,000 pixels on the host. This is reduced drawing work, not a measured target speedup. Shared real warning activation is not interpolated or delayed.

## Touch Settings, clock and two-second boot

Boot/shutdown artwork each lasts 2,000 ms. Tap the gear beside clock/date for Settings; touch selects tiles, choices, action rows and Back. Current prototype menu access is unrestricted by speed/data quality/UART/warnings; warning truth stays live and lifecycle owns screen visibility. Appearance and Clock & Date Apply save validated display preferences through the NVS worker. Clock formats/timezone preview immediately; Cancel/Back restores entry choices. Phone availability now follows validated vehicle radio/UART state; connected name/battery and phone clock sync require current authenticated app metadata.

Run `bash scripts/sync-clock.sh /dev/cu.usbserial-10` to supply actual Mac time and local UTC offset; replace the display port if needed. Keep the helper running for boot/reset resync; stop before flashing or another console. The display keeps ticking while powered, but cold power loses time and requires sync again. Preferences save formats/timezone, never a falsely retained clock. See [clock contract](../../docs/ui.md#clock-date-and-connection-indicator).

Native UI refresh is 16 ms, bounded by panel/render completion. Actual panel smoothness and flash-write coexistence still require measurements. The user confirmed the prior Settings/Appearance follow-up working; this clock follow-up requires its own panel acceptance.

## Vehicle-to-display UART bench

The default source is now UART1, RX GPIO38 on GPIO_D, 115200 8N1 without flow control. TX is unused. Vehicle GPIO17 sends the existing version-3 36-byte R15G discrete-input packet every 20 ms; display UART0 stays available for Mac programming and serial controls. The display reads at most 512 bytes and eight queued driver events per UI cycle, without waiting and outside LVGL rendering. RX storage is 1024 bytes; over-budget backlog/overflow/framing/read faults discard buffered data and reset observations/click baselines, with a counter in the ten-second console report. A fresh complete packet then recovers the link. The parser rejects bad CRC/version/ranges, duplicate/reordered sequence/uptime and rebases actions on reconnect/session changes.

The gear is amber before a packet, normal theme foreground for an advancing valid stream, and red after 1,000 ms without an accepted packet. Clock/date are reserved for real local time. Gear N–6, left/right observed phase, high beam and Trip/Info clicks are supported; speed/RPM/fuel/clock/ODO/phone stay unavailable. Link loss never triggers shutdown. This is the narrow development input protocol, not the full future UART phone/map/command contract. It has no return channel or authentication. The vehicle samples these discrete inputs immediately before sending; v3 has no independently aged vehicle-source observations.

Serial `v` resets/re-enters live UART, `p` explicitly selects the parked Settings fixture, `d` enters the moving demo (again returns to live), and `b` reboots into live. Synthetic and UART observations are never blended. Receiver bytes/clicks are drained during demos so old clicks are not replayed. Settings opens in live mode without a speed source and never synthesizes zero speed. Explicit `p` remains optional demo selection.

### Flash and verify

1. Power both off and disconnect their interconnect before separately flashing. Close serial monitors.
2. Run `bash scripts/flash-display.sh` for the display and `bash scripts/flash-vehicle.sh --port /dev/cu.YOUR_VEHICLE_PORT` for the vehicle. Use the actual vehicle port; the previously observed `/dev/cu.usbserial-0001` can change. Each script builds its own firmware and applies its board/backup guards.
3. Power both off, reconnect GPIO17 → IO38 and GND → GND, leave power/NC leads insulated, then reconnect separate USB supplies.
4. After boot, expect the gear to turn its normal theme color even with no acquisition inputs attached. Gear should be unavailable when no position is asserted. Enabled unwired lamp inputs read inactive in the existing weak-pulldown bench profile; this is not wire-fault detection.
5. For an engine-off, harness-disconnected desk stimulus, connect an isolated dry switch from vehicle 3V3 to one input at a time: GPIO6 neutral, GPIO8 gear 1, GPIO4 left, GPIO5 right, GPIO7 high beam. Remove the stimulus between gear selections. Never connect raw bike voltages. GPIO14/15 momentary presses exercise Trip/Info pages; numbers stay unavailable.
6. Power down both boards, disconnect only the UART signal, then power both again: expect an amber gear. To test timeout while running without rewiring powered hardware, stop/reset the sender long enough for the one-second deadline; check red stale gear and recovery with no replayed clicks. Inspect accepted/overflow counters and repeat independent resets/boot order. A simple quick reset may recover before timeout and need not visibly turn the gear red.

Earlier host checks and build-only packages are software evidence; automated suites are now removed/deferred. Actual GPIO17→GPIO38 communication, input response, electrical levels, load/soak and reset/loss recovery require the user's bench results.

User bench result (2026-10-29): both updated images were user-flashed. Initial UART WAITING was traced by the user to a loose connection; reseating it produced UART / LIVE. This confirms receipt of valid packets over the reported vehicle GPIO17 → display GPIO_D IO38/GND link with separate Mac USB power. Individual input stimulus, cable-loss/stale recovery, independent resets, soak and vehicle use remain separate pending tests.

User bench acceptance (2026-10-29): the user reports the display follow-up working and confirms the described jumper-input tests working over the physical UART link. This covers the tested neutral/gear, left/right/high-beam and Trip/Info behavior, plus functional acceptance of the current display changes. This is user-reported desk evidence, not instrumented timing, electrical-margin, power-loss/RTC, soak or bike verification. Mac USB and authenticated phone UTC/offset are supported sources; no retained RTC hardware is assumed.

## BLE pairing bench

Vehicle BLE bonding, return controls, saved reconnect and authenticated connected phone name/battery/time are implemented; core companion behavior is user-confirmed, with extended faults/load still pending. Follow [wiring, flashing and Android acceptance](../../docs/development.md#vehicle-ble-uart-bench). Use compatible current images on both boards; metadata connection identifiers extend the older R15P status contract. The working GPIO17 → IO38 receive link remains; add display UART0 TX43 → vehicle GPIO18 with no power connection. Background continuity is the next gate; follow [firmware background sync](../../docs/background-sync.md) and the [current phone protocol](../../mobile/docs/protocol.md). Full telemetry, Wi-Fi and navigation remain pending.

## Phase 1 maps

Map firmware is implemented and build/host-preview verified; physical transfer/resource/load acceptance and Android GPS remain pending. See the [implementation handoff](../../docs/map-phase-1.md), [exact wire contract](../../mobile/docs/protocol.md#phase-1-map-extension--implementation-specification) and [build/bench evidence](../../docs/development.md#phase-1-map-firmware-integration). Existing guarded flash commands, pins, baud rate, bond storage and partition policies are preserved.
