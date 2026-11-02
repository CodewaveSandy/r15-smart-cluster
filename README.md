# R15 Smart Cluster

A dual-ESP32 instrument cluster for the Yamaha R15 V4, with an 800 x 480 LVGL interface. The vehicle MCU acquires/validates observations and handles calculations/connectivity; the display MCU renders normalized state received over UART.

The native macOS preview includes Street/Track dashboards, Midnight/Daylight themes, Settings, saved Appearance choices and unified Phone settings. Native display and vehicle discrete-input bench firmware communicate over UART with return controls, authenticated BLE pairing/reconnect and phone name/battery/time sync; full telemetry/Wi-Fi/map integration and vehicle verification remain pending. A custom moving map with an Android-first Flutter companion app and future iOS support is planned; embedded map feasibility remains pending.

## Run the preview

**Looking for a command?** See the [command reference](docs/development.md#development-wiring-and-commands) for display/vehicle flashing, previews, phone connection, diagnostics and asset tools.

To build and flash the latest display desk-test UI over USB: `bash scripts/flash-display.sh`. See the [display instructions](docs/development.md#flash-the-display-with-the-latest-ui) for port selection and checks.

Prerequisites: C++ compiler, CMake, Ninja and SDL2.

```sh
bash scripts/preview.sh
```

Tab changes layout, D previews themes, arrows adjust speed/RPM, brackets adjust fuel, O simulates ignition and X toggles unavailable data. Press P then S for Settings; arrows cycle focus, Enter selects and Backspace/Esc returns. Esc quits outside Settings.

For the synthetic LVGL map renderer, run `bash scripts/preview.sh --map-preview`. This separate 800 × 480 host preview draws a fictional road network/sample route with follow/overview cameras; it has no real map provider, GPS or board connection. For real central-Kolkata OpenStreetMap roads, run `bash scripts/preview.sh --kolkata-map` after the one-time import. See [map controls and evidence](docs/development.md#lvgl-map-render-preview).

See the [step-by-step preview guide](docs/development.md#host-setup-and-controls), [project documentation](README.md#documentation), [architecture](docs/architecture.md#system-architecture-status-and-roadmap) and [implementation plan](docs/architecture.md#current-status-and-roadmap).

## Development verification

Automated test files were removed at the user's request on 2026-10-31 and are deferred until requested again. Use affected builds, static analysis, runtime diagnostics, manual previews and physical bench checks. See [RULES.md](RULES.md) for the project-wide rule. Simulated output is not bike verification.

## Mobile companion

The independent Flutter app lives in [mobile](mobile/README.md). See its [documentation](mobile/README.md#documentation) for the agreed journey, architecture and staged implementation. Mobile tooling is not required to build the cluster preview.

For the real Wi-Fi phone-to-Mac development connection, see the [connection guide](mobile/docs/development.md#mac-preview-adapter). Native Android/Mac BLE discovery is implemented; ESP32 provisioning remains pending.

The display clock accepts authenticated phone time and Mac USB bench sync: `bash scripts/sync-clock.sh /dev/cu.usbserial-10` (use the current display port). Settings → Clock & Date saves 12/24-hour, date-format and timezone choices. Cold power requires time sync again; no retained RTC is assumed.

Next work: [firmware background sync](docs/background-sync.md) and [mobile background sync](mobile/docs/background-sync.md). Both plans share the same existing BLE protocol and B1 acceptance gate; background ride operation is not implemented yet.

## Documentation

Four ongoing firmware/system guides:

| Guide | Responsibility |
| --- | --- |
| [Architecture, status and roadmap](docs/architecture.md#system-architecture-status-and-roadmap) | Ownership, current evidence, resource policy and next gates |
| [Development, wiring and commands](docs/development.md) | Builds/flash/previews, observed wiring and UART bench diagnostics |
| [UI, Settings and lifecycle](docs/ui.md) | Approved rendering, preferences, warnings and two-second power lifecycle |
| [Background-sync plan](docs/background-sync.md) | Independent firmware F1–F3 work and joint locked-screen acceptance |

The [mobile README](mobile/README.md#documentation) lists its four guides. RULES.md/AGENTS.md, board-specific flashing/recovery READMEs, reference artwork and asset licenses/manifests remain separate authorities. Do not add another completed-work handoff or chronological status document; update the relevant guide.
