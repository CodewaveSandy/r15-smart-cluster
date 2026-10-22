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

Startup uses the existing shared R15 artwork/controller, with a 3,000 ms artwork clock distinct from reset/panel startup. Default readings are unavailable. `BENCH / NO VEHICLE` occupies the date field; no clock is invented. Tapping top-left switches Street/Track, top-right Midnight/Daylight, lower-left trip page, lower-middle info page. Touch is consumed on release, and multi-touch/invalid coordinates are rejected. No external GPIO control is enabled.

Serial console at 115200 provides the same non-touch actions: `l` layout, `t` theme, `n` trip, `i` info, `u` units. `d` selects static synthetic instrument observations, clearly marked `BENCH / SYNTHETIC DATA`; real ODO, phone and clock remain unavailable. `w` toggles test warning/turn/beam observations, marked `BENCH / TEST WARNINGS`. `b` restarts bench boot, `o` runs the two-second bench shutdown and stays black. `b` restores operation. Reset discards every bench setting/sample. No retained-power or real ignition behavior is implemented.

Every ten seconds the console reports internal free/minimum/largest heap block, free PSRAM, main stack margin, flush count and maximum flush duration. These become measurements only after the user runs the image. Record the output and inspect both layouts/themes, unavailable values, warnings, boot and repeated reset. Settings, persistence, UART vehicle transport, phone/radios, maps and actual physical navigation inputs remain later integration work.

## Recovery

The factory image was read completely and verified against flash before the vendor example replaced it. It is outside Git at `~/Desktop/R15-factory-backup/crowpanel-v3-factory-rom.bin`, SHA-256 `babb6b68f22ddaf221760a6321a655405cc0b74abfebab1912491a17e48a50ff`. Restoration has not been exercised. To restore this complete image, the original known-working esptool command format is:

```sh
build/display-tools/bin/python -m esptool --chip esp32s3 --port /dev/cu.usbserial-110 --baud 115200 --no-stub write_flash --flash_size 4MB 0x0 "$HOME/Desktop/R15-factory-backup/crowpanel-v3-factory-rom.bin"
```

This is explicitly a restore/write command, not a routine build step. A transfer failure needs diagnosis before retrying; never erase fuses or change pin assignments to troubleshoot it.
