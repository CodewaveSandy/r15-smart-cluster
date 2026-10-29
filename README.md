# R15 Smart Cluster

A dual-ESP32 instrument cluster for the Yamaha R15 V4, with an 800 x 480 LVGL interface. The vehicle MCU acquires/validates observations and handles calculations/connectivity; the display MCU renders normalized state received over UART.

The native macOS preview includes Street/Track dashboards, Midnight/Daylight themes, Settings, saved Appearance choices and unified Phone settings. Hardware firmware, UART integration and vehicle verification are pending. A custom moving map with an Android-first Flutter companion app and future iOS support is planned; embedded map feasibility remains pending.

## Run the preview

**Looking for a command?** See the [command reference](docs/commands.md) for display/vehicle flashing, previews, phone connection, tests and asset tools.

To build and flash the latest display desk-test UI over USB: `bash scripts/flash-display.sh`. See the [display instructions](docs/commands.md#flash-the-display-with-the-latest-ui) for port selection and checks.

Prerequisites: C++ compiler, CMake, Ninja and SDL2.

```sh
bash scripts/preview.sh
```

Tab changes layout, D previews themes, arrows adjust speed/RPM, brackets adjust fuel, O simulates ignition and X toggles unavailable data. Press P then S for Settings; arrows cycle focus, Enter selects and Backspace/Esc returns. Esc quits outside Settings.

See the [step-by-step preview guide](docs/getting-started.md), [project documentation](docs/README.md), [architecture](docs/architecture.md) and [implementation plan](docs/implementation-plan.md).

## Verify

```sh
ctest --test-dir build/lvgl --output-on-failure
```

Eleven test suites check portable state/lifecycle/presentation, settings/radio flows, phone bridge IPC and persistence, and render 210 deterministic scenarios, including the three-second boot / two-second shutdown R15 Light Signature timeline. Simulated output is not bike verification.

## Mobile companion

The independent Flutter app lives in [mobile](mobile/README.md). See its [documentation](mobile/docs/README.md) for the agreed journey, architecture and staged implementation. Mobile tooling is not required to build the cluster preview.

For the real Wi-Fi phone-to-Mac development connection, see the [connection guide](mobile/docs/mac-preview-connection.md). Native Android/Mac BLE discovery is implemented; ESP32 provisioning remains pending.
