# R15 Smart Cluster

Project baseline: 2026-10-18. Phase: first native LVGL host preview implemented; hardware firmware pending.

Build a premium instrument cluster for the Yamaha R15 V4 using two ESP32 MCUs, a UART link and an 800 x 480 LVGL display. Preserve essential instrumentation, clear warning visibility, physical-control operation and a deliberate visual hierarchy.

| Document | Purpose |
| --- | --- |
| [Product scope](product-scope.md) | Requirements and exclusions |
| [Architecture](architecture.md) | System overview and responsibility boundaries |
| [Embedded design](embedded-architecture.md) | Modules, concurrency, state, UART and resource budgets |
| [UI style guide](lvgl-style-guide.md) | Geometry, typography, themes and instrument behavior |
| [Power lifecycle](power-lifecycle.md) | Key-on, boot, running, key-off and shutdown journey |
| [Implementation plan](implementation-plan.md) | Stages, acceptance gates and tests |
| [Decisions](decisions.md) | Selected direction and proposals |
| [Milestones](milestones.md) | Delivery sequence |
| [Status](status.md) | Evidence, blockers and next actions |
| [Getting started](getting-started.md) | Build, preview, keyboard controls and UI tuning |
| [Engineering gates](engineering-review.md) | Hardware and release evidence |
| [OEM inventory](reference/oem-inventory.md) | Functional requirements and verification status |

Record unknowns explicitly. Separate design, implementation, bench verification and bike verification. Git operations remain under user control.

## Repository layout

```text
assets/fonts/             Bundled font sources, licenses and LVGL subsets
shared/model/             Portable normalized observations and validation
embedded/application/     Ignition lifecycle
embedded/ui/              Dashboard, themes and instrument presentation
simulator/lvgl/           Native SDL2 preview, LVGL config and host adapter
tests/embedded/           Portable behavior and render verification
tools/fonts/              Pinned font converter manifest/lockfile
scripts/                  Preview and font generation commands
docs/                     Product, architecture, UI, hardware and workflow
```

The root CMake project builds the LVGL host preview. Firmware folders will be introduced with board-specific implementations; they are not empty placeholders. Start with the [Mac preview guide](getting-started.md).
