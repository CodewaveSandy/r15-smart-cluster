# R15 Smart Cluster

A dual-ESP32 instrument cluster for the Yamaha R15 V4, with an 800 x 480 LVGL interface. The vehicle MCU acquires/validates observations and handles calculations/connectivity; the display MCU renders normalized state received over UART.

The native macOS preview is implemented. Hardware firmware, UART integration and vehicle verification are pending. Full Maps/phone projection remains an unresolved requirement.

## Run the preview

Prerequisites: C++ compiler, CMake, Ninja and SDL2. On this Mac these are already installed.

```sh
bash scripts/preview.sh
```

Tab changes layout, D changes theme, arrow keys adjust speed/RPM, brackets adjust fuel, O simulates ignition, X toggles unavailable data, and Esc quits.

See the [step-by-step preview guide](docs/getting-started.md), [project documentation](docs/README.md), [architecture](docs/architecture.md) and [implementation plan](docs/implementation-plan.md).

## Verify

```sh
ctest --test-dir build/lvgl --output-on-failure
```

Tests check portable state/lifecycle/presentation behavior and render 44 deterministic scenarios. Simulated output is not bike verification.
