# Vehicle ESP32: configurable discrete input test

Separate ESP-IDF 5.5.4 / C++17 image for the user-photographed 7SEMI ESP32-S3 DevKit, reported N8R8. Generic DevKitC-1 profile selects 8 MB flash with PSRAM unused; verify module/flash before flashing. Never flash this image onto the display.

## Wiring and configuration

Latest user report (2026-10-24): optocoupler outputs are active HIGH, approximately 2–3.3 V active and 0.05–0.2 V inactive. This supersedes earlier conflicting polarity reports. At 3.3 V supply ESP32-S3 specifies VIH >= 0.75 × VDD (2.475 V); 2 V is not a guaranteed HIGH. Check each output against HVG while connected, including pulse high/low levels; a meter average of flashing signals is not a peak measurement. Resolve low HIGH levels before relying on readings. Source: [Espressif datasheet, DC characteristics](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf).

| Signal | Opto output / ESP GPIO | Default |
| --- | --- | --- |
| Left | OUT1 → GPIO4 | Enabled, active HIGH |
| Right | OUT2 → GPIO5 | Enabled, active HIGH |
| High beam | Separate conditioned output → GPIO7 | Enabled, active HIGH |
| Neutral | Separate conditioned output → GPIO6 | Enabled, active HIGH |
| Gear 1 | Separate conditioned output → GPIO8 | Enabled, active HIGH |
| Gear 2 | Separate conditioned output → GPIO9 | Enabled, active HIGH |
| Gear 3 | Separate conditioned output → GPIO10 | Enabled, active HIGH |
| Gear 4 | Separate conditioned output → GPIO11 | Enabled, active HIGH |
| Gear 5 | Separate conditioned output → GPIO12 | Enabled, active HIGH |
| Gear 6 | Separate conditioned output → GPIO13 | Enabled, active HIGH |
| Trip button | Conditioned button output → GPIO14 | Enabled, active HIGH |
| Info button | Conditioned button output → GPIO15 | Enabled, active HIGH |

GPIO4–15 are the software bench allocation; confirm the numbered headers exist on your actual vehicle board. Board schematic verification remains pending. These are vehicle MCU pins, not Elecrow display pins. No vehicle voltage connects directly to ESP GPIOs. Opto output HV → 3V3, HVG → MCU GND. Wire with power off. Keep the existing reviewed input conditioning and OEM function; input loading/resistor suitability and power/ground isolation still require electrical review. Use the USB serial bridge at 115200 8N1 for the Mac test; do not simultaneously inject bike power into the board's 5 V header.

All twelve inputs are enabled in `main/InputConfig.h`. **Flash once**, then move the optocoupler's bike-side input to the function being tested and its output-side wire to that function's fixed GPIO. No code edits/reflash are needed between functions. Example: test left on GPIO4; power off, move the bike input from left to the confirmed neutral wire and move OUT1 from GPIO4 to GPIO6; power on, select N and observe N on the Mac. Repeat for gears/high beam. Keep OEM wiring functional; make connections with power off and engine off for initial observation. No harness connector pin numbers are inferred by this map.

Each config entry remains editable as `{GPIO, activeHigh}`; `-1` disables it. Array order is **N, 1, 2, 3, 4, 5, 6**. Duplicate pins or pins outside GPIO4–18 fail compilation. Changing code requires rebuilding/reflashing. All twelve default to active HIGH with internal weak pull-downs, keeping unwired bench inputs inactive. An active-LOW override uses a weak pull-up instead. Check output levels with these pulls connected; they do not repair a marginal HIGH or detect unplugged wires. Unwired lamps consequently appear inactive in this bench profile. Use the current two channels one by one or in pairs, then connect ten separately conditioned outputs to test everything together. All ESP signal pins are inputs; optocoupler IN/OUT routing changes physically.

No selected gear → unavailable; exactly one → that gear; multiple → invalid. Neutral is never inferred from all inputs low. Explicitly disabled lamps remain unavailable; enabled unwired bench lamps read inactive. Samples/packets run every 20 ms with no debounce; turn signals carry observed phase, with no second software blink. Disconnected inputs/output power loss cannot be distinguished reliably from inactive levels by this circuit. This is an observation test, not a validated replacement instrument.

## Flash and preview: Mac workflow

Run from the repository root:

```sh
# Optional: compile/package without accessing any ESP
bash scripts/flash-vehicle.sh --build-only

# Identify the vehicle board's USB serial port (unplug/replug to distinguish it)
bash scripts/flash-vehicle.sh --list-ports

# Replace the placeholder with that exact vehicle port
bash scripts/flash-vehicle.sh --port /dev/cu.YOUR_VEHICLE_PORT

# After flashing finishes, open the Mac dashboard using the same port
bash scripts/preview.sh --vehicle-serial /dev/cu.YOUR_VEHICLE_PORT
```

Use the USB-to-UART bridge socket on the vehicle board, not its separate native USB socket; this firmware sends UART0 at 115200 8N1. Close PlatformIO/Arduino serial monitors and any existing preview before flashing. No display is needed. Keep the vehicle ESP plugged into the Mac after flashing. The flash script builds the separate image, requires an explicit serial port, checks ESP32-S3 and 8 MB flash, rejects the known display identity/4 MB capacity, reads and verifies an entire 8 MB backup, then writes and verifies the combined image. It prints the unique backup path under ignored `build/vehicle/backups/` and the preview command. Keep that backup for recovery. Backup/verification can take several minutes. No firmware is flashed by `--build-only` or `--list-ports`.

If connection fails, hold BOOT, press/release RESET, release BOOT, and rerun the flash command. If higher-speed transfers fail, retry with `R15_VEHICLE_BAUD=115200 bash scripts/flash-vehicle.sh --port /dev/cu.YOUR_VEHICLE_PORT`. After flashing, press RESET if needed to run the application. The script defaults to 460800 only for flashing/backups; the preview always uses 115200. `R15_PIO` overrides the installed PlatformIO executable if needed. An unexpected chip/flash size stops before writing and needs investigation rather than bypassing the check.

With power off, connect opto HV to ESP 3V3, HVG to ESP GND, and OUT1/OUT2 to the mapped GPIOs for the signals under test. Power the ESP from Mac USB only; do not also feed its 5 V pin from the bike. Keep the opto bike-side connections/conditioning appropriate to the measured harness signals; never connect bike signal voltage directly to GPIO. Turn on the bike ignition for the passive, engine-off observation after wiring is secure. Start the preview before or after key-on; USB power does not itself mean ignition-on data.

The Mac preview shows N/gear, left/right arrows and high-beam icon; a measured flashing signal flashes the arrow directly. N also lights the neutral tell-tale. No selected gear shows a dash; conflicting gear inputs are invalid. Other vehicle measurements remain unavailable. Default boot artwork lasts 2 seconds; Tab changes layout, D changes theme. Keyboard demo signal keys do not override USB observations. Terminal diagnostics periodically show left/right/high-beam/gear state and quality. Stop with Ctrl+C before moving wires, power off for rewiring, then restart. After a USB disconnect, restart with the actual current port; automatic reopening is pending. UART0 has exclusive binary access, so keep serial monitors closed. The dedicated GPIO17 transmitter now also supports the physical display; see the direct UART section below.

## Trip and Info clicks

GPIO14 advances Street ODO → TRIP 1 → TRIP 2 → ODO. GPIO15 advances the existing Info pages: coolant → Track → real-time fuel → average fuel → average speed → TCS → unidentified → coolant. Track layout retains its best-lap panel, so Trip clicks have no visible effect there; use Tab for Street. Only page selection changes; values stay unavailable without real sources. No trip reset, TCS command, long-press action or hold repeat is implemented. Settings ignores dashboard page clicks as before.

These two channels expect separately conditioned active-HIGH output signals, with weak pull-downs. Their bike-side electrical behavior has not been measured; do not attach raw harness button wires directly to GPIO or assume they share lamp polarity. For a desk test, an isolated dry momentary switch between ESP 3V3 and GPIO14/15 exercises the active-HIGH input; never bridge this to the vehicle circuit. Per-channel polarity remains configurable in InputConfig.h. Stable press and release require 40 ms; an initially held button must be released before it can produce a click.

Button counters are repeated in every packet so one lost frame does not lose a click. First connection, reboot and stale recovery establish a baseline without replaying earlier clicks. Duplicate/reordered packets cannot repeat actions. At most eight pending clicks per button are retained; excessive jumps are discarded. This bounded development action transport does not control the bike.

This update requires reflashing the vehicle and rebuilding the preview. Stop the preview first and use the normal flash command. **Do not resume using the old factory backup:** flash now contains the previously flashed R15 image and no longer matches that backup. A normal run preserves the current image before writing the new one.

## Development protocol

`shared/protocol/GearPacket.h` implements version **3**, incompatible with the prior versions 1/2. Both firmware and host must be rebuilt. Fixed 36-byte packets: `R15G` at 0–3, version at 4, gear quality at 5 (0 unavailable, 1 available, 2 invalid), gear at 6 (0–6 when available, otherwise zero), raw left/right levels at 7 (bits 0/1), boot session at 8–11, sequence at 12–15, sample uptime ms at 16–19, left/right/high-beam at 20/21/22 (0 unavailable, 1 inactive, 2 active), reserved zero at 23, cumulative Trip presses at 24–27, Info presses at 28–31, CRC-32/ISO-HDLC over bytes 0–31 at 32–35. Integers are little endian. Random boot sessions and wrap-aware sequence/uptime checks reject duplicate/reordered observations. One second without advancing accepted packets makes signals stale. Host I/O/buffers are bounded. No host-to-bike commands are sent.

Host input/codec tests cover N–6, conflicts/no selection, lamps, CRC, framing, version/range rejection, freshness, wrap/reset and a pseudo-terminal serial adapter. Physical wiring, electrical margins, flashing and bike behavior require separate user verification.

Verified 2026-10-24: all-ten-input firmware build/package succeeds via `flash-vehicle.sh --build-only`; GPIO map, N–6/conflict/lamps, codec and pseudo-terminal serial checks pass. Flash-script tests use mock tools to cover build-only, missing ports, wrong capacity/display identity, incomplete or unverifiable backups, and backup-before-write/verification order. No device was flashed by the agent. Weak-pull electrical behavior, port identity, real flashing and harness response still require physical verification. Existing 800 × 480 USB rendering covers N–6, conflicts/stale and both layouts/themes; no visual design changes in this follow-up.

## Manual-BOOT reset/reconnect recovery

User hardware evidence (2026-10-24): CP2102 port `/dev/cu.usbserial-0001`, ESP32-S3 revision v0.2, embedded 8 MB PSRAM, detected 8 MB quad flash, MAC `14:c1:9f:55:d2:14`. Automatic boot entry needed manual BOOT assistance. An entire 8 MB read completed at 115200, but the former script hard-reset between commands and failed reconnecting for backup verification; no new image was written in that run. The saved image is not yet verified against flash.

The corrected script leaves the flasher stub running through identification, backup, verification and write, syncing without reset for each following command. Only final firmware verification requests a hard reset. Hold BOOT during initial Connecting and release when the stub/transfer starts. Do not press RESET or unplug between stages. This sequence is covered by mock regression checks; physical retry remains pending.

Resume an interrupted pre-write backup using the explicit saved path:

```sh
R15_VEHICLE_BAUD=115200 bash scripts/flash-vehicle.sh --port /dev/cu.usbserial-0001 \
  --resume-backup build/vehicle/backups/before-flash.oNaUP1/vehicle-8mb.bin
```

Resume requires an 8 MB file, adjacent saved identity matching the connected MAC, and successful full backup verification against current flash before writing. It avoids rereading the 8 MB; verification still takes time. A missing/incomplete/wrong-board/mismatched backup stops before write. Use resume only while flash still matches that backup; after a partial or complete firmware write, a fresh normal run is required. BOOT can be released after initial connection; after completion, press RESET once with BOOT released if the application does not start.

Trip/Info verification: debounce, startup-held suppression, bounce/hold behavior, counter wrap, duplicate/stale/reset baselines and pseudo-terminal action delivery pass, along with existing page-action checks. Native build-only passes; physical button behavior remains unverified.

## Dedicated display output

UART1 TX is assigned to photographed header GPIO17, 115200 8N1, no flow control, transmitting the same version-3 R15G packet as USB UART0 every 20 ms. There is no RX wire, display command handler, phone or map stream in this slice. InputConfig rejects a collision between GPIO17 and any acquisition input. USB binary output remains compatible with the Mac preview. UART1 uses a nonblocking FIFO write (36 bytes versus the 20 ms sample interval); a short write is not retried, and the display discards damaged frames using CRC/resynchronization. No hard real-time transmission guarantee is claimed.

Connect GPIO17 → display GPIO_D IO38 and GND → GND, leaving display cable 3V3/NC disconnected. Keep UART0 headers disconnected from another transmitter while USB bridges are present. Disconnect the interconnect with both boards off before separately flashing; reconnect with both off and power separately afterward. Full commands and acceptance checks: [display UART bench](../display/README.md#vehicle-to-display-uart-bench). Build-only/host tests do not establish successful physical communication.

### Initial connection fails with `Invalid head of packet (0x52)`

The build can succeed while the initial bootloader connection fails; no write has occurred at this stage. `0x52` is ASCII R and matches the start of the running R15G stream, suggesting the application may still be running rather than entering download mode; it is not conclusive proof of the cause. The script now exposes initial esptool progress while still capturing identity for its guards. Close monitors, disconnect the interconnect with both boards off, then retry at 115200. Hold BOOT through initial connection; if necessary keep BOOT held while tapping RESET, and release BOOT when the stub starts. Do not reset between backup/verification/write stages.
