# R15 Smart Cluster

A dual-ESP32 instrument cluster for the Yamaha R15 V4, with an 800 x 480 LVGL interface. The vehicle MCU acquires/validates observations and handles calculations/connectivity; the display MCU renders normalized state received over UART.

The native macOS preview includes Street/Track dashboards, Midnight/Daylight themes, Settings, saved Appearance choices and simulated Bluetooth/Wi-Fi flows. Hardware firmware, UART integration and vehicle verification are pending. Full Maps/phone projection remains an unresolved requirement.

## Run the preview

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

Nine test suites check portable state/lifecycle/presentation, settings/radio flows and persistence, and render 294 deterministic scenarios, including the three-second boot / two-second shutdown R15 Light Signature timeline. Simulated output is not bike verification.
