# Engineering and hardware gates

Baseline: 2026-10-18. This is a verification plan, not a completed hardware review.

- Confirm actual board revision, panel pin mapping, timing, PSRAM and UART/console conflicts using matching vendor resources.
- Record harness measurements with connector orientation, pin numbering, conditions, voltage, polarity, loading and uncertainty. Do not infer protocol from wire labels alone.
- CAN bus lines connect through suitable bus protection/transceiver circuitry. Isolation requires a designed signal/power barrier; generic optocouplers in CAN H/L are not an approved interface. Verify bus type, termination and passive controller operation.
- Validate each discrete/analog/pulse input separately. No direct vehicle voltage to MCU GPIO; board power input rating does not establish GPIO tolerance.
- Verify regulated supply under startup, cranking, key-off hold-up, rapid cycling and hard power loss. Shutdown artwork requires retained supply; review [power lifecycle](power-lifecycle.md).
- Confirm independent warning strategy and failure behavior if either MCU or UART stops. Two MCUs alone do not provide safety certification or fault independence.
- Measure internal SRAM, PSRAM, flash/image fit, task stacks, queues, render times and radio/acquisition contention under representative load.
- Review real odometer provenance, persistence atomicity/wear and reset permissions before storing vehicle-owned data.
- Test boot/shutdown animation timing and urgent-warning interruption on host and panel. Record total key-on latency separately.
- Keep OEM functionality during initial passive testing. Do not transmit to a real vehicle without an explicitly reviewed active-test phase.

Software, bench, hardware and bike verification are separate evidence levels. No regulatory compliance or production readiness is currently claimed.
