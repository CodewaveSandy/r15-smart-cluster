# Decision log

Statuses: **Accepted** means the working project direction; **Proposed** needs investigation or selection; **Superseded** preserves an earlier direction for context.

## D01 — R15 first, personal prototype first

- Date: 2026-10-06
- Status: Accepted
- Target the user's 2026 Yamaha R15 V4. Build a working personal cluster before commercial development or multi-bike support.
- Earlier SP125-first learning plans are superseded for this project. The ZX-10R is not the development target.

## D02 — Start with the Mac

- Date: 2026-10-06
- Status: Accepted
- Use the existing Mac for UI development and simulation. Add the ordered ESP32 bench components afterward.
- Reason: begin immediately and avoid premature hardware expenditure.
- Earlier Pi 5 and separate-display shopping lists are superseded. No SBC or display is currently selected.

## D03 — Qt/QML with a separate C++ state layer

- Date: 2026-10-06
- Status: Accepted as the planned baseline
- Start with Qt 6/QML, C++, CMake, and an 800 × 480 desktop application.
- Keep simulator and hardware data behind a shared normalized state contract.
- Earlier React and ESP32-only/LVGL directions are not the current implementation plan.

## D04 — Full Maps is essential; projection is the candidate approach

- Date: 2026-10-06
- Status: Requirement accepted; implementation proposed
- Preserve full Google Maps as a final requirement. Investigate phone projection rather than assuming we will implement a navigation engine.
- Android Auto/CarPlay receiver availability, platform support, UI composition, controls, and audio are unproven. Validate early before buying hardware around an assumption.

## D05 — Physical controls

- Date: 2026-10-06
- Status: Accepted
- The interface must not depend on touch. Prefer OEM controls when their capabilities are understood.
- Use keyboard actions first, then bench buttons. Final control wiring and projection input support remain open.

## D06 — Separate current work from final engineering

- Date: 2026-10-06
- Status: Accepted
- Begin with simulation and bench hardware. Investigate real-bike signals with the OEM cluster functioning before attempting replacement.
- Final dimensions, enclosure, connectors, independent indicators, and production electronics require their own validation.

## D07 — Git operations belong to the user

- Date: 2026-10-06
- Status: Accepted
- The user handles pulls and pushes. Do not perform these as routine development steps.

## D08 — Central documentation hub

- Date: 2026-10-06
- Status: Accepted
- Maintain scope, architecture, decisions, milestones, and progress together in `docs/`, with README as the entry point.
- Keep status/evidence separate from planned work; update documents when decisions or implementation materially change.

## D09 — Continuous three-color RPM bar

- Date: 2026-10-06
- Status: Accepted visual direction
- Use one continuous RPM bar. Green indicates the normal region, yellow the middle, and red the high region. Blend green to yellow from 3,500–4,500 RPM and yellow to red from 6,500–7,500 RPM.
- Keep colors anchored to the full RPM scale as the fill grows. These are user-selected visual thresholds, not verified engine limits.
- Remove the dashboard's top branding and desktop-simulator badge. Simulation remains identified in the window title and lower status text.

## D10 — Preserve the OEM information hierarchy

- Date: 2026-10-06
- Status: Accepted
- Start with gear, display mode, QS, VVA, app/phone battery, and clock. Place speed/RPM in the middle and selectable information plus fuel below.
- Street and Track select different UI layouts; they do not send riding-mode commands to the motorcycle. Theme remains an independent setting.
- Show QS only when enabled; simulate VVA strictly above 7,500 RPM as specified by the user. Phone battery belongs to the connected phone, not the bike.
- Preserve trips, lap information, warnings, and additional fields from the [reference inventory](reference/oem-inventory.md). Clearly distinguish samples and unavailable values from integrated vehicle data.
- Add physical-style configuration navigation with theme, units, clock/date, and vehicle information.

## D11 — Larger tachometer, background shift alert, and hazards

- Date: 2026-10-06
- Status: Accepted; supersedes D09's upper transition and D10's on-screen shift lights.
- RPM fill height is 44 px in Street and 52 px in Track. Midrange is 4,000–8,000 RPM, with blends at 3,500–4,500 and 7,500–8,500 RPM.
- Remove the five on-screen shift dots. At 10,500 RPM show a steady red background; at 11,400 RPM alternate red/normal every 300 ms. Below 10,500 restore the selected theme. These are user-requested simulator thresholds, not verified engine limits.
- Use readable light foregrounds on the red background in both themes. Active Daylight badges have white text and darker fills; inactive badges retain dark text on their pale fill.
- Place turn arrows at opposite ends of the full-width tell-tale row. L and R are exclusive; Z toggles synchronized hazards. Selecting a direction cancels hazards; disabling hazards clears both signals.

## New decision template

```markdown
## DNN — Title
- Date: YYYY-MM-DD
- Status: Accepted / Proposed / Superseded
- Decision:
- Reason:
- Consequences and unresolved points:
- Supersedes / related decision:
```
