# Vehicle BLE pairing bench

Implemented in firmware; first numeric-comparison pairing and saved-bond reconnect after powering both boards off/on are user bench-confirmed (2026-10-30). Vehicle owns BLE and bond storage; display owns the approval UI. This milestone provides authenticated BLE bonding only. Wi-Fi, companion application data, navigation, media and maps remain pending. The existing Flutter/Mac companion transport is not yet adapted to this firmware.

## Wiring and flashing

Power off and unplug both boards before changing leads. Keep the working vehicle GPIO17 → display GPIO_D IO38 wire and shared GND. Add **display UART0 TX GPIO43 → vehicle GPIO18** using the second connector cable. Identify TX from the PCB labels/schematic, not cable color or viewing orientation. Insulate its RX GPIO44, 3V3 and unused GND leads. Both boards retain separate USB power; never join their power rails.

UART0 RX44 shares the display USB bridge transmitter: do not connect a vehicle transmitter there. TX43 is the existing console output and also feeds the USB bridge receiver. Firmware sends framed control messages through that same output without reassigning the pin. The GPIO_D receive path stays unchanged.

Stop the clock helper and serial monitors. Disconnect inter-board leads before separately flashing each board. Ports can change; list them first with `bash scripts/flash-vehicle.sh --list-ports`.

```bash
R15_VEHICLE_BAUD=115200 bash scripts/flash-vehicle.sh --port /dev/cu.usbserial-0001
R15_DISPLAY_PORT=/dev/cu.usbserial-10 bash scripts/flash-display.sh
```

For the vehicle's manual download mode, hold BOOT, tap RESET, and release BOOT once the flashing stub starts. Reconnect leads with both boards powered off, then power both by USB. The gear icon should show a live vehicle connection; Settings → Phone should offer Pair phone. Optional clock sync:

```bash
bash scripts/sync-clock.sh /dev/cu.usbserial-10
```

## Android acceptance

Use [Nordic nRF Connect for Android](https://github.com/nordicsemi/Android-nRF-Connect) for this bench test. Android's ordinary Bluetooth settings may not list this BLE peripheral.

1. Select Pair phone on the cluster. This opens a 60-second enrollment window.
2. Scan for **R15 Cluster**, connect and request Bond in nRF Connect.
3. Compare the actual six-digit code on Android and the cluster. Confirm only when both match. The cluster defaults to Reject; select Allow within 30 seconds and approve on Android.
4. Expect **Authenticated BLE bond connected** only after authentication and bond completion. A connected GATT link alone is insufficient.
5. Disconnect retains the bond. Reconnect from the phone and test after power cycling both boards. Forget explicitly removes the vehicle's saved bond; remove the corresponding Android bond too before pairing again.

Also test Reject, both timeouts, a missing return wire, UART loss during approval, and GPIO indicators/navigation while BLE is busy. Existing telemetry must continue and stale connection state must remain visible. Do not claim radio, panel performance or bike verification from host tests.

## Implemented boundaries

NimBLE uses authenticated LE Secure Connections with a 128-bit key, numeric comparison, one connection and one saved bond. Legacy pairing, debug keys and automatic bond replacement are disabled. Fresh display heartbeat and an explicit enrollment action are required for a new bond. Pending approval is cancelled on display loss. An established authenticated connection may remain connected when display communication is lost. No application GATT service or vehicle-control commands are exposed.

Bonds persist in vehicle NVS across resets/power cycles. Storage errors make the service unavailable; firmware never erases NVS to recover. Development NVS is not encrypted or protected against physical readout. Reflashing a combined factory image may overwrite NVS; persistence across firmware updates is not promised.

The BLE host owns all radio/storage calls. A bounded four-command queue expires entries after one second. Host health expires after one second independently of UART telemetry. UI never calls BLE APIs or mutates confirmed connection state optimistically.

## Narrow UART sideband v1

The existing 36-byte R15G v3 telemetry remains unchanged. A fixed 48-byte R15P v1 stream carries phone status vehicle→display and phone controls display→vehicle at 115200 8N1. Shared codec: `shared/protocol/PhoneLink.h`; independent golden vectors: `shared/protocol/phone-link-v1-vectors.txt`.

| Bytes | Encoding |
| --- | --- |
| 0–3 | ASCII R15P |
| 4, 5 | Version 1; kind Status=1 / Control=2 |
| 6 | Flags: paired, connected, requested, approved, enrollment, command failed (bits 0–5) |
| 7 | Result None=0 / Queued=1 / Rejected=2 |
| 8–11, 12–15 | Vehicle boot session, display boot session |
| 16–19, 20–23 | Sender sequence, request ID |
| 24–27, 28–31 | Pairing challenge nonce, numeric code 0–999999 |
| 32 | Heartbeat=0 / Pair=1 / Approve=2 / Reject=3 / Disconnect=4 / Forget=5 |
| 33 | Status quality Available=0 / Unavailable=1 / Stale=4; control=0 |
| 34–35 | Reserved zero |
| 36–39 | Source monotonic uptime milliseconds modulo 2^32 |
| 40–43 | Reserved zero |
| 44–47 | CRC-32/ISO-HDLC over bytes 0–43 |

All integers are little endian; no native structs are transmitted. CRC uses reflected polynomial 0xEDB88320, initial/final XOR 0xFFFFFFFF. Receivers use bounded sliding buffers, strict field validation and session/sequence/uptime ordering. CRC detects corruption; it does not authenticate physical UART attackers.

Status and heartbeat cadence is 200 ms; freshness is 1,000 ms. Controls require an established matching session and fresh heartbeat. Requests retry every 200 ms with a 1,500 ms local deadline; vehicle deduplicates IDs. Approve/Reject must match the current random pairing nonce. Queued acknowledges queue admission only; subsequent status confirms the result. Reboots invalidate prior-session commands. RX overflow invalidates freshness without clearing accepted-request deduplication.

Display console output includes binary control frames followed by LF alongside diagnostic text. LF conversion is disabled to preserve bytes. Vehicle accepts only validated Control frames; clock helper tolerates binary console lines. This is a bench sideband, not the final full telemetry/map protocol.

## Diagnosing unavailable Phone status

The normal gear color proves only the R15G telemetry path. Phone distinguishes missing status packets, a mismatched vehicle session, an unconfirmed return UART and a vehicle BLE service fault. “Return UART not confirmed” means the vehicle has not echoed this display boot session; check TX43 → GPIO18 and the existing common ground. One shared GND is sufficient. Repeated Android status 19 disconnects before enrollment are consistent with the vehicle rejecting an unapproved connection; they do not establish a radio failure.

BLE health age is measured when the snapshot is observed, accounting for the host task publishing after the acquisition loop timestamp was captured. A newer concurrent publication must not be reported unavailable. The fake-radio regression reproduces this scheduling order and retains the one-second stale-health boundary. Subsequent user bench evidence confirms first pairing and a saved-bond reconnect; prolonged/concurrent load testing remains pending.

If the return channel is confirmed but BLE becomes unavailable during connection, capture the bounded vehicle fault diagnostic over its USB console. Stop other serial monitors first. With the current native USB vehicle port:

```bash
/Users/codewavesandy/.platformio/penv/bin/python tools/vehicle/ble_log.py /dev/cu.usbmodem1101
```

The reader sends no commands and filters binary input snapshots out. Start it before selecting Pair phone and connecting Android. `@BLE FAULT` reports only an operation, numeric result/object type or health age, never keys or numeric-comparison codes. Faults are latched/rate-limited; if already emitted, start the reader before powering the vehicle and reconnect if USB enumeration changes. This diagnostic identifies a failure; it does not establish its cause without the reported result.

Advertising restarts only when synchronization, disconnect, a failed connection or advertising completion explicitly requests it. In pinned ESP-IDF 5.5.4 NimBLE, the sole connection slot can be reserved before CONNECT reaches the application (remote-feature exchange precedes that callback). Periodic polling must not restart advertising during this interval: `ble_gap_adv_start` returns `BLE_HS_ENOMEM` (6) when no connection slot remains. The fake-radio regression models this interval and verifies pairing status remains available and no restart is attempted. This is not evidence of general heap exhaustion. Subsequent first pairing and saved-bond reconnect are user bench-confirmed.

## User bench evidence — 2026-10-30

User reports matching-code first pairing succeeded, then both boards were powered off/on and nRF Connect reconnected after tapping Connect without another code. The supplied Android log shows BONDED, then a reconnect with encryption ENABLED / AES (16 bytes), with no new pairing exchange in that reconnect segment. This supports bond persistence across the reported power cycle and encrypted saved-bond reuse. Connection initiation at 16:03:04.663 reaches Connected at 16:03:05.019 (about 356 ms); encryption notification follows at 16:03:05.166 (about 503 ms from initiation). These are this run's Android log timings, not latency guarantees.

A GATT CONN TIMEOUT appears before the reconnect, consistent with the reported power interruption; the log alone cannot establish that cause. The phone currently shows CONNECTED/BONDED. Automatic phone-initiated reconnect, extended stability, failure cases and GPIO/render performance under radio load remain separate acceptance work. No pairing code, bond key or phone content is retained in this record.

## Forget while connected

Forget phone opens a confirmation with Cancel as the initial focus. Confirm sends one semantic Forget request. The display then shows “Disconnecting and forgetting phone...” and suppresses repeat operation rows while waiting; a queue acknowledgement alone does not count as completion. Back remains available. The vehicle requests disconnection once, waits for the actual disconnect event, stops advertising before NimBLE unpair, verifies the saved-bond list and resumes advertising. For an already disconnected saved phone it stops advertising and removes the bond directly. Only a confirmed unpaired status restores Pair phone / Back. The enrollment window stays closed until Pair phone is selected again.

Vehicle completion has a three-second deadline, and the display's completion wait expires after 3.5 seconds. A refused request, storage error, lost link or timeout remains explicit rather than assuming the phone was forgotten. Android may retain its own bond; remove that on Android before a fresh pairing if necessary. Connected/disconnected completion and repeated-input handling are host-tested; physical Forget acceptance still needs testing on the boards. No companion-app files change for this fix.
