# R15 Smart Cluster

A dual-ESP32 instrument cluster for the Yamaha R15 V4, with an 800 x 480 LVGL interface. The vehicle MCU acquires/validates observations and handles calculations/connectivity; the display MCU renders normalized state received over UART.

The native macOS preview includes Street/Track dashboards, Midnight/Daylight themes, Settings, saved Appearance choices and unified Phone settings. Native display and vehicle discrete-input bench firmware communicate over a one-way UART link; full telemetry/radio/map integration and vehicle verification remain pending. A custom moving map with an Android-first Flutter companion app and future iOS support is planned; embedded map feasibility remains pending.

## Run the preview

**Looking for a command?** See the [command reference](docs/commands.md) for display/vehicle flashing, previews, phone connection, tests and asset tools.

To build and flash the latest display desk-test UI over USB: `bash scripts/flash-display.sh`. See the [display instructions](docs/commands.md#flash-the-display-with-the-latest-ui) for port selection and checks.

Prerequisites: C++ compiler, CMake, Ninja and SDL2.

```sh
bash scripts/preview.sh
```

Tab changes layout, D previews themes, arrows adjust speed/RPM, brackets adjust fuel, O simulates ignition and X toggles unavailable data. Press P then S for Settings; arrows cycle focus, Enter selects and Backspace/Esc returns. Esc quits outside Settings.

See the [step-by-step preview guide](docs/getting-started.md), [project documentation](docs/README.md), [architecture](docs/architecture.md) and [implementation plan](docs/implementation-plan.md).

## Development verification

Automated test files were removed at the user's request on 2026-10-31 and are deferred until requested again. Use affected builds, static analysis, runtime diagnostics, manual previews and physical bench checks. See [RULES.md](RULES.md) for the project-wide rule. Simulated output is not bike verification.

## Mobile companion

The independent Flutter app lives in [mobile](mobile/README.md). See its [documentation](mobile/docs/README.md) for the agreed journey, architecture and staged implementation. Mobile tooling is not required to build the cluster preview.

For the real Wi-Fi phone-to-Mac development connection, see the [connection guide](mobile/docs/mac-preview-connection.md). Native Android/Mac BLE discovery is implemented; ESP32 provisioning remains pending.

The display clock uses Mac USB sync during bench work: `bash scripts/sync-clock.sh /dev/cu.usbserial-10` (use the current display port). Settings → Clock & Date saves 12/24-hour, date-format and timezone choices. Cold power requires time sync again; no retained RTC is assumed.
