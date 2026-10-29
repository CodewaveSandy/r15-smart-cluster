# Project documentation

The current implementation is a C++17/LVGL 9.2.2/SDL2 host preview for an 800 x 480 dual-ESP32 Yamaha R15 V4 cluster. Dashboard, Appearance preferences and unified Phone UI are implemented. The opt-in Mac bridge supports native BLE discovery and TLS Wi-Fi sessions; vehicle observations remain simulated. The native ESP-IDF display bench target has been user-flashed with initial artwork/layout/theme/touch observations; current firmware changes still require panel verification. Vehicle discrete-input USB bench firmware is implemented; one-way discrete-input UART bench software is implemented; physical link testing, full UART protocol and ESP32 radio services remain pending. Embedded moving-map feasibility remains pending. An Android-first Flutter companion foundation is documented separately, with future iOS support.

| Guide | Responsibility |
| --- | --- |
| [Command reference](commands.md) | Copyable commands and descriptions for flashing, previews, phone connection, builds, tests and asset tools |
| [Preview and development](getting-started.md) | Mac setup, controls, builds, captures and asset regeneration |
| [Display firmware](../firmware/display/README.md) | V3.0 board provenance, native ESP-IDF desk-test build, user-operated flash and recovery |
| [Hardware connections](hardware-connections.md) | Confirmed power path, engine-off bike power-on evidence and missing wiring details |
| [Vehicle USB input test](../firmware/vehicle/README.md) | Configurable N–6, turn/high-beam inputs, separate ESP32 firmware and Mac serial preview |
| [Architecture](architecture.md) | MCU ownership, source structure, state/concurrency, proposed UART and hardware boundaries |
| [UI style](lvgl-style-guide.md) | Approved layouts, typography, themes, instruments and warnings |
| [Settings and connectivity](settings.md) | Appearance/persistence, navigation, unified Phone behavior and integration gates |
| [Power lifecycle](power-lifecycle.md) | Two-second boot, two-second shutdown, interruptions and retained-power contract |
| [Implementation status and plan](implementation-plan.md) | Current feature/evidence matrix, next stages, OEM inventory and release gates |
| [Companion app](../mobile/docs/README.md) | Flutter architecture, phone journey, protocol/security plan and implementation stages |
| [Resource budgets](resource-budgets.md) | Current host metrics, repeatable audits and required target measurements |

Start with the preview guide to run the project, or the implementation plan to understand what works and what remains. [RULES.md](../RULES.md) is the engineering authority; [AGENTS.md](../AGENTS.md) defines the agent workflow. Keep each guide authoritative for its topic; update it with the implementation rather than appending separate change-history/status documents. Asset-local READMEs, licenses and manifests remain with their source files. Reference images under `reference/` are visual references, not evidence of available vehicle signals or hardware capability.
