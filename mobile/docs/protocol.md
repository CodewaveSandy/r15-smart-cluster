# Phone protocol: implemented contracts and future gates

Updated: 2026-10-31. This is the single phone/firmware wire-contract authority. Pairing and connected metadata are implemented and their core physical path is user-confirmed. Background service, bidirectional vehicle telemetry, navigation and Wi-Fi map delivery are not implemented. Source codecs and runtime checks must agree with this guide. See [firmware UART/bench](../../docs/development.md#vehicle-ble-uart-bench) for R15P and physical wiring; [development](development.md) for current evidence and the separate Mac bridge.

## Pairing, reconnect and trust

One BLE connection and one saved phone bond. Vehicle advertises `R15 Cluster`; exact prefix filtering is discovery only. User opens Settings → Phone → Pair phone (60 seconds), compares the real six-digit code and approves on both phone and cluster within the 30-second comparison deadline. Reject is initially focused. Preserve leading zeros. No automatic approval, legacy pairing, debug keys or silent bond replacement. Vehicle requires authenticated LE Secure Connections, bonding and a 16-byte key; Android owns phone keys, vehicle owns bond NVS.

Keep the same GATT after success. Saved peers initiate security immediately; no enrollment write/window/new code is needed. Pinned NimBLE `BLE_HS_EALREADY` means preserve the already-active peer security procedure. First-pair and saved-peer timing must not race a second GATT owner. Name writes never prove trust. A BONDED observation alone is not a live link or application authorization; c012 acceptance establishes that endpoint's authenticated acceptance only.

Disconnect retains trust. App Forget clears its selected hint only and explains Android Bluetooth settings plus cluster Forget for both copies; cluster Forget disconnects then removes its bond. No companion-accessible remote Forget command exists. Never use hidden bond-removal APIs, silently erase NVS or auto-pair an asymmetric bond. Numeric link/security/fault diagnostics belong in logs; names, addresses, codes, keys and payloads do not.

## Enrollment name (c011)

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

## Enrollment identity UART (R15N)

R15G v3 is unchanged. R15P v1 connection-identifier extension is specified below. Optional R15N v1 messages carry the label only while numeric comparison is pending. Codec: `shared/protocol/PairingIdentity.h`.

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


## Connected metadata behavior

On each authenticated BLE connection, send the actual phone label, current battery percentage and current UTC time/local UTC offset. The cluster header shows the phone icon and battery text only while a fresh authenticated connected status is available. Settings → Phone shows the transmitted name and `Battery N%`; missing fields show `Phone name unavailable` / `Battery unavailable`. A saved disconnected bond does not imply a live phone or retain its metadata. No navigation capability is introduced.

The existing enrollment name characteristic `813b1240-8fd9-4d67-a53b-9f0a2531c011` remains unchanged. Continue sending it before a first bond for the pairing modal. It does not supply connected metadata and remains forbidden for saved bonded peers.

## Connected metadata (c012)

| Item | Contract |
| --- | --- |
| Service | `813b1240-8fd9-4d67-a53b-9f0a2531c010` |
| Characteristic | `813b1240-8fd9-4d67-a53b-9f0a2531c012` |
| Operation | Write with response, encrypted/authenticated bonded current link, 16-byte key |
| Payload | Exactly 44 bytes below; no JSON or trailing terminator |
| Minimum interval | One accepted write per second; normally send every 30 seconds |
| Expiry | Metadata becomes unavailable after 90 seconds without an accepted update |

All multi-byte integers are little endian. Each accepted payload atomically replaces all three fields; always send the full snapshot.

| Byte offset | Meaning |
| --- | --- |
| 0 | Version = 1 |
| 1 | Name length 0–32; zero means unavailable |
| 2 | Battery 0–100 inclusive; 255 means unavailable |
| 3 | Reserved = 0 |
| 4–7 | Unsigned UTC Unix seconds, 2020-01-01 through 2099-12-31 inclusive; 0 means unavailable |
| 8–9 | Signed int16 local UTC offset in minutes, −720 through +840; must be 0 when UTC is unavailable |
| 10–11 | Reserved = 0 |
| 12–43 | Name: printable ASCII 0x20–0x7E, at least one non-space when present; zero padding after the declared length |

Do not send milliseconds as epoch seconds or apply the offset to the UTC epoch. For India the offset is +330. UTC is phone wall-clock time, not uptime. Use the timezone offset at the instant sampled, including DST where applicable. Use real battery state; 0% is valid and must not mean unknown. Name uses the existing local label policy, never the cluster's advertised name. Full Unicode is not supported by the cluster font/contract.

A valid snapshot with all fields unavailable begins `01 00 FF 00` followed by 40 zero bytes. Reject invalid ranges/lengths locally. Firmware rejects malformed snapshots atomically with ATT invalid value length; disallowed state/rate gets write-not-permitted. Failure does not erase trust, disconnect the link or authorize vehicle commands.

## Android operation ordering

1. Keep the existing first-pair name → numeric comparison flow. For saved peers preserve immediate security/reconnect.
2. Discover the new characteristic on every live connection, including saved reconnect. Negotiate ATT MTU of at least 47 before this write (44-byte value plus 3-byte ATT overhead); serialize MTU/discovery/write operations through real callbacks. Do not interfere with the enrollment name's three-second deadline. An unsupported MTU/endpoint must leave the existing connection functional and report metadata unavailable.
3. Wait for OS bonding/security completion. BONDED alone can precede firmware security completion; if the new write is temporarily refused, retry with bounded delays (for example 1/2/4 seconds) while the same GATT is current. Do not restart pairing or request another enrollment window. A successful authenticated write confirms that this metadata endpoint accepted the secured current connection; it does not establish future navigation/application services.
4. Immediately send a full snapshot after each successful new connection/reconnect. Send every 30 seconds while connected, and on foreground resume, phone time/timezone change or battery change, coalescing events to remain at least one second apart. Use lifecycle-safe Android battery/time observations; handle permission/API limitations explicitly. No new background-service promise is implied.
5. Cancel work, callbacks and queued snapshots on disconnect, Bluetooth loss, selection change or disposal. Guard every callback with the current GATT/generation. Recreate metadata from current phone observations on reconnect; never replay a previous connection's pending snapshot.
6. Older firmware without the endpoint must continue the existing pairing/connection behavior. Do not repeatedly rediscover or fail the connection just because metadata is unsupported.

## Connected metadata UART (R15M)

Both MCUs require new firmware. R15P status version 1 now uses bytes 40–43 for a nonzero random per-connection identifier (zero when disconnected); CRC remains at 44. Older strict displays reject this extension, so update both boards together. Pairing R15N and discrete-input R15G framing are unchanged.

New R15M v1 UART frames are 80 bytes: magic at 0–3; version at 4; zero 5–7; vehicle boot session 8–11; BLE connection identifier 12–15; stream sequence 16–19; source uptime 20–23; the 44-byte payload 24–67; zero 68–75; CRC-32/ISO-HDLC over bytes 0–75 at 76–79. Integers are little endian. Fixed receivers validate framing, CRC, padding, bounds, source session, connection identity, sequence and uptime ordering. Frames expire locally after one second. Metadata is sent at most 5 Hz, separately from status/pairing-name frames; 36-byte input plus 80-byte metadata fits the 128-byte TX FIFO. No extra task or LVGL access is added to BLE callbacks.

Vehicle extrapolates accepted UTC using monotonic elapsed time while metadata is fresh. Display synchronizes its existing clock service from current matching frames; automatic timezone follows the phone offset, manual timezone and persisted date/12-hour preferences remain respected. After disconnect/metadata expiry the last validated clock keeps ticking on display uptime. Cold power still requires a new real sync; no RTC retention or timestamp persistence is claimed. USB bench sync remains available; the most recently received valid source applies.


## Transport policy

BLE is proposed for authenticated setup, capabilities, health, commands/acknowledgements, GPS/maneuvers, notification summaries and media metadata. Wi-Fi is proposed for bounded map chunks, larger route geometry and optional artwork. Selection follows urgency/size/capability, not a hard per-feature split. Avoid dual-path duplicate commands. Audio remains phone/headset.

Investigate vehicle ESP32 private access point with phone joining via OS-approved provisioning. Confirm supported phone cellular internet routing during local Wi-Fi, background behavior and actual ESP32 coexistence. No claim of automatic hotspot toggling or identical iOS setup. See [Android networking](https://developer.android.com/develop/connectivity/wifi/wifi-infrastructure).

Wi-Fi does not remove the UART bottleneck. The current proposed 115200 8N1 link has about 11,520 bytes/s before framing/telemetry. One 800 x 480 RGB565 frame is 768,000 bytes, about 66.7 seconds even without other traffic. Use compact geometry/chunks and measure the whole path; revised baud/framing requires board validation. Preload route corridors and bound cache/transfer work; don't infer performance from host CPU or PSRAM capacity.

## Future telemetry and navigation contract gate

- Major/minor compatibility, explicit message kinds and bounded lengths, byte order, units, enum validity, optional fields and atomic rejection.
- Stable device identity, authenticated session/boot IDs, per-stream sequence/wrap rules and transfer/route generations. Never compare sequence across a reboot as the same session.
- Per-feature capability and quality: available, unsupported, unavailable, invalid, stale; permission denial separately represented. A live heartbeat cannot refresh old GPS/content.
- Source age plus local receipt age, monotonic deadlines and clock-offset uncertainty; do not subtract unsynchronized wall clocks to infer freshness.
- Hello/capability negotiation, full snapshot after reconnect and old-session invalidation. No replay of expired notifications or media commands.
- Request IDs, acknowledgement/rejection, bounded retries/deadlines, idempotency and cancellation. Bind responses to device/session/request, not name or current list index.
- Map region/zoom/projection/coordinate format, attribution/version, chunk offsets/length/integrity, total transfer limits, flow control, resume/expiry and atomic cache activation. Reject old-route chunks after reroute.
- Control versus bulk scheduling, bounded queues, latest-state coalescing, backpressure and observable overflow. Vehicle acquisition/warnings always win over optional map work.
- UTF-8 byte limits and rendering fallback for unavailable glyphs. Caller/sender/track text and notifications have explicit truncation and retention limits.
- Threat model and vetted authenticated encrypted transport. CRC detects corruption and is not authentication.

Maintain language-neutral fixtures alongside the first normative specification. During this development phase preserve language-neutral normative vectors and inspect boundaries manually; defer automated test suites until requested. At the later testing stage Dart and C++ codecs must consume the same positive/negative golden vectors, including oversized/truncated data, unknown versions, corrupt integrity, stale source, duplicate/out-of-order, reboot, reconnect and interleaved transfer cancellation.


## Development bridge boundary

Mac c001/c002 endpoint discovery, TLS bootstrap and HTTP sessions are separate from physical c010/c011/c012. Never apply Mac session success to ESP32 feature readiness. Run and security details are in [development](development.md#mac-preview-adapter).
