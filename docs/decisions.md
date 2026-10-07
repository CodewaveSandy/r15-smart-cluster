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

## D12 — Bundle Bricolage Grotesque

- Date: 2026-10-06
- Status: Accepted
- Use the official Bricolage Grotesque Regular, SemiBold, and Bold across the interface, replacing Helvetica Neue.
- Bundle the unmodified font files and OFL license into the app and load them before QML; no machine-wide installation or runtime network dependency.
- Provenance and file hashes: [font assets](../assets/fonts/bricolage/README.md).

## D13 — Street speed prominence for the UI baseline

- Date: 2026-10-06
- Status: Accepted
- Increase Street speed digits from 91 to 128 px and move the lower-section boundary down by 40 px (y=336 → 376).
- Compress Street's information/fuel strip: smaller primary information value, inline distance unit, compact fuel gauge, and no duplicate information-cycle hint (the footer retains it).
- Track mode keeps its existing dimensions and lap layout. This is a visual baseline, not completion of hardware or data-integration milestones.

## D14 — Keep the original UI; adopt icons and segmented fuel

- Date: 2026-10-07
- Status: Accepted
- The user rejected and reverted the futuristic dashboard redesign. Keep the original layout, Bricolage font, continuous RPM gradient and existing shift warning.
- Adopt only its icon direction and slanted eight-segment fuel gauge with a fuel-pump icon. Draw icons locally in QML Canvas; no external assets or new dependencies. Keep QS/VVA and recognizable TCS/ABS lettering.
- Extract ClusterIcon, TellTale and FuelGauge as focused presentation components. Replace APP text with a phone pictogram; preserve phone battery behavior and existing simulator controls.
- Below 15% fuel, immediately show an amber warning and blink the fuel icon/filled segments every 500 ms continuously while the dashboard is visible. Keep the percentage readable; at zero the pump still blinks, with no filled segments. At 15% or above stop blinking and restore the steady gauge.
- This user-selected warning is presentation behavior for normalized simulator fuel, not a verified Yamaha reserve threshold. Existing reserve-trip calculations are unchanged.
- Verification: configure/build, both CTest targets and offscreen startup passed. UI tests cover 0/14/15/100%, warning phases, recovery, both themes and both modes. Rendered 800×480 states inspected.

## D15 — Inactive icon contrast

- Date: 2026-10-07
- Status: Accepted
- Tell-tales and the phone connection pictogram use 12% opacity when inactive and 100% opacity with their existing status color when active, in both themes. Apply changes immediately without fades. Indicator blink-off phases use the same inactive opacity.
- Fuel level is an always-present gauge rather than an on/off tell-tale; its existing low-fuel blinking behavior remains unchanged.

## D16 — Fuel severity and finer simulator increments

- Date: 2026-10-07
- Status: Accepted; supersedes D14's fuel-warning threshold.
- Fuel gauge is yellow from 10% through 20% inclusive and red below 10%. Blink the icon/filled segments every 500 ms below 20%; exactly 20% remains steady yellow. Percentage remains visible. Above 20% restore the normal gauge.
- Desktop keys change speed by 1 km/h, RPM by 100 and fuel by 1 percentage point in both directions. These increments affect simulator input only, not vehicle data processing or RPM thresholds.
- Reserve-trip calculations retain their existing separate semantics. No Yamaha threshold is claimed.

## D17 — Layer separation and explicit failure handling

- Date: 2026-10-07
- Status: Accepted
- Preserve the approved presentation while extracting focused QML screens/components/theme/desktop input. Keep Main.qml as window/navigation composition with small presentation helpers.
- Introduce a used IBikeDataSource boundary and concrete simulator; BikeState accepts validated unit-qualified observations only. Separate header/.cpp responsibilities and share cluster_core across app/tests.
- Add available/disconnected/invalid/stale frame handling with a monotonic three-second freshness default; valid frames recover. Pause distance integration and mask unavailable measurements rather than substituting zero. Battery/coolant carry individual availability.
- Simulator owns sample measurements and the demo odometer baseline; no vehicle-owned persistence is introduced.
- Persist validated preferences through one background worker with coalesced bounded writes and observable errors. Add bounded rotating asynchronous diagnostics and an independent startup failure screen; runtime QML errors are logged and surfaced.
- Safety claims are limited to tested software behavior. Fatal faults, embedded power/boot/watchdog behavior, full per-signal semantics and independent critical indications remain hardware milestones.
- Rationale, limits, ownership, verification and assembly gates: [engineering review](engineering-review.md).

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
