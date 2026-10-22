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

One `app_main` task owns all LVGL operations and application state. The RGB ISR only signals a static semaphore. Two RGB565 scanout buffers use 1,536,000 PSRAM bytes, with two internal ten-line bounce buffers. Flush waits for two VSYNCs before releasing the previous buffer; each wait is bounded to 100 ms. A timeout turns off the backlight and aborts rather than handing a potentially active buffer to LVGL. I2C transactions are bounded to 10/25 ms. No NVS/flash writes occur during rendering.

Startup uses the existing shared R15 artwork/controller, with a 3,000 ms artwork clock distinct from reset/panel startup. Default readings are unavailable. `BENCH / NO VEHICLE` occupies the date field; no clock is invented. Tapping top-left switches Street/Track, top-right Midnight/Daylight, lower-left trip page, lower-middle info page. Tap the main center area to start/stop the moving bench demo. Tap the lower-right fuel area to preview shutdown; after it finishes black, tap anywhere to boot again. Touch during shutdown is ignored so the complete two-second preview can play. Touch is consumed on release, and multi-touch/invalid coordinates are rejected. No external GPIO control is enabled.

Serial console at 115200 provides the same non-touch actions: `l` layout, `t` theme, `n` trip, `i` info, `u` units. `d` (or a center tap) selects a repeating synthetic sweep: 10 seconds rising from 0 to 160 km/h and 0 to 13,000 RPM, two seconds held high, ten seconds falling, two seconds stopped. These independently chosen samples are not a vehicle speed/RPM/gear calibration. The existing UI shift tint starts at 10,500 RPM and flashes at/above 11,400 RPM (300 ms on/off phases); these are UI demonstration thresholds, not verified Yamaha limits. The demo is clearly marked `BENCH / SYNTHETIC DATA`; real ODO, phone and clock remain unavailable. Fuel repeats full-to-empty-to-full over 48 seconds, holding empty for four seconds to expose pump flashing. Automatic tell-tales advance every three seconds through left, right, high beam, ABS, engine, coolant, hazards, then an explicitly labelled all-icon flashing lamp test. The all-icon test also exposes unsupported icon artwork; it does not claim those features are connected. `w` toggles test warning/turn/beam observations, marked `BENCH / TEST WARNINGS`. `b` restarts bench boot and clears the demo. `o` (or a fuel-area tap) freezes the synthetic snapshot, clears synthetic urgent-warning overrides and runs the two-second bench shutdown, then stays black. This prevents auto-demo warnings from cancelling the requested artwork; shared real-warning priority remains unchanged. `b` or a tap after black restores operation through boot, initially with unavailable data. Reset discards every bench setting/sample. No retained-power or real ignition behavior is implemented.

Every ten seconds the console reports internal free/minimum/largest heap block, free PSRAM, main stack margin, flush count and maximum flush duration. These become measurements only after the user runs the image. Record the output and inspect both layouts/themes, unavailable values, warnings, boot and repeated reset. Settings, persistence, UART vehicle transport, phone/radios, maps and actual physical navigation inputs remain later integration work.

## Recovery

The factory image was read completely and verified against flash before the vendor example replaced it. It is outside Git at `~/Desktop/R15-factory-backup/crowpanel-v3-factory-rom.bin`, SHA-256 `babb6b68f22ddaf221760a6321a655405cc0b74abfebab1912491a17e48a50ff`. Restoration has not been exercised. To restore this complete image, the original known-working esptool command format is:

```sh
build/display-tools/bin/python -m esptool --chip esp32s3 --port /dev/cu.usbserial-110 --baud 115200 --no-stub write_flash --flash_size 4MB 0x0 "$HOME/Desktop/R15-factory-backup/crowpanel-v3-factory-rom.bin"
```

This is explicitly a restore/write command, not a routine build step. A transfer failure needs diagnosis before retrying; never erase fuses or change pin assignments to troubleshoot it.

## First-panel observations and follow-up

The user flashed the initial R15 image and supplied photo/video evidence of artwork followed by Street; layout/theme touch controls and reset operation are user confirmed. The user reports smoother animation on the Mac. The conservative full-frame port waits for two VSYNCs per update; at the nominal vendor timing (928 × 525 clocks per frame, 24 MHz pixel clock request), this alone caps update throughput near 24.6 updates/s before software rendering. Actual divider, render and scanout timings must be measured on the panel. The new log reports rendered updates/s over ten seconds, including idle time, rather than claiming panel refresh FPS. No synchronization optimization has been applied.

Icon opacity/tone intentionally matches the shared host policy: available/active uses its role color at 255 opacity; available/inactive and unsupported use muted color at 31; unknown uses amber at 150. In the initial unavailable-data screen, amber/partial-brightness icons mean unknown, not active warnings. Camera appearance alone cannot establish RGB color accuracy. The sweep supplies known inactive warnings for comparison; `w` exercises active warnings.
