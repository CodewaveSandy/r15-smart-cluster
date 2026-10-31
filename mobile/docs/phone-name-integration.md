# Pairing phone-name integration (firmware contract)

Implemented 2026-10-31; firmware builds and host tests are evidence, not phone/panel acceptance. Companion implementation remains on the second laptop; no mobile sources were edited here.

Connected name, battery and clock updates now have a separate [authenticated endpoint and app handoff](phone-connected-metadata-handoff.md). The enrollment endpoint below remains unchanged.

## Companion → vehicle BLE

The cluster cannot infer “Samsung S25 Ultra” from the current numeric-comparison event. The app supplies a display label on the same GATT connection it uses to pair:

| Item | Value |
| --- | --- |
| Service UUID | `813b1240-8fd9-4d67-a53b-9f0a2531c010` |
| Phone-name characteristic UUID | `813b1240-8fd9-4d67-a53b-9f0a2531c011` |
| Operation | Write with response; no read/notify/application commands |
| Value | 1–32 printable ASCII bytes (`0x20`–`0x7E`), at least one non-space byte |
| Example | `Samsung S25 Ultra` (17 bytes), hex `53616d73756e672053323520556c747261` |

No JSON, terminator, length prefix, newline or embedded NUL. The app may use its local Bluetooth name or an explicitly chosen friendly label. Names outside this font-compatible ASCII profile require an appropriate user-visible fallback; full Unicode support is not implemented.

1. User selects Pair phone on the display (60-second window).
2. App connects, discovers the UUIDs and writes its name immediately, before explicitly requesting bonding. Keep this same connection for pairing.
3. For an unbonded peer the vehicle allows up to three seconds for discovery/name delivery before initiating security. A valid name advances security initiation to the next host turn (at least 20 ms). A peer may start security earlier; name receipt never bypasses the code check.
4. Compare the actual six-digit code on both devices, then approve on both. Name write success is not pairing approval.

Existing bonded reconnects initiate security immediately and do not wait for metadata. Name writes require the current connection, fresh display heartbeat, an open enrollment window, a ready service and no prior approval. Existing bonded peers cannot rename themselves through this enrollment endpoint. The first valid label is fixed for the connection; an identical retry is allowed, a different replacement is refused. No extra BLE connections or central/scanner roles are enabled.

The name is **untrusted identification text**, not authenticated proof of which phone is present. Before approval this single label field is deliberately writable without encryption; it cannot trigger vehicle actions, alter the pairing code or authorize a bond. Invalid length/control/non-ASCII/all-space values fail with ATT invalid value length; disallowed state/peer/replacement fails with write-not-permitted. No name is logged or persisted. Disconnect/reset clears it.

nRF Connect does not automatically send this custom value. It can still pair with the fallback “Phone name unavailable”. The companion must implement the write to show the actual phone name reliably. For a bench test using a BLE client, send the example write on this characteristic during the enrollment connection; automatic scripting is preferable to racing the three-second window manually.

## Vehicle → display UART

Existing R15G v3 and R15P v1 frames are unchanged. Optional R15N v1 messages carry the label only while numeric comparison is pending. Codec: `shared/protocol/PairingIdentity.h`.

| Bytes | Encoding |
| --- | --- |
| 0–3 | ASCII R15N |
| 4 | Version 1 |
| 5 | Name length 1–32 |
| 6–7 | Zero |
| 8–11 | Vehicle boot session |
| 12–15 | Current R15P pairing nonce |
| 16–19 | Name-stream sequence, modulo 2^32 |
| 20–23 | Source uptime milliseconds, modulo 2^32 |
| 24–55 | Printable ASCII name, zero-padded |
| 56–59 | Zero |
| 60–63 | CRC-32/ISO-HDLC over 0–59 |

All integers are little endian. Strict bounds, reserved bytes, padding and CRC validation precede publication; sequence/uptime ordering rejects duplicates/rollback within a vehicle session. Receiver buffers are fixed. Labels expire after one second and are shown only when both the current vehicle session and active pairing nonce match. A corrupt/stale/mismatched name never changes the approval code or authorizes a connection. New firmware on both MCUs is required for name forwarding; older displays simply ignore the new frames.

Name cadence is at most 5 Hz. A name is sent on a different sampling cycle from R15P status: 36-byte input + 64-byte name fits the 128-byte UART TX FIFO. It never extends the 36+48-byte status cycle. Physical throughput/render performance remains to be measured.

## Modal and acceptance

The large modal occupies only Settings content, preserving the header and warning lamps. Phone identity is a bold, single-line heading; long labels use an ellipsis. Six-digit codes retain leading zeros and use the same bold 128-size font as speed. Allow pair has solid theme-accent fill; Reject pair has transparent fill and an outlined border. Non-touch focus defaults to Reject. Backdrop taps cannot approve, and Allow is unavailable after approval while the phone finishes. Confirmed pairing, rejection, timeout, stale/lost service or lifecycle change removes the modal through validated state. It returns to the normal Phone screen; the fallback never invents a model name.

Test first pairing with/without name, long names, invalid payloads, reject/timeout, power-cycle saved reconnect and warnings during the modal. Matching-code approval remains necessary even when the displayed name looks familiar.

Security-initiation regression: Android/nRF Connect may start SMP before the optional name timer fires. In pinned NimBLE, `BLE_HS_EALREADY` (2) means an existing security procedure is active and must be retained. The vehicle now accepts this result for both initial bonding and saved-bond reconnect; real failures still terminate with an explicit numeric diagnostic. Fake-radio tests cover the peer-first ordering, name/no-name paths, rejection, authenticated reconnect and Forget. Actual-device acceptance of this correction remains pending.
