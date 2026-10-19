# Ignition and display lifecycle

Date: 2026-10-18. User-selected journey; hardware implementation unverified.

## Intended journey

```text
Key ON -> boot animation (1.0–1.5 s) -> live instrumentation
Key OFF -> shutdown animation (1.0–1.5 s) -> controlled power-off
```

Both animations are product requirements. The host implements initial progress-line artwork with Lifecycle::animationMs set to 1,250 ms. Final artwork remains undecided; preserve the instrument style, use lightweight LVGL primitives/assets and avoid video decoding. Actual first-visible-frame timing and retained shutdown power still require panel and power-path measurements.

## State model and ownership

| State | Behavior | Exit |
| --- | --- | --- |
| Off | No display execution assumed | Stable power and key-on |
| Starting | Minimal board initialization; start acquisition/UART concurrently; unknown values remain unknown | Display can render boot animation |
| BootAnimation | Play 1–1.5 s using monotonic timing while state acquisition continues | Animation completes -> Running; valid urgent warnings can interrupt/overlay immediately |
| Running | Actual speed/ODO/gear and other supported observations; dashes/status for missing data | Confirmed key-off -> ShutdownAnimation; independent faults remain explicit |
| ShutdownAnimation | Play 1–1.5 s; stop optional work, request bounded persistence and retain essential indication | Animation complete and required bounded shutdown work finishes -> ReadyForPowerOff |
| ReadyForPowerOff | Report readiness to the power owner if supported; no unverified power-control GPIO | Hardware safely removes retained supply |

The vehicle MCU owns validated ignition observation and supplies a defined lifecycle event/status. The display lifecycle controller owns navigation/animation transitions; widgets do not monitor power pins. The electrical hold-up/latch controller is a separate hardware responsibility. Display communications failure alone never means key-off. If ignition events are not yet available, bench simulation must be visibly identified.

## Timing and critical truth

Animation duration is measured from its first visible frame to its completion. It is distinct from total key-on latency: reset, bootloader and panel initialization add time that must be measured. No total boot-time guarantee is claimed. Boot animation must not wait for CAN, phone, Wi-Fi or BLE readiness. Data acquisition starts as early as practical; latest valid state is ready when instrumentation appears.

Proposed exception: a valid urgent warning interrupts or overlays the boot/shutdown artwork immediately. Never hide critical truth to finish decoration. This exception and which indications qualify must be reviewed before bike use. If animation resources fail, proceed to instrumentation or an explicit fault indication rather than holding a blank screen.

Running means actual received observations where supported. Do not show demo zero speed, neutral, fuel or odometer while waiting. Vehicle odometer is not derived from a demo baseline; unavailable ODO stays unavailable. Date/time needs an explicit source/quality after cold power loss.

## Shutdown power contract

A switched supply that disappears at key-off cannot run a shutdown animation. Hardware must provide ignition sensing separately from a retained regulated supply for both MCUs as needed. Required retention covers detection, the 1–1.5 s animation, bounded pending writes, coordination and measured margin. Component values, capacitor sizing, latch circuit, thresholds and readiness wiring remain undecided. Firmware cannot promise animation under hard power removal or brownout.

Persist preferences on change through bounded/coalesced writes; do not depend exclusively on a final shutdown write. Vehicle counters require their own wear, retention and power-loss policy before being treated as reliable persistence. The display may report ready, but it must not cut shared power while vehicle-side required work is pending. Define an overall bounded shutdown deadline and hardware fallback before implementation; no indefinite wait for the other MCU or phone.

Key-on during shutdown cancels power-off and returns to Running if state is valid; discard any stale readiness request. The hardware handshake must prevent a previous readiness message from cutting power after key-on. Rapid key changes, reset during shutdown and loss of UART require explicit session/transition identifiers and idempotent handling. Debounce and ignition polarity come from measured electrical behavior, not guessed constants.

## Verification

Host tests use an injected monotonic clock: duration lower/upper bounds, initial missing data, animation failure, warning interruption, key-on during shutdown, repeated key events and no key-off from link loss. Bench tests measure first visible frame, total startup, shutdown retention, persistence completion, brownout/hard cut, independent MCU resets and rapid off/on cycles. Final animation readiness requires actual-panel inspection and power-path verification.
