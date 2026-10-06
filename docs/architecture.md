# Technical architecture

Last updated: 2026-10-06
Status: desktop app builds and launches. Basic state and dashboard exist; full adapter/input architecture remains planned.

## Stack

| Layer | Current direction | Status |
| --- | --- | --- |
| Development host | M1 MacBook Air / macOS | User-selected |
| UI | Qt 6 / Qt Quick / QML | Qt 6.11.2 installed and build verified |
| Application and state | C++17 | Basic BikeState implemented |
| Build | CMake + Ninja | CMake 4.4.4 / Ninja 1.13.2 verified |
| Desktop canvas | 800 × 480 | Initial logical target, not final panel specification |
| Bench controller | ESP32-S3 | User reported ordering prototype electronics; exact board to verify |
| Firmware | C++; ESP-IDF or PlatformIO workflow | Tooling choice pending |
| Bench transport | USB serial | Planned; framing/schema pending |
| Standalone host | Embedded Linux-capable computer | Hardware not selected |
| Maps | Phone projection through Android Auto and/or CarPlay | Proposed implementation path requiring a working proof |

## Boundaries

```text
Desktop simulator ─┐
ESP32 serial ──────┼─> validation / normalization ─> BikeState ─> QML UI
Future bike adapter┘

Keyboard / bench buttons / future OEM switches
                       └─> input actions ─> screen navigation

Phone ─> projection receiver ─> navigation surface in application shell
```

- QML components consume normalized application state, never raw serial bytes or bike protocol frames.
- Data sources share a contract so the desktop simulator can be replaced without rebuilding the UI.
- Bike state describes vehicle values; UI state describes focus, screen, and theme. Keep these separate.
- Distinguish unavailable, invalid, and stale values from valid zero values and false warning flags.
- Define units, valid ranges, timestamp/freshness behavior, and gear/neutral semantics before implementing adapters.
- Treat connection state and reconnect behavior as visible application states.
- Theme selection changes presentation; it must not imply a real ECU riding-mode change.
- Settings that affect the real vehicle require verified support. Prototype settings must not imply commands were sent to a bike.
- Projection is a separate integration. A placeholder or phone pairing alone does not prove navigation support.

## Input design

Use semantic actions such as Up, Down, Select, and Back for interface navigation. Keep simulator shortcuts for changing speed/RPM/gear distinct from those actions. Exact key mappings will be documented when implemented.

## Source layout

```text
app/          application startup, state, services
ui/           QML screens, components, themes
simulator/    fake vehicle source and scenarios
firmware/     ESP32 bench firmware, added when needed
docs/         project reference and progress
CMakeLists.txt
```

Create source folders when they are needed rather than scaffolding unused systems.

Current implementation includes `app/`, `ui/`, `scripts/`, and `tests/`. The desktop-only BikeState currently owns simulator mutation methods and QML maps keys directly to them. Dedicated simulator adapters, semantic menu inputs, and per-value freshness/availability are next steps, not completed features.

## Later hardware architecture

The proposed final architecture separates a vehicle MCU from the application processor that handles graphics and projection. Exact MCU/SoM/SoC choices are open. Independent warning indication and failure behavior need explicit design and verification; two processors alone do not provide a complete fallback.

See [product scope](product-scope.md) for unresolved bike and hardware questions and [milestones](milestones.md) for validation gates.
