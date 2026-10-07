# Engineering review and refactor — 2026-10-07

## Outcome and scope

Reviewed application startup, all C++ models, QML screens/components, tests, CMake, scripts, bundled-font provenance and project documentation. Refactored the desktop simulator while preserving the approved 800×480 UI, Bricolage font, RPM gradient, fuel thresholds, icon opacity and keyboard increments. No firmware, vehicle transmission, projection subsystem or unverified Yamaha protocol was introduced.

The existing uncommitted visual changes were preserved. Routine Git operations remain with the user. Factual technical-debt notes in AGENTS.md and RULES.md were updated to match the implementation; engineering standards remain unchanged.

## Findings and changes

| Finding | Change | Remaining boundary |
| --- | --- | --- |
| Main.qml contained every screen and desktop controls | Five focused screens, ClusterHeader, Tachometer, ClusterText, Theme and DesktopInput extracted; shell retains navigation and small presentation helpers | A future physical input adapter can call semantic shell actions; no physical adapter exists yet |
| BikeState contained simulator mutation | Read-only observation model in domain/, typed BikeSnapshot contract, IBikeDataSource boundary, concrete SimulatorBikeDataSource | Real adapters must validate raw framing, versions and required fields before publishing |
| Ambiguous public units | speedKph, engineRpm, fuelPercent, phoneBatteryPercent; distance properties end in Km, lap durations in Ms | Fixed simulator ranges are not verified bike limits |
| Missing source-loss behavior | Available/disconnected/invalid/stale frame states, monotonic freshness watchdog, explicit UI dashes and source-error footer | Full per-signal unsupported/invalid/stale semantics are still required for real integration |
| Sample battery/coolant values lived in QML | Simulator publishes sample normalized values and availability flags; UI formats only observations | These fields remain simulated, not electrically measured |
| Demo odometer baseline lived in domain logic | Startup explicitly supplies the simulator's demo baseline; RideComputer defaults to zero initial session distance | Vehicle-owned odometer ingestion/persistence is a separate unimplemented contract |
| Repeated values caused broad notifications | Instrument/tell-tale/phone notification groups; identical heartbeats do not notify presentation | Additional per-field groups should follow measured need |
| Signed arithmetic could overflow | Simulator adjustments use 64-bit intermediate arithmetic and clamp; extreme input tests added | Adapters must independently reject malformed raw values |
| RideComputer held an unchecked raw pointer | Required state reference at construction, guarded QPointer thereafter, source destruction stops timer; unavailable speed is never integrated | Lap timing stays monotonic and session-owned |
| Settings did not validate stored data or check save failures | Stored booleans/offset validated; background worker checks QSettings status; failures visible in footer; latest pending values coalesced | Runtime preference changes survive in memory after a failed write but may be lost on restart |
| No persistent diagnostics | Named Qt categories/source locations, bounded background log queue, rotating files, stderr fallback | Fatal process faults/power loss cannot guarantee disk flush |
| UI load failures only exited | Independent dependency-light error window for startup failures; runtime QML warnings and invalid navigation are logged and surfaced | A failure window is not an independent motorcycle safety channel |
| Header-only service implementations blurred responsibilities | Focused headers/.cpp files and shared cluster_core CMake target used by app and tests | No speculative service framework or empty hardware classes |

## Current error contracts

A normalized full snapshot uses km/h, RPM, percent, volts and Celsius. Software guard ranges are 0–400 km/h, 0–30,000 RPM, gear 0–6, fuel 0–100%, phone battery −1–100% (connected requires 0–100%), 0–60 V and −40–200°C. Float values must be finite. Both direction flags without hazards are rejected. These guards are defensive software choices, not Yamaha specifications. Simulator controls retain 180 km/h/13,000 RPM bounds.

Invalid snapshots leave prior observations intact and mark the frame invalid. Disconnection marks unavailable immediately. The simulator publishes a one-second heartbeat; BikeState marks stale after three seconds without a valid update, checked at most every 250 ms. Both durations are software defaults requiring review for a real adapter. Freshness currently measures time since domain receipt; upstream queued-packet age and full per-signal timestamps must be added with a real transport. A valid snapshot recovers availability. Repeated invalid frames do not flood UI notifications or logs; the first transition records its rejected field/reason. Qt queued delivery is supported by the registered snapshot metatype; future worker-thread sources must respect QObject affinity.

Unavailable speed, RPM, gear and fuel use dashes; the RPM fill is depleted and fuel has no filled segments. The percentage is unknown, never shown as a known empty tank. Neutral/phone notifications do not imply an active connection when unavailable. Last observed warning/turn flags are retained with the explicit unavailable footer rather than silently clearing them. This does not establish a safe critical tell-tale policy for a real vehicle.

Battery/coolant have explicit availability flags and render dashes independently. Every real adapter must deliberately populate required observations; default-constructed numeric fields must never be used to conceal missing parser fields. Full per-signal freshness/support modeling remains a gate before real signal integration.

Settings writes use one worker, one in-flight batch and at most four pending keys with the latest value winning. UI changes are immediate; persistence completion/error is observable. Pending writes are drained during normal shutdown. Logging uses one worker, a 512-message queue, 4 KiB line bound, and three files of up to 512 KiB each. Overload records dropped-message counts; failed disk access retains stderr output. Disk I/O is outside interactive UI callbacks. Shutdown waits for owned workers; pathological storage stalls and abrupt power loss still require embedded shutdown/watchdog design.

The application catches standard/unknown C++ exceptions at its startup/application call boundary and logs them while the logger is alive. This is not a mechanism for recovering segmentation faults, memory corruption, Qt fatal errors or arbitrary exceptions escaping asynchronous Qt callbacks. Expected boundary failures use validation and explicit result/status paths instead of exception swallowing. Runtime QML errors latch a visible diagnostic message; they do not fabricate healthy readings or automatically reload the UI.

## Ownership and portability

Application startup owns state, simulator, settings, ride computer, diagnostics and QML engines by stack lifetime. The engine is destroyed before its injected dependencies. The ride computer guards its borrowed state. Settings owns its QThread and a worker deleted when that thread finishes. The logger outlives services and their worker threads and restores the previous Qt message handler on destruction. A single process log sink is the only global required by Qt's message-hook API.

Qt 6/C++17, portable CMake targets, Qt filesystem/settings APIs and packaged fonts remain unchanged. Main startup selects the simulator explicitly; an embedded build will supply a verified source/input/power architecture rather than inheriting desktop keyboard assumptions. No Raspberry Pi/ESP32 production specification or final connector/display compatibility is claimed.

## Verification

- Build and five CTest groups: simulator-state, failure-boundaries, cockpit-ui, startup-smoke, startup-fallback.
- Coverage includes arithmetic extremes, rejected snapshots, grouped/no-op signals, stale/disconnect/recovery, source destruction, invalid ride intervals, corrupt preferences, failed disk writes, coalesced shutdown writes, log rotation, runtime diagnostics and independent fallback rendering.
- Existing keyboard, themes, Street/Track, clock, trips/laps, hazard exclusivity, shift blinking and 0/9/10/19/20/100% fuel checks retained.
- Offscreen 800×480 rendering reviewed. Pixel comparison with the saved pre-refactor baseline found zero changes outside the live clock for Street, Track, Daylight, settings and help. Blink-phase captures vary intentionally.
- AddressSanitizer build succeeded, but its runtime stalled during interceptor initialization on this host and was interrupted before tests ran; no ASan pass is claimed. All five test groups also passed in a separate UndefinedBehaviorSanitizer build with no runtime error reports.
- Temporary baseline/verification captures live under ignored build/; they are not source assets or bike verification.

## Before assembling or replacing the OEM console

1. Confirm the exact ordered boards, display, voltages and connectors. Current software does not establish electrical compatibility.
2. Define a versioned ESP32 bench protocol with framing, units, required/optional fields, heartbeat, error and reconnect behavior; independently test the parser.
3. Implement serial and physical-input adapters behind the existing boundaries; bench-test malformed data, cable loss, reconnection and simultaneous warnings.
4. Verify ARM/Linux build, packaged assets, actual display readability/frame timing, startup/shutdown, ignition power behavior, storage health, watchdog and recovery on the selected hardware.
5. Specify independent vehicle-MCU/physical critical indications and failure policy. A QML/Linux crash must not be assumed to preserve safety-critical information.
6. Begin passive/read-only vehicle observation only after bench verification; verify exact R15 signal meanings, OEM inventory and connector behavior before replacement.

Production-ready, hardware-verified and bike-verified are still unachieved states.
