# Vehicle BLE pairing bench

Implemented in firmware; actual Android/radio acceptance remains pending. Vehicle owns BLE and bond storage; display owns the approval UI. This milestone provides authenticated BLE bonding only. Wi-Fi, companion application data, navigation, media and maps remain pending. The existing Flutter/Mac companion transport is not yet adapted to this firmware.

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
