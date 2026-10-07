# AGENTS.md

# R15 Smart Cluster --- Coding Agent Instructions

This file defines how coding agents must work inside the R15 Smart
Cluster repository.

The project is evolving from a macOS software simulator into a real
embedded motorcycle instrument cluster. Changes must therefore be made
with production-quality architecture, maintainability, portability,
testability, and hardware safety in mind.

## 1. Mandatory Reading Order

Before making a non-trivial change, read:

1.  `AGENTS.md` --- agent workflow and project boundaries.
2.  `RULES.md` --- authoritative engineering standards.
3.  Relevant files under `docs/`.
4.  The existing implementation and tests related to the requested task.

`RULES.md` is the primary engineering authority for this repository.

If existing code conflicts with `RULES.md`, do not perform an unrelated
repository-wide rewrite. Follow the rules for new work and improve
existing technical debt incrementally when it is relevant to the task.

If a user instruction intentionally conflicts with an existing
architectural convention, call out the conflict and implement the user's
requested direction unless doing so would create an unsafe vehicle
behavior or require inventing unverified hardware facts.

------------------------------------------------------------------------

## 2. Project Mission

Build a modern replacement instrument cluster for the Yamaha R15 V4.

The long-term product direction includes:

-   OEM-fit replacement form factor;
-   modern TFT interface;
-   all required motorcycle instrumentation;
-   themes and riding-oriented layouts;
-   non-touch operation;
-   navigation through physical/OEM handlebar controls;
-   phone connectivity;
-   Android Auto / Apple CarPlay projection;
-   Google Maps through phone projection;
-   calls;
-   dedicated critical tell-tales where appropriate;
-   separate vehicle-side MCU and application processor;
-   eventual real Yamaha R15 integration.

The current development environment is an Apple Silicon MacBook Air.

Current development must not wait for motorcycle hardware.

------------------------------------------------------------------------

## 3. Current Technology Stack

Current application stack:

``` text
Qt 6
QML / Qt Quick
C++17
CMake
CTest / Qt testing
macOS development host
800×480 logical cluster viewport
```

Future development is expected to include:

``` text
ESP32-S3 development vehicle controller
USB serial communication
Embedded Linux application processor
Raw/TFT display hardware
Real Yamaha vehicle adapter
Android Auto / CarPlay projection
```

Do not silently replace the current stack.

Do not change the C++ language standard unless explicitly justified and
approved.

------------------------------------------------------------------------

## 4. Current Development Phase

The current application is primarily a software simulator.

The Mac acts as the temporary cluster computer and display.

Current conceptual flow:

``` text
Simulator
   |
   v
Normalized BikeState
   |
   v
Application/domain logic
   |
   v
Qt/QML UI
```

Expected evolution:

``` text
Simulator -----------+
                     |
ESP32 serial --------+--> normalized state --> application --> UI
                     |
R15 adapter ---------+
```

The UI must not care which source produced the vehicle data.

------------------------------------------------------------------------

## 5. Architecture Direction

Maintain clear separation between:

``` text
Presentation
Application/domain
Data sources/adapters
Transport/hardware
```

Target conceptual architecture:

``` text
┌───────────────────────────────┐
│ QML Presentation              │
│ screens, components, themes   │
└───────────────┬───────────────┘
                │
┌───────────────▼───────────────┐
│ Application / Domain          │
│ BikeState, ride logic, input  │
└───────────────┬───────────────┘
                │
┌───────────────▼───────────────┐
│ Data Sources                  │
│ simulator / serial / Yamaha   │
└───────────────┬───────────────┘
                │
┌───────────────▼───────────────┐
│ Hardware / Transport          │
│ USB / UART / CAN / K-Line     │
└───────────────────────────────┘
```

Do not bypass these boundaries for convenience.

------------------------------------------------------------------------

## 6. Repository Direction

The repository may evolve toward:

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
        # only verified Yamaha-specific implementation

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
        Tachometer.qml
        Speedometer.qml
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

This is architectural direction, not permission to create empty
abstractions.

Only introduce files/classes when they have a real responsibility.

------------------------------------------------------------------------

## 7. Known Current Technical Debt

Agents must understand the current prototype before extending it.

Known areas that should be improved incrementally:

Current factual status updated 2026-10-07; see `docs/engineering-review.md`.

1.  Screens/components/theme are extracted. Main.qml still owns small
    navigation and clock-draft presentation helpers.
2.  Simulator mutation is isolated in SimulatorBikeDataSource; verified
    hardware data sources remain unimplemented.
3.  IBikeDataSource and normalized snapshots exist. Full per-signal
    support/freshness and real transport contracts remain incomplete.
4.  Frame availability/freshness and optional battery/coolant availability
    exist; complete real-signal semantics still need design.
5.  DesktopInput isolates keyboard mappings and semantic actions;
    physical/OEM input adapters remain unimplemented.
6.  Several displayed vehicle values are intentionally simulated.
7.  The ESP32 serial protocol is not yet defined.
8.  Android Auto / CarPlay projection has not yet been proven in the
    target embedded environment.
9.  Yamaha R15 communication details remain unverified.
10. Production hardware architecture is not finalized.

Do not attempt to fix all of these in one task.

Refactor incrementally while keeping the project runnable.

------------------------------------------------------------------------

## 8. Immediate Architecture Priorities

Unless the user explicitly asks for a different feature, structural work
should generally progress in this order:

``` text
Split monolithic QML
        ↓
Separate simulator behavior from BikeState
        ↓
Introduce clean data-source contract
        ↓
Model connection / availability / freshness
        ↓
Introduce semantic input actions
        ↓
Define versioned ESP32 protocol
        ↓
Integrate ESP32 bench hardware
        ↓
Begin verified R15 adapter work
```

Do not jump directly into speculative Yamaha integration.

------------------------------------------------------------------------

## 9. BikeState Contract

`BikeState` represents normalized motorcycle state.

Typical state includes:

``` text
speed
RPM
gear
fuel
engine/coolant temperature
left indicator
right indicator
high beam
neutral
ABS warning
engine warning
ignition state
phone connection state
```

Future state may include:

``` text
odometer
trip data
battery voltage
riding mode
navigation status
call state
signal availability
signal freshness
```

Use explicit units where ambiguity exists.

Examples:

``` text
speedKph
coolantTemperatureC
batteryVoltageV
odometerKm
```

Do not expose Yamaha-specific protocol fields directly to QML.

------------------------------------------------------------------------

## 10. Do Not Invent Motorcycle Facts

The exact Yamaha R15 V4 cluster protocol is not yet known.

Never fabricate:

-   CAN identifiers;
-   CAN payload formats;
-   K-Line messages;
-   connector pinouts;
-   ECU commands;
-   ABS commands;
-   switch resistor values;
-   cluster handshake behavior;
-   odometer protocol;
-   riding-mode protocol;
-   warning-bit meanings.

If information is unknown, represent the missing implementation with an
interface, TODO, mock, or clearly documented hypothesis.

Do not make fake values appear verified.

------------------------------------------------------------------------

## 11. Simulator Rules

The simulator is a replaceable data source.

It exists to enable development before hardware arrives.

Desktop controls may simulate:

``` text
speed
RPM
gear
fuel
indicators
high beam
neutral
ABS
engine warning
phone state
other test states
```

Simulator-specific mutation must gradually move out of the production
domain model.

Never make production code depend on keyboard availability.

------------------------------------------------------------------------

## 12. Input Architecture

The final product is non-touch.

Production-facing screens must ultimately work with physical controls.

Move toward semantic input actions such as:

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

Desktop keyboard input should translate into semantic actions.

Future ESP32/OEM switch input should translate into the same actions.

Screens should not need to know whether an action came from a keyboard,
GPIO, resistor ladder, CAN message, or other vehicle input.

------------------------------------------------------------------------

## 13. QML Rules

QML is the presentation layer.

Use it for:

-   visual composition;
-   layout;
-   animation;
-   presentation formatting;
-   focus/navigation presentation;
-   theme application.

Do not put into QML:

-   serial parsing;
-   vehicle protocol decoding;
-   persistent storage ownership;
-   blocking I/O;
-   substantial domain calculations;
-   hardware discovery;
-   Yamaha-specific message handling.

Small presentation-only formatting helpers are acceptable.

------------------------------------------------------------------------

## 14. Main.qml

`Main.qml` should become a thin application shell.

Do not continue indefinitely adding entire screens and subsystems to it.

Extract focused screens and components when modifying related areas.

Do not perform a giant rewrite solely to reduce file length.

Prefer behavior-preserving incremental extraction backed by tests.

------------------------------------------------------------------------

## 15. UI Quality Standard

The interface should look like a deliberate premium motorcycle HMI.

It must not look like:

-   a Qt example application;
-   an Arduino dashboard;
-   a desktop settings program;
-   a web dashboard pasted onto a motorcycle;
-   an unfinished developer tool.

Prioritize:

-   glanceability;
-   speed readability;
-   gear readability;
-   clear RPM progression;
-   warning visibility;
-   strong hierarchy;
-   consistent typography;
-   intentional spacing;
-   restrained animation;
-   high contrast;
-   predictable physical navigation.

Visual polish is a product requirement, not optional cleanup.

------------------------------------------------------------------------

## 16. Themes

Themes affect presentation only.

Theme logic must not change motorcycle/domain behavior.

Centralize design tokens such as:

``` text
colors
typography
spacing
radii
line weights
opacity
animation durations
warning states
```

Avoid scattering arbitrary colors and dimensions across QML.

------------------------------------------------------------------------

## 17. Animation

Animation must improve comprehension.

Never delay critical truth for visual effect.

Good:

``` text
RPM bar smoothly approaches latest RPM.
```

Bad:

``` text
ABS warning waits for a decorative transition before becoming visible.
```

Avoid animation queues and expensive effects that assume desktop GPU
performance.

Remember that the target is future embedded hardware.

------------------------------------------------------------------------

## 18. Application Processor vs Vehicle MCU

The long-term architecture intentionally separates rich UI from
real-time vehicle responsibilities.

Conceptually:

``` text
Phone
  |
Wi-Fi / Bluetooth
  |
  v
Embedded Linux application processor
  |
  | Qt/QML
  | projection
  | navigation/calls
  |
internal communication
  |
  v
Vehicle MCU
  |
  | vehicle communication
  | switch input
  | tell-tales
  | watchdog
  | ignition/power coordination
  |
  v
Motorcycle
```

The current Raspberry Pi/SBC discussion is prototyping strategy, not a
locked production component.

Do not assume Raspberry Pi 5 is part of the final product.

Do not assume ESP32-S3 must remain the production vehicle MCU.

------------------------------------------------------------------------

## 19. Google Maps / Phone Projection

Full Google Maps is a major product requirement.

The intended strategy is phone projection through Android Auto / Apple
CarPlay rather than recreating Google Maps inside the cluster.

Do not build a fake Maps clone.

Do not assume arbitrary compositing of projection surfaces with custom
cluster graphics is guaranteed.

Keep projection behind a subsystem boundary until feasibility is proven
on target hardware.

Calls may also be handled through the projection/phone integration path.

------------------------------------------------------------------------

## 20. Safety

Real motorcycle integration changes the risk profile of this project.

Early vehicle integration must be passive/read-only.

Do not transmit onto a real vehicle network unless the user explicitly
reaches a reviewed active-test phase.

Do not instruct code to cut or permanently modify the OEM harness.

Critical indicators may eventually require independent
vehicle-MCU/physical tell-tale behavior.

A Linux/QML crash must not casually be treated as acceptable loss of all
safety-critical indications.

Never claim regulatory compliance without actual verification.

------------------------------------------------------------------------

## 21. External Data Validation

Treat all external input as untrusted:

``` text
ESP32 messages
serial packets
vehicle data
phone/projection data
persisted settings
configuration files
```

Validate:

-   framing;
-   lengths;
-   types;
-   ranges;
-   protocol versions;
-   enum values;
-   malformed messages.

Unknown must not silently become zero.

Disconnected must not silently become a plausible value.

------------------------------------------------------------------------

## 22. ESP32 Integration

When hardware arrives, do not connect the ESP32 directly to QML.

Expected flow:

``` text
ESP32-S3
    |
USB serial
    |
SerialBikeDataSource
    |
normalization / validation
    |
BikeState
    |
UI
```

Define a documented, versioned serial protocol before relying on it.

Protocol documentation should specify:

-   framing;
-   protocol version;
-   message types;
-   units;
-   ranges;
-   invalid-message behavior;
-   connection/heartbeat semantics if required.

Keep protocol parsing independently testable.

------------------------------------------------------------------------

## 23. Signal Availability

Real data needs availability semantics.

Plan for distinctions such as:

``` text
available
unavailable
unsupported
stale
invalid
disconnected
```

Do not equate unavailable speed with `0 km/h`.

Do not equate unavailable fuel with empty.

UI behavior for missing data must be intentional.

------------------------------------------------------------------------

## 24. Threading

Never block the QML/UI thread with:

-   serial reads;
-   sleeps;
-   hardware waits;
-   long parsing;
-   network operations;
-   unpredictable filesystem work.

Prefer Qt's asynchronous/event-driven facilities.

Introduce worker threads only when justified.

Thread ownership and synchronization must be explicit.

------------------------------------------------------------------------

## 25. Timing

Use monotonic timing for durations.

Examples:

-   lap timing;
-   timeouts;
-   stale-data detection;
-   reconnect backoff;
-   heartbeat monitoring.

Use wall-clock time for clock/date display.

Do not derive elapsed durations from system wall-clock time.

------------------------------------------------------------------------

## 26. Persistence

Keep these concepts separate:

``` text
display/user preferences
simulation/session state
vehicle-owned persistent data
```

Never treat demo odometer data as real vehicle persistence.

Do not design software that could overwrite real odometer state without
an explicit, verified architecture.

------------------------------------------------------------------------

## 27. C++ Expectations

Follow `RULES.md`.

In particular:

-   use RAII;
-   make ownership clear;
-   avoid unnecessary globals;
-   use const correctness;
-   prefer scoped enums;
-   avoid manual memory management where unnecessary;
-   use explicit units;
-   keep classes cohesive;
-   avoid generic `Manager`, `Helper`, and `Utils` dumping grounds;
-   do not overengineer.

Code should be understandable to an experienced Qt/C++ engineer without
requiring hidden project knowledge.

------------------------------------------------------------------------

## 28. Qt Ownership

For every QObject, understand its lifetime.

Use one clear ownership strategy:

``` text
stack lifetime
QObject parent
smart pointer
QML engine ownership
```

Do not mix ownership models carelessly.

Do not manually delete parent-owned QObjects.

------------------------------------------------------------------------

## 29. Signals and State Updates

Avoid unnecessary notifications.

If a value did not effectively change, do not emit change signals unless
repeated notification is intentionally meaningful.

As the state model grows, prefer sufficiently specific
signals/properties rather than forcing the entire UI to react to one
universal `changed()` signal.

------------------------------------------------------------------------

## 30. Error Handling

System-boundary failures must be visible to the software architecture.

Examples:

``` text
serial disconnected
malformed packet
unsupported protocol version
stale data
settings failure
projection unavailable
missing resource
```

Do not hide these by injecting realistic-looking fallback data.

Use explicit unavailable/error state and diagnostic logging.

------------------------------------------------------------------------

## 31. Logging

Use meaningful Qt logging.

As subsystems grow, use categories such as:

``` text
cluster.app
cluster.ui
cluster.simulator
cluster.serial
cluster.vehicle
cluster.projection
```

Avoid per-frame spam.

Do not log private phone/user content unnecessarily.

------------------------------------------------------------------------

## 32. Testing Requirements

Changes to nontrivial behavior require tests when practical.

Testing layers should evolve toward:

``` text
Unit tests
    domain logic
    validation
    parsers
    calculations

Integration tests
    data source -> BikeState
    persistence
    disconnect/reconnect

UI tests
    navigation
    important screen states
    keyboard/semantic actions

Hardware tests
    ESP32 protocol
    bench electrical behavior

Vehicle validation
    actual R15 observations
```

Do not describe simulation as vehicle verification.

------------------------------------------------------------------------

## 33. Boundary Testing

Numeric behavior should consider:

``` text
below minimum
minimum
normal
maximum
above maximum
```

Connection behavior should consider:

``` text
disconnected
connecting
connected
valid data
malformed data
stale data
disconnect
reconnect
```

Warnings should be tested in meaningful simultaneous combinations.

------------------------------------------------------------------------

## 34. UI Verification

For significant presentation changes, verify representative states at
800×480.

Where applicable inspect:

-   Street layout;
-   Track layout;
-   Midnight theme;
-   Daylight theme;
-   normal state;
-   warning state;
-   low fuel;
-   disconnected/unavailable data;
-   menu/settings navigation.

Compilation alone is not visual verification.

------------------------------------------------------------------------

## 35. Build Requirements

Before declaring a change complete:

1.  configure/build the relevant target;
2.  run relevant tests;
3.  inspect runtime warnings/errors;
4.  visually verify UI when changed.

Do not claim commands were executed if they were not.

Do not mark a feature done merely because source code was written.

------------------------------------------------------------------------

## 36. CMake

Use target-oriented modern CMake.

Prefer:

``` cmake
target_sources(...)
target_link_libraries(...)
target_include_directories(...)
target_compile_definitions(...)
```

Avoid unnecessary global flags and directory-wide side effects.

Keep dependencies explicit.

Keep macOS-specific and future Linux-specific configuration isolated.

------------------------------------------------------------------------

## 37. Dependencies

Before adding a dependency, evaluate:

-   necessity;
-   Qt/standard-library alternatives;
-   ARM/Linux support;
-   binary size;
-   runtime cost;
-   licensing;
-   maintenance status;
-   security impact.

This will become an embedded product.

Do not add large dependencies for trivial convenience.

------------------------------------------------------------------------

## 38. Assets

Runtime-critical assets should ship with the application.

Do not depend on:

-   internet fonts;
-   CDNs;
-   fonts installed only on the developer Mac;
-   absolute filesystem paths.

Track licenses/provenance for bundled fonts and other third-party
assets.

------------------------------------------------------------------------

## 39. Documentation

Update documentation when a change affects architecture, behavior,
workflow, milestones, hardware assumptions, or input mappings.

Use the repository's existing documentation structure.

Important architectural decisions belong in `docs/decisions.md`.

Do not silently rewrite historical decisions; mark old decisions
superseded when necessary.

------------------------------------------------------------------------

## 40. Git Rules for Agents

Routine Git operations remain under user control unless explicitly
requested.

Never perform destructive Git operations without explicit permission.

Do not:

-   force push;
-   reset user work;
-   discard unrelated modifications;
-   rewrite history;
-   commit build artifacts;
-   commit generated source-export files.

Keep changes scoped to the requested task.

Respect existing uncommitted changes.

------------------------------------------------------------------------

## 41. Generated Files

Do not treat generated output as source.

Typical generated/local artifacts that should remain untracked include:

``` text
build/
project-source.txt
temporary screenshots
IDE metadata
OS metadata
local caches
```

Do not modify generated build files as a substitute for changing
source/CMake configuration.

------------------------------------------------------------------------

## 42. Refactoring

Refactors must preserve behavior unless behavior changes are part of the
request.

For meaningful refactors:

1.  inspect current behavior;
2.  inspect tests;
3.  add characterization tests if needed;
4.  refactor incrementally;
5.  build;
6.  run tests;
7.  visually inspect affected UI.

Do not combine unrelated architectural and visual rewrites into one
giant change.

------------------------------------------------------------------------

## 43. Avoid Premature Abstraction

Production-grade does not mean maximum abstraction.

Do not create:

-   factories with one trivial implementation and no boundary benefit;
-   generic repositories for simple settings;
-   dependency-injection frameworks without need;
-   event buses to avoid ordinary signals;
-   deep inheritance trees;
-   unnecessary service locators;
-   generic `Utils` modules.

Abstractions must solve a real problem.

Prefer explicit, boring, understandable code.

------------------------------------------------------------------------

## 44. Avoid Prototype Shortcuts Becoming Architecture

Also avoid the opposite problem.

Do not permanently solve cross-layer problems by:

-   calling C++ hardware APIs directly from random QML components;
-   storing every state field globally;
-   putting all screens in `Main.qml`;
-   using hard-coded demo data throughout UI files;
-   parsing protocols inside button handlers;
-   adding arbitrary timers until behavior appears correct.

Prototype convenience must have an exit path.

------------------------------------------------------------------------

## 45. No Fake Completeness

Use project-status language precisely.

Distinguish:

``` text
designed
simulated
implemented
unit tested
bench verified
hardware verified
bike verified
production ready
```

Never report one as another.

Examples:

-   A Maps placeholder is not Google Maps integration.
-   A simulated ABS lamp is not ABS integration.
-   A mocked serial packet is not ESP32 verification.
-   A guessed CAN frame is not Yamaha protocol support.

------------------------------------------------------------------------

## 46. Task Workflow for Coding Agents

For each requested task:

### Step 1 --- Understand

Read:

``` text
AGENTS.md
RULES.md
relevant docs
relevant source
relevant tests
```

Determine the correct architectural layer.

### Step 2 --- Plan

Identify:

-   files affected;
-   state/API changes;
-   tests required;
-   documentation impact;
-   assumptions;
-   risks.

Do not produce a giant plan for trivial work.

### Step 3 --- Implement

Make the smallest coherent production-quality change.

Reuse existing patterns when they are sound.

Do not rewrite unrelated code.

### Step 4 --- Verify

Run:

-   build;
-   relevant tests;
-   UI preview/render if applicable.

Check warnings.

### Step 5 --- Document

Update relevant docs if architecture or behavior changed.

### Step 6 --- Report

Summarize:

-   what changed;
-   important design decisions;
-   tests/commands run;
-   remaining limitations;
-   anything requiring physical hardware verification.

------------------------------------------------------------------------

## 47. When Requirements Are Ambiguous

Do not guess important product or hardware behavior.

For software-only ambiguity, prefer the interpretation most consistent
with existing architecture and documentation.

For hardware/protocol ambiguity:

-   do not invent;
-   preserve an interface;
-   add a TODO;
-   state what measurement/documentation is required.

Ask the user only when the ambiguity materially changes the product
behavior or architecture and cannot safely be deferred.

------------------------------------------------------------------------

## 48. Definition of Done

For normal software work, a change is complete when:

-   architecture boundaries remain clean;
-   code builds;
-   relevant tests pass;
-   edge cases are handled;
-   no obvious runtime/QML warnings were introduced;
-   visual changes were actually inspected;
-   non-touch navigation remains viable;
-   simulated values remain clearly identified;
-   documentation is updated where required;
-   no unverified Yamaha assumptions were introduced.

Hardware-dependent functionality is not complete until verified at the
appropriate hardware milestone.

------------------------------------------------------------------------

## 49. Agent Review Checklist

Before finishing, verify:

### Architecture

-   Is the code in the correct layer?
-   Did QML gain hardware knowledge?
-   Did BikeState gain unrelated responsibility?
-   Is a new abstraction justified?

### Correctness

-   Are units explicit?
-   Are values range-checked?
-   Is unavailable different from zero?
-   Are external inputs validated?

### Qt/C++

-   Is QObject ownership clear?
-   Are signals emitted appropriately?
-   Is the UI thread protected from blocking work?
-   Are lifetimes deterministic?

### UI

-   Does it fit 800×480?
-   Is it readable at a glance?
-   Does it work without touch?
-   Are relevant themes/layouts still valid?
-   Are animations bounded?

### Testing

-   Were relevant tests added/updated?
-   Were they actually executed?
-   Was UI output visually checked when necessary?

### Vehicle Safety

-   Did this introduce an unverified Yamaha assumption?
-   Did it accidentally introduce vehicle transmission behavior?
-   Could failure hide critical information?
-   Is simulated behavior clearly distinguished from real behavior?

------------------------------------------------------------------------

## 50. Guiding Principle

The repository should communicate deliberate engineering decisions
rather than developer seniority through complexity.

A new experienced Qt/embedded engineer should be able to open the
project and quickly understand:

``` text
where data comes from
where it is normalized
where domain logic lives
where UI behavior lives
how input reaches screens
what is simulated
what is verified
what is hardware-specific
how failures are represented
how behavior is tested
what remains unresolved
```

If a subsystem can only be understood through tribal knowledge, improve
either its design or its documentation.

Keep the codebase simple where possible, strict where necessary, and
explicit everywhere that matters.
