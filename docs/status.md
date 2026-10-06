# Current status

Last updated: 2026-10-06
Current phase: M1 complete; M2–M4 in progress, with OEM-based dashboard and configuration screens implemented.

## Completed

- Reviewed the shared planning conversation and captured the latest project direction.
- Verified project directory read/write access using a temporary file, then removed it.
- Inspected Git configuration: local `main` had no commits; origin points to `git@github.com:CodewaveSandy/r15-smart-cluster.git`.
- Created the centralized documentation hub and roadmap.
- Installed Qt 6.11.2, CMake 4.4.4, and Ninja 1.13.2 through Homebrew; Apple Clang 17 was already available.
- Built and launched the native 800 × 480 desktop application.
- Added basic C++ BikeState and a first QML dashboard with speed, RPM, gear, fuel, and indicator controls.
- Added `scripts/preview.sh` and the [beginner build/preview guide](getting-started.md).
- Passed the CTest state check covering bounds, indicator toggles, reset, and change notifications.
- Passed offscreen QML startup smoke check. Native app launch and window title confirmed through the UI tool.

## Latest increment — OEM reference

- Saved the user's photo and a functional inventory under `docs/reference/`.
- Added distinct Street/Track layouts, gear-first status row, QS/VVA, app/phone battery, clock/date, warning badges, shift lights, and lower information pages.
- Added persistent Midnight/Daylight themes, units, display mode, and clock offset; keyboard-operated settings, clock editor, and vehicle info.
- Added session trip counters and monotonic lap timing. Unavailable efficiency/average-speed data remains visibly unavailable.
- Offscreen interaction tests and rendered review cover Street, Track, settings, date editor, vehicle info, Daylight, and help. Native visual capture was not needed for this increment.

## Not implemented yet

No dedicated simulator adapter, per-signal freshness/availability contract, firmware, serial adapter, projection receiver, or real vehicle integration yet. Warning badges are textual placeholders for final pictograms; factory menu ordering and all economy unit variants remain unverified. The navigation/Maps screen is still pending.

## Next actions

1. User previews Street/Track and settings and reviews the reference inventory.
2. Complete BikeState availability/freshness and separate simulator/input contracts (M2).
3. Refine final icon artwork and missing-data/warning presentation; investigate real projection feasibility.

## Dependencies and unanswered questions

- User reported ordering the bench electronics in the earlier chat. Delivery status and exact ESP32 board variant have not been verified.
- Phone platform and projection approach need selection before the projection proof.
- Exact bike measurements, connector/protocol, and switch details remain unverified.
- None of those hardware unknowns blocks starting the Mac application.

## Session log

| Date | Work | Verification / result |
| --- | --- | --- |
| 2026-10-06 | Repository access inspection | Read/write probe succeeded; Git remote read locally; remote authentication not tested |
| 2026-10-06 | Documentation baseline | Scope, stack, decisions, phases, and open questions recorded; no application code changes |
| 2026-10-06 | First desktop app | Build and CTest passed; offscreen QML startup passed; native window opened. Detailed native capture/input verification limited by capture failure |
| 2026-10-06 | Dashboard visual revision | Replaced segmented RPM blocks with a continuous bar; green/yellow blend at 3,500–4,500 and yellow/red at 6,500–7,500 RPM. Removed the top branding and desktop-simulator badge |

| 2026-10-06 | OEM-based cockpit and settings | State and offscreen UI interaction tests; rendered screen review; no real vehicle or phone connection |

| 2026-10-06 | RPM and signal refinement | Thicker bar, 4k–8k midrange, full-background shift alert, Daylight badge contrast, and exclusive turn signals with Z hazards |

## Next update template

Record the date, meaningful changes, verification performed and outcome, remaining issues, and next action. Do not mark planned features as completed or imply mocked behavior is a real integration.

