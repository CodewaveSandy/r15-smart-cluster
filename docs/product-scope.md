# Product scope

Baseline: 2026-10-18.

Create a premium personal replacement instrument cluster for the Yamaha R15 V4. Two ESP32 MCUs separate vehicle acquisition/calculations/connectivity from LVGL display rendering. The selected display family is the Elecrow 7-inch 800 x 480 HMI. Mechanical compatibility remains unverified.

| Requirement | Scope and qualification |
| --- | --- |
| Instrumentation | Speed, RPM, gear/neutral, fuel, warnings and verified OEM functions; unknown data remains visibly unknown |
| Presentation | Street/Track layouts, Midnight/Daylight themes, Inter/Barlow typography, continuous RPM bar and segmented fuel |
| Controls | Physical navigation; touch may be optional but never required |
| Ride information | Trips and lap information; real odometer provenance/persistence must be established |
| Preferences | Display mode, theme, units and clock settings, with validation and visible save failures |
| Ignition journey | Key-on -> 1–1.5 s boot animation -> instrumentation; key-off -> 1–1.5 s shutdown animation -> power-off, subject to retained power |
| Phone connectivity | Vehicle MCU owns Wi-Fi/BLE; actual services, calls and audio path require feasibility work |
| Full Google Maps / Android Auto / CarPlay | Retained as an unresolved requirement to investigate; no receiver path is established |
| OEM fit and reliability | Mounting, viewing angle, connectors, power and environmental exposure require measurement |

The first deliverable is a reviewed design and reproducible UI/firmware foundation. No vehicle protocol, pinout or calibration is invented. First bike observations remain passive with OEM functionality retained. Commercialization, additional bikes and accessories are outside the immediate implementation.

See [power lifecycle](power-lifecycle.md) for animation/failure semantics and [implementation plan](implementation-plan.md) for validation stages.
