# Development milestones

Last updated: 2026-10-06

Immediate implementation scope is M0–M3. Begin projection feasibility investigation early in parallel with desktop work; later roadmap phases are not evidence that integrations already work.

| Phase | Deliverable | Acceptance criteria | Status |
| --- | --- | --- | --- |
| M0 | Project reference | Scope, architecture, decision log, roadmap, and status saved with unresolved questions identified | Complete |
| M1 | Mac development setup | Installed tooling inspected; required setup completed; smallest QML app builds and opens at 800 × 480; commands documented | Complete |
| M2 | State and simulator | Shared BikeState, simulated source, semantic inputs, and connection/freshness states drive a basic UI without hardware dependencies | In progress: basic state and mutation checks exist |
| M3 | First polished dashboard | One coherent theme shows core instrumentation; keyboard simulation works; normal, warning, and unavailable-data states visually checked | In progress: Street/Track and Daylight/Midnight rendered and interaction-tested; full missing-data behavior pending |
| M4 | Screens and themes | Trips/info, settings, theme selection, and labeled navigation placeholder work through physical-style navigation; second theme shares the data layer | In progress: settings, info, time/date and themes implemented; Maps placeholder pending |
| M5 | ESP32 bench integration | Verified board and firmware drive buttons/pots over a documented versioned serial protocol; disconnect, malformed data, and reconnect behavior checked | Not started |
| M6 | Real Maps projection | Selected phone projects real Google Maps; essential bike information coexists; input, reconnect, and call/audio behavior assessed with limitations recorded | Not started |
| M7 | Standalone prototype | Selected computer/display run without Mac; startup, performance, readability, power cycling, and reconnect behavior verified | Not started |
| M8 | R15 integration and replacement engineering | Exact OEM function inventory, signals, controls, mounting, and connector validated; required behavior and failure handling demonstrated before replacement | Not started |

## M1 — Next implementation step

1. Inspect existing Qt, compiler, CMake, and build tooling.
2. Select compatible versions based on the actual environment.
3. Install only missing dependencies with any required environment approval.
4. Create a minimal app and build configuration.
5. Launch it and record reproducible build/run instructions and verification results.

## Projection feasibility gate

Investigate during early desktop development, before committing to an SBC or final display. Determine the phone platform, receiver path, rendering/composition constraints, physical inputs, and hardware requirements. M6 may change the layout or hardware plan. If no viable approach is demonstrated, record the blocker and revisit architecture rather than claiming a placeholder fulfills Maps.

## Validation principles

- Match verification to each change: builds and visual inspection for UI, focused behavioral tests for state/protocol logic, hardware checks for physical integrations.
- Keep simulated, bench-verified, and bike-verified results distinguishable.
- A feature is complete only when its stated behavior has been demonstrated, with material limitations recorded.
- Avoid calendar promises until toolchain and hardware unknowns are resolved.

Detailed progress and evidence belong in [status](status.md); changes to direction belong in [decisions](decisions.md).
