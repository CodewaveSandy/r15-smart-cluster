# Technical architecture

Last updated: 2026-10-07
Status: desktop app builds and launches. OEM-based layouts, display preferences, and session ride computer exist; full adapter architecture remains planned.

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

Current implementation separates:

```text
app/                  startup composition and bundled fonts
domain/               normalized BikeSnapshot/BikeState; RideComputer
data/                 IBikeDataSource contract
  simulator/          simulator observations and desktop mutation
settings/             validated display preferences and asynchronous storage
application/          observable UI diagnostics
diagnostics/          bounded asynchronous rotating log sink
ui/
  Main.qml            window, navigation, dependency composition
  screens/            dashboard, settings, clock, vehicle info, help
  components/         header, tachometer, fuel, tell-tales, icons, shared text
  theme/              shared palette and warning timing tokens
  input/              desktop keyboard-to-semantic-action adapter
  ErrorScreen.qml     independent startup failure window
tests/                shared core, failure boundaries and rendered UI checks
```

The concrete simulator publishes normalized observations through IBikeDataSource. BikeState validates snapshots, owns frame availability/freshness and exposes unit-qualified read-only properties. It has no simulator invokables. RideComputer integrates only available speed and owns monotonic/session timing; startup supplies the simulator demo odometer baseline explicitly. DisplaySettings validates persisted preferences and uses a worker thread for bounded/coalesced writes. QML formats the simulator's battery/coolant observations rather than embedding samples.

Qt signal groups reduce unrelated updates; one-second simulator heartbeats refresh a monotonic three-second freshness watchdog without repainting identical values. Data-source disconnect/invalid/stale states render explicit unavailable readings. Individual battery/coolant availability exists; full per-signal freshness/support semantics remain pending before real integration.

Main.qml coordinates navigation and small presentation formatting/clock-edit helpers. Its validated action vocabulary is back/select/next/previous/up/down/left/right; directional actions are resolved by screen context, independently of the physical input source. DesktopInput translates keys into semantic screen actions or simulator commands. Screens/components have explicit injected dependencies and never parse hardware data. Future physical inputs can use the same action entry point, but their adapter is not implemented.

cluster_core is the shared CMake target used by application and tests. Startup owns QObject lifetimes; guarded borrowed state avoids source-destruction dereferences. Structured diagnostics use bounded background file writes, stderr fallback and visible status for settings/QML/data failures. See [engineering review](engineering-review.md) for exact error contracts, threading/shutdown behavior and remaining hardware gates.

## Later hardware architecture

The proposed final architecture separates a vehicle MCU from the application processor that handles graphics and projection. Exact MCU/SoM/SoC choices are open. Independent warning indication and failure behavior need explicit design and verification; two processors alone do not provide a complete fallback.

See [product scope](product-scope.md) for unresolved bike and hardware questions and [milestones](milestones.md) for validation gates.
