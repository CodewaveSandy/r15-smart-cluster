# Project documentation

The current implementation is a C++17/LVGL 9.2.2/SDL2 host preview for an 800 x 480 dual-ESP32 Yamaha R15 V4 cluster. Dashboard, Appearance preferences, Bluetooth UI and Wi-Fi UI are implemented; radio behavior and vehicle observations are simulated. Hardware firmware and full Maps/projection feasibility remain pending.

| Guide | Responsibility |
| --- | --- |
| [Preview and development](getting-started.md) | Mac setup, controls, builds, captures and asset regeneration |
| [Architecture](architecture.md) | MCU ownership, source structure, state/concurrency, proposed UART and hardware boundaries |
| [UI style](lvgl-style-guide.md) | Approved layouts, typography, themes, instruments and warnings |
| [Settings and connectivity](settings.md) | Appearance/persistence, navigation, Bluetooth and Wi-Fi behavior/integration gates |
| [Power lifecycle](power-lifecycle.md) | Three-second boot, two-second shutdown, interruptions and retained-power contract |
| [Implementation status and plan](implementation-plan.md) | Current feature/evidence matrix, next stages, OEM inventory and release gates |
| [Resource budgets](resource-budgets.md) | Current host metrics, repeatable audits and required target measurements |

Start with the preview guide to run the project, or the implementation plan to understand what works and what remains. [RULES.md](../RULES.md) is the engineering authority; [AGENTS.md](../AGENTS.md) defines the agent workflow. Keep each guide authoritative for its topic; update it with the implementation rather than appending separate change-history/status documents. Asset-local READMEs, licenses and manifests remain with their source files. Reference images under `reference/` are visual references, not evidence of available vehicle signals or hardware capability.
