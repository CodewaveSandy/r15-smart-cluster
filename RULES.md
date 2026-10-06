# RULES.md

# R15 Smart Cluster --- Engineering Rules

This document defines the engineering standards for coding agents and
contributors working on the R15 Smart Cluster.

These rules exist to keep the codebase production-oriented,
maintainable, testable, portable, and safe as it evolves from a macOS
simulator into an embedded motorcycle instrument cluster.

Read this file together with `AGENTS.md` and the documentation under
`docs/` before making architectural changes.

------------------------------------------------------------------------

## 1. Engineering Priorities

Optimize in this order:

1.  Correctness and safety
2.  Clear architecture and ownership
3.  Maintainability and testability
4.  Deterministic behavior
5.  UI responsiveness and readability
6.  Portability to embedded Linux
7.  Performance
8.  Development convenience

Do not sacrifice architecture or correctness merely to make a feature
work quickly.

Avoid both extremes:

-   prototype-quality shortcuts that become permanent;
-   unnecessary enterprise abstractions for problems the project does
    not have.

Prefer the simplest design that preserves clean boundaries and future
evolution.

------------------------------------------------------------------------

## 2. Product Context

The current application runs on macOS as an 800×480 Qt/QML simulator.

The intended evolution is:

``` text
Desktop simulator
        |
        v
Normalized vehicle state
        |
        v
Application/domain layer
        |
        v
Qt/QML presentation

Later:

ESP32 / vehicle MCU ----+
                        |
Real Yamaha adapter ----+--> normalization --> same application/UI
```

The eventual product may contain:

-   an embedded Linux application processor;
-   a separate real-time vehicle MCU;
-   a TFT display;
-   physical handlebar controls;
-   independent critical tell-tales;
-   Android Auto / CarPlay projection;
-   real motorcycle interfaces.

Desktop implementation choices must not unnecessarily prevent this
migration.

------------------------------------------------------------------------

## 3. Architectural Boundaries

Maintain these conceptual layers.

``` text
┌─────────────────────────────┐
│ Presentation                │
│ QML screens/components      │
└──────────────┬──────────────┘
               │
┌──────────────▼──────────────┐
│ Application / Domain        │
│ BikeState, navigation,      │
│ ride calculations, settings │
└──────────────┬──────────────┘
               │
┌──────────────▼──────────────┐
│ Data Sources / Adapters     │
│ simulator, serial, Yamaha   │
└──────────────┬──────────────┘
               │
┌──────────────▼──────────────┐
│ Hardware / Transport        │
│ USB, UART, CAN, K-Line etc. │
└─────────────────────────────┘
```

Dependencies should generally point inward/upward toward stable domain
contracts.

### Presentation must not

-   parse serial packets;
-   decode CAN frames;
-   contain Yamaha protocol knowledge;
-   perform hardware discovery;
-   own persistence;
-   perform blocking I/O;
-   contain substantial domain/business logic.

### Domain/application code must not

-   depend on coordinates, colors, fonts, or visual QML details;
-   assume a particular physical transport;
-   assume simulated data is real vehicle data.

### Hardware/data adapters must not

-   decide how information is visually presented;
-   directly manipulate QML objects;
-   expose raw protocol details to UI components.

------------------------------------------------------------------------

## 4. Source Organization

Move toward a structure such as:

``` text
app/
    main.cpp

domain/
    BikeState.*
    RideComputer.*

application/
    AppController.*
    InputController.*
    ScreenController.*

data/
    IBikeDataSource.h
    simulator/
        SimulatorBikeDataSource.*
    serial/
        SerialBikeDataSource.*
        SerialProtocol.*
    yamaha/
        # only after verified protocol work exists

settings/
    DisplaySettings.*

ui/
    Main.qml

    screens/
        DashboardScreen.qml
        SettingsScreen.qml
        ClockScreen.qml
        VehicleInfoScreen.qml
        HelpScreen.qml

    components/
        Speedometer.qml
        Tachometer.qml
        GearIndicator.qml
        FuelGauge.qml
        TellTale.qml
        TellTaleBar.qml
        InfoPanel.qml

    theme/
        Theme.qml
        Typography.qml
        Metrics.qml

tests/
docs/
scripts/
firmware/
```

Do not create empty architecture merely to match this tree. Introduce
directories/classes when responsibility genuinely exists.

One file should have one clear primary responsibility.

------------------------------------------------------------------------

## 5. Main.qml Rule

`Main.qml` is an application shell, not the entire application.

It may coordinate:

-   top-level window;
-   global theme;
-   top-level navigation/screen container;
-   dependency injection into screens;
-   application-wide overlays.

It should not grow into a monolithic implementation containing every
screen and component.

When a visual concept has its own behavior or reusable identity, extract
it.

As a guideline rather than a hard limit, investigate QML files
approaching roughly 250--300 lines for opportunities to split
responsibilities.

Do not split components purely to reduce line count.

------------------------------------------------------------------------

## 6. BikeState Is a Domain Contract

`BikeState` represents normalized motorcycle state.

It must not become:

-   a serial parser;
-   a simulator controller;
-   a Yamaha protocol object;
-   a UI controller;
-   a miscellaneous global state bag.

Prefer explicit domain names with units:

``` cpp
speedKph
engineRpm
coolantTemperatureC
batteryVoltageV
fuelPercent
odometerKm
```

Avoid ambiguous names such as:

``` cpp
value
temp
speedRaw
data
status1
```

unless they exist only inside a narrowly scoped protocol parser.

The UI should not care where `BikeState` came from.

------------------------------------------------------------------------

## 7. Separate State From Commands

Reading vehicle state and issuing simulator actions are different
responsibilities.

Avoid permanently exposing simulator methods such as:

``` cpp
bike.adjustRpm(...)
bike.adjustSpeed(...)
bike.toggleAbs(...)
```

on the production domain state object.

Move toward:

``` text
SimulatorBikeDataSource
        |
        v
BikeState
```

or an equivalent controller/source architecture.

Production `BikeState` should primarily represent observed normalized
state.

------------------------------------------------------------------------

## 8. Data Availability and Freshness

A real vehicle signal has more states than simply a value.

Distinguish:

``` text
valid value
unavailable
not supported
stale
disconnected
invalid/malformed
```

Do not represent "unknown" by silently using `0`.

For example, a disconnected speed source is not necessarily:

``` text
speed = 0
```

because zero means the motorcycle is known to be stationary.

Design adapters so timestamps/freshness can eventually be tracked where
necessary.

The UI must have deliberate presentation behavior for unavailable or
stale information.

------------------------------------------------------------------------

## 9. Units

Internally use one canonical unit per measurement.

Recommended examples:

``` text
speed          km/h
distance       km
temperature    °C
voltage        V
time duration  milliseconds or std::chrono types
fuel           documented normalized percentage/range
```

Convert units at presentation boundaries.

Do not store a value in mph simply because the user selected imperial
display.

Names must make units obvious when confusion is possible.

------------------------------------------------------------------------

## 10. C++ Standard

Use modern, idiomatic C++ appropriate to the project's configured
standard.

The repository currently uses C++17. Do not silently change the language
standard.

Prefer:

-   RAII;
-   value semantics where appropriate;
-   const correctness;
-   scoped enums;
-   explicit ownership;
-   smart pointers where dynamic ownership is required;
-   references/pointers whose ownership semantics are obvious;
-   small cohesive classes;
-   standard library facilities before custom utilities.

Avoid:

-   owning raw pointers without explicit lifetime guarantees;
-   manual `new` / `delete` where RAII can be used;
-   hidden mutable globals;
-   macros for ordinary constants/functions;
-   unnecessary inheritance;
-   clever template machinery without a concrete benefit.

------------------------------------------------------------------------

## 11. QObject Ownership

Qt object lifetime must be intentional.

For every `QObject`, it should be obvious whether lifetime is controlled
by:

-   stack lifetime;
-   QObject parent ownership;
-   smart pointer;
-   QML engine ownership.

Never create ambiguous mixed ownership.

Do not manually delete an object owned by a QObject parent.

------------------------------------------------------------------------

## 12. Signals and Properties

Use Qt properties for observable state and signals for meaningful state
changes/events.

Prefer specific notifications when state complexity grows.

A universal `changed()` signal is acceptable during early development
but should not become the permanent mechanism for a large domain model
if it causes unnecessary recomputation or obscures what changed.

Setters should avoid emitting notifications when the effective value did
not change unless repeated notification is intentionally part of the
contract.

Example:

``` cpp
void setSpeedKph(int value)
{
    value = std::clamp(value, 0, kMaximumValue);

    if (m_speedKph == value)
        return;

    m_speedKph = value;
    emit speedKphChanged();
}
```

------------------------------------------------------------------------

## 13. QML Responsibilities

QML should primarily contain:

-   layout;
-   visual state;
-   animation;
-   formatting close to presentation;
-   focus/navigation presentation;
-   component composition.

Move substantial calculations, persistence, protocol behavior, and
domain rules into C++.

Small formatting helpers may remain in QML when they are genuinely
presentation-specific.

Do not build a second business-logic layer in JavaScript.

------------------------------------------------------------------------

## 14. QML Component Design

Prefer focused components with explicit inputs.

Good:

``` qml
Tachometer {
    rpm: bike.engineRpm
    maximumRpm: 13000
}
```

Avoid components that reach into unrelated global objects for everything
they need.

Components should expose properties representing their contract.

Reusable visual components should not secretly mutate application state.

------------------------------------------------------------------------

## 15. Theme and Design Tokens

Do not scatter arbitrary visual constants throughout QML.

Centralize reusable tokens for:

-   palette;
-   typography;
-   spacing;
-   radii;
-   line thickness;
-   standard component dimensions;
-   animation durations;
-   warning colors;
-   opacity levels.

Themes should change presentation, not domain behavior.

Avoid magic values whose purpose cannot be understood.

For highly intentional one-off layout coordinates, document why they
exist if the reason is not obvious.

------------------------------------------------------------------------

## 16. Fixed 800×480 Does Not Mean Careless Layout

The current design target is intentionally 800×480.

Pixel-specific HMI design is acceptable where it produces deliberate
instrumentation layout.

However:

-   avoid accidental overlaps;
-   maintain clear alignment systems;
-   group related coordinates;
-   prefer components/layout containers when appropriate;
-   ensure changes do not break Street/Track or Daylight/Midnight
    variants.

Do not prematurely build a generic responsive web-style layout if the
actual product has a fixed panel.

------------------------------------------------------------------------

## 17. HMI Readability

This is an instrument cluster, not a general desktop application.

Information priority should reflect riding use.

Critical information must be readable at a glance.

Avoid:

-   excessive decorative text;
-   low contrast;
-   long animations;
-   tiny critical labels;
-   visual noise;
-   gratuitous glassmorphism;
-   animations that obscure current state.

Speed, gear, warnings, and important tell-tales take precedence over
decorative elements.

------------------------------------------------------------------------

## 18. Animation

Animation must support comprehension.

Animate presentation, not truth.

A tachometer may smoothly interpolate toward the latest RPM, but a
critical warning state should not wait for a decorative transition
before becoming visible.

Animations must:

-   remain bounded;
-   avoid queue buildup;
-   avoid blocking state updates;
-   have predictable duration;
-   perform acceptably on future embedded hardware.

Do not assume desktop GPU performance will exist in production.

------------------------------------------------------------------------

## 19. Non-Touch Interaction Is Mandatory

Production UX must not depend on touch.

All user-facing functionality must ultimately be operable through a
small physical action vocabulary such as:

``` text
Up
Down
Select
Back
```

Additional OEM buttons may later expand this.

Do not require:

-   swipe;
-   drag;
-   pinch;
-   hover;
-   mouse precision;
-   touch keyboard;
-   long free-form text entry.

Desktop keyboard shortcuts are simulator conveniences, not the product
interaction model.

------------------------------------------------------------------------

## 20. Semantic Input Actions

Do not spread physical key/button knowledge across screens.

Move toward semantic actions:

``` text
NavigateUp
NavigateDown
NavigateLeft
NavigateRight
Select
Back
NextPage
PreviousPage
```

Hardware and keyboard mappings should translate into these actions.

Screens should respond to semantic actions rather than knowing which
keyboard key or GPIO generated them.

------------------------------------------------------------------------

## 21. Simulator Isolation

Simulation is a first-class development tool but not production logic.

Simulator-only behavior must be identifiable and replaceable.

Never make production behavior depend on:

-   keyboard presence;
-   desktop timers used solely for fake data;
-   sample battery values;
-   fake coolant values;
-   hard-coded demo odometer values.

Mark demo values clearly.

------------------------------------------------------------------------

## 22. Hardware Abstraction

All hardware-specific behavior must sit behind an interface or narrowly
defined adapter.

Examples:

``` text
IBikeDataSource
IInputSource
IPhoneProjectionService
IPersistentStorage
```

Do not create interfaces speculatively for every imaginable subsystem.

Introduce one when there are multiple implementations, a clear hardware
boundary, or testing benefits justify it.

------------------------------------------------------------------------

## 23. Serial Protocol Rules

When ESP32 integration begins, define a documented protocol rather than
ad-hoc text parsing scattered through the app.

The protocol must define:

-   version;
-   framing;
-   message type;
-   field types;
-   units;
-   range constraints;
-   malformed-frame behavior;
-   compatibility policy;
-   heartbeat/connection semantics if required.

The desktop application must validate every external value before
applying it to domain state.

Never trust serial input merely because the sender is our ESP32.

Parsing must not happen inside QML.

------------------------------------------------------------------------

## 24. Vehicle Protocol Rules

Never invent Yamaha protocol information.

Do not guess:

-   connector pinouts;
-   CAN IDs;
-   K-Line messages;
-   bit meanings;
-   resistor ladder values;
-   ECU commands;
-   ABS commands;
-   cluster handshakes.

Unverified discoveries must be explicitly labeled as hypotheses.

Verified findings should be documented with:

-   test conditions;
-   observations;
-   captures where appropriate;
-   confidence/status;
-   bike/model/year context.

Initial real-bike communication must remain passive/read-only unless a
separate reviewed test explicitly authorizes transmission.

------------------------------------------------------------------------

## 25. Safety-Critical Separation

The rich Linux/QML UI must not be treated as the sole safety layer.

Final architecture may require the vehicle MCU to maintain critical
functions independently.

Do not design critical tell-tales so that a QML crash necessarily
removes every warning indication.

Examples requiring explicit safety consideration include:

-   turn indication;
-   neutral;
-   high beam;
-   ABS warning;
-   engine warning;
-   power/ignition behavior.

Software prototypes must not falsely claim safety guarantees that have
not been engineered and tested.

------------------------------------------------------------------------

## 26. Threading

Never block the UI thread with:

-   serial reads;
-   hardware waits;
-   network requests;
-   long parsing;
-   filesystem operations of unpredictable duration;
-   sleeps.

Use asynchronous/event-driven Qt facilities or worker threads when
needed.

Do not add threads casually. Prefer Qt's event-driven APIs when they
solve the problem cleanly.

Shared mutable state across threads requires explicit synchronization
and ownership.

------------------------------------------------------------------------

## 27. Timing

Use monotonic clocks for elapsed durations.

Do not calculate lap times or protocol timeouts using wall-clock time.

Wall-clock time is appropriate for date/time display.

Elapsed timing should use mechanisms such as `QElapsedTimer` or
`std::chrono::steady_clock`.

------------------------------------------------------------------------

## 28. Persistence

Separate:

``` text
vehicle-owned persistent data
application preferences
session simulation data
```

Never overwrite a real odometer from desktop/demo state.

User display preferences may use application persistence.

Vehicle-owned values require a deliberately designed persistence/vehicle
integration strategy.

Persist only what needs persistence.

Version persisted schemas when incompatibility becomes possible.

------------------------------------------------------------------------

## 29. Error Handling

Failures at system boundaries must be explicit.

Examples:

-   serial disconnected;
-   malformed packet;
-   unsupported protocol version;
-   projection unavailable;
-   settings write failure;
-   missing resource;
-   stale vehicle data.

Do not silently replace errors with plausible-looking fake values.

Prefer a safe unavailable state plus diagnostic logging.

------------------------------------------------------------------------

## 30. Logging

Use structured, meaningful Qt logging.

Introduce logging categories as subsystems grow, for example:

``` text
cluster.app
cluster.simulator
cluster.serial
cluster.vehicle
cluster.ui
cluster.projection
```

Avoid noisy per-frame logs.

Never log sensitive phone/user data unnecessarily.

Logs should help diagnose state transitions and failures.

------------------------------------------------------------------------

## 31. Assertions vs Runtime Validation

Assertions are for programmer invariants.

External input must always receive runtime validation.

Never rely on `assert` to validate:

-   serial data;
-   vehicle frames;
-   user-controlled settings;
-   persisted data;
-   phone/projection input.

Release builds may remove assertions.

------------------------------------------------------------------------

## 32. Testing Strategy

Every nontrivial domain behavior should be testable without rendering
the full UI.

Maintain layers of testing:

``` text
Unit tests
    domain calculations
    validation
    parsers
    state transitions

Integration tests
    data source -> BikeState
    persistence
    reconnect behavior

QML/UI tests
    screen navigation
    key/semantic input
    visibility
    important visual states

Hardware tests
    ESP32 protocol
    real electrical behavior

Bike validation
    actual R15 behavior
```

Do not call a simulated test "vehicle verified."

------------------------------------------------------------------------

## 33. Tests Must Cover Boundaries

For numeric values test at least:

``` text
below minimum
minimum
normal value
maximum
above maximum
```

For connection-oriented data test:

``` text
not connected
connect
valid data
malformed data
stale data
disconnect
reconnect
```

For warning logic test simultaneous warnings where realistic.

------------------------------------------------------------------------

## 34. Deterministic Tests

Tests must not depend unnecessarily on:

-   internet access;
-   the developer's current settings;
-   random timing;
-   arbitrary long sleeps;
-   actual wall-clock date;
-   installed system fonts;
-   existing user preference files.

Use temporary settings/storage and bundled resources.

Keep timing tolerances realistic.

------------------------------------------------------------------------

## 35. Visual Regression Discipline

When changing significant UI:

1.  render representative states;
2.  inspect both themes;
3.  inspect Street and Track where relevant;
4.  inspect normal and warning states;
5.  inspect low-fuel/unavailable states where relevant;
6.  verify 800×480 bounds;
7.  ensure critical content remains legible.

Do not accept a UI change solely because QML compiles.

------------------------------------------------------------------------

## 36. Build Hygiene

The project must build from a clean checkout using documented commands.

Generated output must not be committed unless intentionally required.

Keep out of Git:

``` text
build/
temporary captures
generated source exports
IDE caches
OS metadata
```

Do not make the build depend on absolute developer-machine paths.

Scripts should resolve paths relative to the repository whenever
practical.

------------------------------------------------------------------------

## 37. CMake Rules

Keep CMake target-oriented.

Prefer:

``` cmake
target_sources(...)
target_link_libraries(...)
target_include_directories(...)
target_compile_definitions(...)
```

over global flags and directory-wide side effects.

Dependencies must be explicit.

Do not glob source files when explicit source lists provide better build
clarity, unless there is a concrete reason.

Keep platform-specific configuration isolated.

------------------------------------------------------------------------

## 38. Dependency Policy

Every new dependency has long-term embedded consequences.

Before adding one, consider:

-   what problem it solves;
-   binary size;
-   runtime cost;
-   Linux/ARM support;
-   licensing;
-   maintenance activity;
-   security implications;
-   whether Qt or the standard library already solves the problem.

Do not add a library for a trivial utility function.

------------------------------------------------------------------------

## 39. Resource Policy

Runtime-critical UI assets should normally be packaged with the
application.

Do not depend on:

-   internet-hosted fonts;
-   random CDN assets;
-   developer-installed fonts;
-   absolute local file paths.

Track asset provenance and licenses.

Optimize large assets before embedding them.

------------------------------------------------------------------------

## 40. Security

Treat all external communication as untrusted input, including:

-   ESP32 messages;
-   future vehicle adapters;
-   phone integrations;
-   network/projection services;
-   imported configuration.

Validate lengths, ranges, message types, and versions.

Never embed credentials, signing keys, private tokens, or personal
secrets in source control.

------------------------------------------------------------------------

## 41. Performance

Performance work should be evidence-driven.

For the UI:

-   avoid unnecessary bindings that reevaluate continuously;
-   avoid creating/destroying large object trees repeatedly;
-   avoid expensive JavaScript in animation paths;
-   avoid blocking operations;
-   avoid unnecessary image scaling at runtime.

For data processing:

-   bound buffers;
-   avoid unbounded queues;
-   avoid unnecessary allocations in high-frequency paths;
-   process only at the rate needed by the HMI.

Do not optimize away clarity without measurements.

------------------------------------------------------------------------

## 42. Startup and Shutdown

The eventual cluster will be ignition-driven.

Architect with future requirements in mind:

-   predictable startup;
-   known state before first vehicle data;
-   clean data-source initialization;
-   graceful disconnect;
-   persistence flushing;
-   watchdog/recovery strategy later.

Do not assume a desktop-style indefinite startup time is acceptable in
production.

No specific boot-time target should be invented until measured
requirements are established.

------------------------------------------------------------------------

## 43. No Fake Completeness

Use terminology precisely:

``` text
simulated
implemented
bench verified
hardware verified
bike verified
production ready
```

These are not interchangeable.

A QML Maps placeholder does not satisfy Google Maps.

A simulated ABS warning does not prove ABS integration.

A serial mock does not prove ESP32 communication.

A bench CAN frame does not prove safe R15 behavior.

Documentation must preserve these distinctions.

------------------------------------------------------------------------

## 44. Documentation Is Part of the Change

Update documentation when a change affects:

-   architecture;
-   product behavior;
-   development workflow;
-   accepted decisions;
-   milestone status;
-   hardware assumptions;
-   keyboard/input mapping;
-   protocol definitions.

Use the existing documentation structure:

``` text
docs/product-scope.md
docs/architecture.md
docs/decisions.md
docs/milestones.md
docs/status.md
docs/getting-started.md
```

Do not duplicate the same evolving truth across many documents
unnecessarily.

------------------------------------------------------------------------

## 45. Decision Records

Consequential decisions belong in `docs/decisions.md`.

Record:

-   date;
-   status;
-   decision;
-   reason;
-   consequences;
-   unresolved questions;
-   superseded decision where applicable.

Do not silently erase old decisions. Mark them superseded when direction
changes.

------------------------------------------------------------------------

## 46. Comments

Comments should explain why, constraints, safety implications, or
non-obvious behavior.

Bad:

``` cpp
// Increment speed
speed += 5;
```

Useful:

``` cpp
// Simulator bounds are intentionally not treated as verified R15 limits.
```

Do not use comments to compensate for confusing names or architecture.

------------------------------------------------------------------------

## 47. Naming

Names should communicate domain intent.

Prefer:

``` text
BikeState
RideComputer
SerialBikeDataSource
coolantTemperatureC
phoneConnected
```

Avoid:

``` text
Manager
Helper
Utils
Thing
DataProcessor
Common
Misc
```

unless the name truly describes a cohesive concept.

Boolean names should normally read naturally:

``` text
isConnected
hasWarning
phoneConnected
navigationActive
```

Follow the established Qt/project naming convention consistently.

------------------------------------------------------------------------

## 48. Functions

Functions should do one coherent thing.

Prefer early returns to deep nesting where appropriate.

Avoid functions with many boolean parameters.

Instead of:

``` cpp
render(true, false, true, false);
```

use a typed options object or meaningful API.

Keep side effects obvious.

------------------------------------------------------------------------

## 49. Constants

Named constants should replace domain-significant magic values.

Prototype values must indicate when they are not verified vehicle
specifications.

For example:

``` cpp
constexpr int kSimulatorMaximumRpm = 13000;
```

is preferable to unexplained repeated `13000`.

UI-only coordinates may remain local when their meaning is purely visual
and deliberate.

------------------------------------------------------------------------

## 50. Enums Over Magic Integers

Use scoped enums for conceptual states.

Prefer:

``` cpp
enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Stale,
    Error
};
```

over:

``` cpp
int connectionState = 3;
```

Gear may use an integer where arithmetic/display behavior makes it
reasonable, but neutral semantics must remain explicit.

------------------------------------------------------------------------

## 51. State Machines

For systems with meaningful lifecycle behavior---serial connections,
projection, ignition/power, pairing---prefer explicit states and
transitions rather than collections of loosely related booleans.

Example:

``` text
Disconnected
Connecting
Connected
Stale
Recovering
Error
```

Do not introduce a state-machine framework unless complexity actually
requires it.

------------------------------------------------------------------------

## 52. Feature Flags and Temporary Code

Temporary prototype behavior must be clearly identified.

Do not leave abandoned code commented out.

Use version control instead.

TODOs should describe a concrete unresolved task or missing
verification.

Bad:

``` cpp
// TODO fix
```

Good:

``` cpp
// TODO(vehicle): Replace simulator coolant thresholds after R15 signal semantics are verified.
```

------------------------------------------------------------------------

## 53. Git Discipline for Agents

The user owns routine Git pull/push operations.

Coding agents must not perform destructive Git operations unless
explicitly requested.

Do not:

-   force push;
-   rewrite history;
-   discard unrelated local changes;
-   reset user work;
-   commit generated build artifacts.

Before modifying a file, respect existing uncommitted work.

Keep changes scoped to the requested task.

------------------------------------------------------------------------

## 54. Refactoring Rule

Refactoring must preserve observable behavior unless behavior change is
explicitly part of the task.

For meaningful refactors:

1.  understand existing tests;
2.  add missing characterization tests when needed;
3.  refactor incrementally;
4.  build;
5.  run tests;
6.  visually verify UI when affected.

Do not combine a large architecture rewrite and unrelated visual
redesign in one change.

------------------------------------------------------------------------

## 55. Agent Change Procedure

For every substantial coding task, the agent should:

1.  Read `RULES.md`.
2.  Read relevant sections of `docs/`.
3.  Inspect existing implementation before proposing replacements.
4.  State assumptions internally and avoid inventing hardware facts.
5.  Make the smallest coherent production-quality change.
6.  Add/update tests.
7.  Build the affected targets.
8.  Run relevant tests.
9.  Perform UI rendering/visual checks if presentation changed.
10. Update documentation when behavior/architecture changed.
11. Report files changed, verification performed, and remaining
    limitations.

Never claim tests passed if they were not executed.

------------------------------------------------------------------------

## 56. Definition of Done

A feature is not done merely because code exists.

For software-only work, done generally means:

-   implementation follows architecture boundaries;
-   project builds;
-   relevant automated tests pass;
-   important edge cases are covered;
-   no known QML/runtime warnings were introduced;
-   UI was visually checked when applicable;
-   keyboard/physical-style navigation still works;
-   documentation reflects meaningful behavior changes;
-   simulated behavior remains labeled simulated.

Hardware-related features require additional evidence appropriate to
their milestone.

------------------------------------------------------------------------

## 57. Code Review Checklist

Before considering a change complete, ask:

### Architecture

-   Is this logic in the correct layer?
-   Did presentation acquire hardware/domain knowledge?
-   Did a domain object become a miscellaneous state bag?
-   Is a new abstraction justified?

### C++

-   Is ownership clear?
-   Are units explicit?
-   Are ranges validated?
-   Are signals emitted correctly?
-   Is blocking work kept off the UI thread?

### QML

-   Is the component focused?
-   Are theme tokens reused?
-   Is navigation possible without touch?
-   Is important information readable at 800×480?
-   Does it work in relevant themes/modes?

### Data

-   Is unavailable different from zero?
-   Is stale/disconnected behavior defined?
-   Are external values validated?
-   Are simulated values clearly marked?

### Tests

-   Is behavior testable below the UI?
-   Are boundaries covered?
-   Were tests actually run?
-   Was UI visually checked when changed?

### Product integrity

-   Are we claiming anything about the R15 that has not been verified?
-   Did we accidentally imply a simulated integration is real?
-   Could failure hide a critical indication?

------------------------------------------------------------------------

## 58. Current Technical Debt to Address Deliberately

The existing project is a strong prototype foundation, but agents should
gradually address these known structural issues rather than expanding
them indefinitely:

1.  `ui/Main.qml` currently owns too many screens and responsibilities.
2.  `BikeState` currently contains simulator mutation methods.
3.  A dedicated `IBikeDataSource`/simulator data-source boundary is not
    yet implemented.
4.  Availability/freshness semantics for real signals are not yet
    modeled.
5.  Keyboard mappings and semantic product navigation are still coupled
    in the QML shell.
6.  Several battery/coolant/odometer values are intentionally simulated
    samples.
7.  Final warning pictograms and independent physical tell-tale behavior
    remain unresolved.
8.  Serial/ESP32 protocol does not yet exist.
9.  Projection feasibility is not yet proven.
10. Yamaha/R15 communication details remain unverified.

Address these incrementally with tests. Do not attempt to "fix
everything" in a single rewrite.

------------------------------------------------------------------------

## 59. Immediate Architecture Direction

Before substantial ESP32 or Maps integration, prioritize:

``` text
1. Split monolithic QML into screens/components/theme
              ↓
2. Separate simulator commands from BikeState
              ↓
3. Introduce a clear data-source contract
              ↓
4. Model connection + availability/freshness
              ↓
5. Define semantic input actions
              ↓
6. Define versioned ESP32 serial protocol
              ↓
7. Integrate real bench hardware
```

Each step should leave the application buildable and testable.

------------------------------------------------------------------------

## 60. Final Principle

Write code as though another experienced embedded/Qt engineer will
inherit the repository tomorrow.

They should be able to determine:

-   where a feature belongs;
-   who owns each object;
-   where data originates;
-   whether a value is simulated or real;
-   which units are used;
-   how failures are represented;
-   how the behavior is tested;
-   which hardware facts are verified;
-   what remains uncertain.

If understanding a subsystem requires tribal knowledge from the original
author, improve the design or documentation.

The goal is not to make the repository look complicated.

The goal is to make it look deliberate.
