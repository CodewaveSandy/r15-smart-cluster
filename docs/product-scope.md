# Product scope

Last updated: 2026-10-06

## Vision

Create an integrated, premium color instrument cluster for the user's Yamaha R15 V4 (2026), replacing the factory monochrome assembly within an OEM-compatible physical envelope. The eventual installation should use matching mounting points and connectors. Build a successful personal unit first; commercialization is a future possibility, not the current development deliverable.

The user is a frontend developer and motorcycle enthusiast. Visual quality, motion, readability, and interaction quality are core product requirements. Embedded electronics are a learning area.

## Final product requirements

| ID | Requirement | Qualification |
| --- | --- | --- |
| R01 | Preserve all existing OEM instrumentation, warnings, and applicable configuration | Inventory and validate against the exact bike; do not assume every example field is available |
| R02 | Sophisticated modern interface with selectable themes | Riding-mode-linked themes apply only to bikes that expose riding modes |
| R03 | Full Google Maps navigation | Essential; a turn arrow or simulated map alone does not satisfy this requirement |
| R04 | Phone connectivity and incoming-call interaction | Exact phone platform, receiver, control integration, and audio route need validation |
| R05 | Physical-control navigation without dependence on touch | Prefer existing OEM controls where feasible; actual switch capabilities are unverified |
| R06 | OEM-style mounting, footprint, and connector integration | Dimensions and connector details remain unmeasured/unverified |
| R07 | Preserve access to essential bike information during navigation | Exact layout depends on display and projection constraints |

Candidate OEM data to inventory: speed, RPM, gear/neutral, fuel, temperature where available, odometer, trips, clock, turn indicators, high beam, ABS/check-engine warnings, shift indication, and existing settings. This list is not a claim about the bike's available signals.

## Current scope: desktop prototype

The user supplied an OEM cluster photo and detailed behavior on 2026-10-06. The [reference inventory](reference/oem-inventory.md) is the detailed source for the gear-first header, Street/Track layouts, QS/VVA/app indicators, trip/lap pages, warnings, and configuration screens.

- Run locally on the user's M1 MacBook Air.
- Develop a Qt/QML dashboard at an initial 800 × 480 target resolution.
- Supply simulated bike data and keyboard inputs before hardware arrives.
- Add ESP32-S3, buttons, and potentiometers through USB serial afterward.
- Keep data acquisition separate from UI rendering.
- Prove one polished theme before expanding the interface.
- Investigate projection feasibility early; a desktop navigation placeholder must be labeled as simulated.

## Deferred or optional

- General phone notifications are optional and may be omitted.
- Music is not a separately committed feature; receiver capabilities can be evaluated later.
- Additional motorcycles, commercial launch, production PCB, and final enclosure follow a proven personal prototype.
- Cameras, TPMS, standalone navigation, SIM/4G, and other accessories are outside the current scope.
- Raspberry Pi and a separate display are not required for the current phase.

## Open questions

1. Which phone/OS must the first working projection implementation support? Are both Android Auto and CarPlay required for the first final unit?
2. Which receiver implementation can coexist with our UI and accept the intended controls?
3. What are the exact usable display dimensions, mounting geometry, and enclosure depth?
4. Which signals and communication protocols does this exact R15 use, and what does its OEM cluster transmit?
5. Which OEM controls exist and how are their events exposed?
6. Which indicators will use independent physical LEDs, and what behavior must remain available if the main UI fails?
7. What are the final processor, display, power, thermal, environmental, and cost requirements?

The approximate 18 × 12 × 3 cm enclosure and 4–5 inch display discussions are planning references, not confirmed mechanical specifications. A commercial release would need its own applicable compliance investigation.
