# OEM cluster reference and implementation inventory

Source: user-provided photo and explanation, 2026-10-06. The photo appears to illuminate many display segments simultaneously; it is a feature reference, not evidence of simultaneous real operating states.

![User-provided OEM cluster](oem-cluster.png)

## Required structure

1. Gear first, then Street/Track display mode, QS, VVA, app connection, phone battery, and clock.
2. Speed and RPM in the middle. Preserve the continuous green/yellow/red gradient selected earlier.
3. Selectable information on the lower left, fuel gauge on the lower right.
4. Separate configuration and vehicle information screens, navigable without touch.

## Inventory

| Detail | Current prototype behavior | Integration status |
| --- | --- | --- |
| Gear / neutral | Gear 1–6 or N, with separate neutral tell-tale | Keyboard simulated |
| Street / Track | Distinct layouts; Street emphasizes speed, Track enlarges RPM and adds current lap | Display mode only; no ECU mode command |
| QS | Badge visible only when enabled | Q toggles simulated state |
| VVA | Badge visible strictly above 7,500 RPM | User-specified simulator rule; real signal later |
| App / phone battery | Connection badge and separate phone battery graphic; unavailable battery when disconnected | P toggles connection; connected battery is a demo 84%; no Y-Connect integration |
| Clock | Live HH:mm with date; user-editable offset and restore-Mac-time option | Local display preference only |
| RPM | 0–13 scale from reference, continuous fill with existing blends | Scale matches image; not a verified safe engine limit |
| Speed / units | km/h or mph | Internal speed remains km/h |
| ODO | Demo baseline 12,480 km plus simulated distance | Session-only; never a real odometer |
| Trip A / B | Independent session counters, individually resettable | Integrated from simulated speed |
| Trip F | Distance accumulated at demo fuel level ≤15%; clears above threshold | Prototype threshold, not verified OEM reserve behavior |
| Current / latest / fastest lap | Monotonic timing, start/record, stop, reset | Keyboard controlled; no track/GPS detection |
| Fuel / E / F | Percentage, pump icon and segmented gauge; yellow at 10–20%, red below 10%, blinking below 20% | Keyboard simulated |
| BATT | Bike battery voltage, separate from phone battery | 12.8 V sample |
| COOLANT | Temperature and separate warning tell-tale | 84°C/115°C samples |
| AVG / economy / speed | Information pages retained, unavailable values shown | Fuel consumption input and exact OEM unit/menu mapping not yet verified |
| TCS | Warning tell-tale and information page | Warning simulated; actual control status unknown |
| ABS / engine | Separate tell-tales | Keyboard simulated |
| Turn / high beam | Turn arrows at opposite edges; exclusive L/R, synchronized Z hazards | Keyboard simulated |
| Message / call | Indicators gated on simulated app connection | No phone integration |
| Shift indication | On-screen dots removed at user request; background red at 10,500 RPM, alternating red/normal every 300 ms from 11,400 | User-selected prototype thresholds; physical OEM shift light remains outside this UI |

Warnings now use locally drawn pictograms with dim inactive states. OEM-standard symbol validation, physical LED placement, light behavior, and vehicle configuration support remain engineering work. The Yamaha emblem is not functional instrumentation and is not reproduced in the custom interface.

The photograph includes km/L, L/100km, MPG, km/h, mph and mile/km segment labels. The current prototype supports speed/distance unit conversion and an unavailable economy page; all factory consumption-unit combinations and factory menu ordering still require verification. No OEM data capability should be inferred from a static sample.

## Configuration screen

- Street / Track layout.
- Midnight / Daylight theme, independent of layout.
- Metric / imperial speed and distance.
- Date/time editor: choose a field, adjust it, explicitly save or cancel. Dates are validated and month changes clamp the day to that month's length.
- Vehicle information at a glance, with unavailable VIN/ECU fields.
- Trip A, Trip B, and lap-session reset.
- Restore automatic Mac time.

Mode, theme, units, and clock offset persist through validated asynchronous local Qt settings with visible write failures. Trip/lap/demo odometer values remain session-only. Tests use an isolated temporary settings directory.
