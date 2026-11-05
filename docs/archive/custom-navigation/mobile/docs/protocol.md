# Phone protocol: implemented contracts and future gates

**Planned replacement — 2026-11-05:** the [notification guidance design](../../docs/turn-by-turn-plan.md) proposes retiring c016–c01b and adding a new lightweight guidance channel. These new bytes are a proposal pending feasibility/contract freeze; the implemented map contracts below remain present until migration. c011–c015 must remain compatible.

Phase 2 firmware discovery, exact layouts, limits, session/credit rules and normative examples are implemented in the [v2 contract below](#phase-2-navigation-extension--implemented-firmware-contract). Existing v1 frames/reserved bits/UUIDs remain unchanged. Android must target that contract; its source/cache proof and actual integration are still pending.

Updated: 2026-11-02. This is the single phone/firmware wire-contract authority. Pairing and connected metadata are implemented and their core physical path is user-confirmed. Firmware now includes authenticated capabilities/discrete-input telemetry/health endpoints for the background-sync handoff; Android background service/telemetry source integration is implemented; latest APK packaging and physical B1/B2 acceptance remain pending. Phase 1 BLE/UART map firmware is implemented/build and host-preview verified; physical acceptance and Android GPS remain pending. Firmware route-window presentation and quality-gated guidance are implemented through v2; Android v2 and Wi-Fi delivery are not implemented. Source codecs and runtime checks must agree with this guide. See [firmware UART/bench](../../docs/development.md#vehicle-ble-uart-bench) for R15P and physical wiring; [development](development.md) for current evidence and the separate Mac bridge.

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
4. Immediately send a full snapshot after each successful new connection/reconnect. Send every 30 seconds while connected, and on foreground resume, phone time/timezone change or battery change, coalescing events to remain at least one second apart. Use lifecycle-safe Android battery/time observations; handle permission/API limitations explicitly. The Android foreground service now owns this scheduling independently of Activity/Flutter lifetime; physical locked-screen acceptance remains pending.
5. Cancel work, callbacks and queued snapshots on disconnect, Bluetooth loss, selection change or service disposal. Disposing the UI observer does not stop the started Android sync service. Guard every callback with the current GATT/generation. Recreate metadata from current phone observations on reconnect; never replay a previous connection's pending snapshot.
6. Older firmware without the endpoint must continue the existing pairing/connection behavior. Do not repeatedly rediscover or fail the connection just because metadata is unsupported.

## Connected metadata UART (R15M)

Both MCUs require new firmware. R15P status version 1 now uses bytes 40–43 for a nonzero random per-connection identifier (zero when disconnected); CRC remains at 44. Older strict displays reject this extension, so update both boards together. Pairing R15N and discrete-input R15G framing are unchanged.

New R15M v1 UART frames are 80 bytes: magic at 0–3; version at 4; zero 5–7; vehicle boot session 8–11; BLE connection identifier 12–15; stream sequence 16–19; source uptime 20–23; the 44-byte payload 24–67; zero 68–75; CRC-32/ISO-HDLC over bytes 0–75 at 76–79. Integers are little endian. Fixed receivers validate framing, CRC, padding, bounds, source session, connection identity, sequence and uptime ordering. Frames expire locally after one second. Metadata is sent at most 5 Hz, separately from status/pairing-name frames; 36-byte input plus 80-byte metadata fits the 128-byte TX FIFO. No extra task or LVGL access is added to BLE callbacks.

Vehicle extrapolates accepted UTC using monotonic elapsed time while metadata is fresh. Display synchronizes its existing clock service from current matching frames; automatic timezone follows the phone offset, manual timezone and persisted date/12-hour preferences remain respected. After disconnect/metadata expiry the last validated clock keeps ticking on display uptime. Cold power still requires a new real sync; no RTC retention or timestamp persistence is claimed. USB bench sync remains available; the most recently received valid source applies.


## Background-sync capabilities and telemetry (c013–c015)

Implemented in vehicle firmware and integrated in Android source; latest APK packaging and physical B1/B2 acceptance are pending. See [implemented Android ownership/timing policy](background-sync.md#implemented-android-policy-and-remaining-acceptance). Added to the existing service `813b1240-8fd9-4d67-a53b-9f0a2531c010`. c011/c012 and every UART packet remain unchanged. Portable codec: `shared/protocol/PhoneTelemetry.h`; normative non-executable examples: `shared/protocol/phone-telemetry-v1-vectors.txt`.

| Characteristic UUID | Properties | Value / prerequisite |
| --- | --- | --- |
| `813b1240-8fd9-4d67-a53b-9f0a2531c013` | Read | 20-byte R15C capability snapshot, default MTU sufficient |
| `813b1240-8fd9-4d67-a53b-9f0a2531c014` | Read + Notify | 64-byte R15T full snapshot, negotiated MTU >=67 |
| `813b1240-8fd9-4d67-a53b-9f0a2531c015` | Read | 80-byte R15H health snapshot, negotiated MTU >=83 |

All accesses require the current firmware-confirmed authenticated/bonded encrypted link with a 16-byte key and a healthy ready service. ATT read permission flags and callback checks both enforce this; writes to the new values are not supported. c014 notifications additionally check this security state before every send. CCCD enable alone proves no authorization. This is read-only vehicle observation, with no bike control, phone-to-vehicle command, navigation input or map capability. GATT attributes may move after a firmware update: discover by UUID, never hardcode handles; handle Android service-change/rediscovery through the serialized owner, never hidden cache-refresh APIs or automatic trust erasure.

Request MTU 96 (or greater) and use the actual successful callback value. With MTU 67..82 telemetry works but health is unavailable. Below 67, c014 reads fail and notifications are suppressed; existing c012 still works at MTU >=47. c014/c015 intentionally reject nonzero Read Blob offsets and insufficient MTU (invalid offset / invalid value length); do not splice changing snapshots across long reads. Capability reads, telemetry reads/notifications and health reads are separate operations, not chunks of one message. No extra length prefix, JSON or string terminator.

### R15C capability snapshot (20 bytes)

| Bytes | Meaning |
| --- | --- |
| 0–3 | ASCII R15C |
| 4, 5 | Major 1, minor 0 |
| 6, 7 | Maximum active connections 1; reserved zero |
| 8–11 | Vehicle boot session, nonzero; changes on MCU reboot, not merely reconnect |
| 12–15 | Feature bits: bit0 connected metadata, bit1 discrete-input telemetry, bit2 health reads; current mask 7. Higher bits reserved |
| 16–17 | Telemetry value size 64, uint16 |
| 18–19 | Minimum notification interval 200ms, uint16 |

No CRC on R15C; it is read through authenticated ATT. Feature bit1 does not claim speed/RPM/fuel/voltage support or completed app/bench acceptance. Reject unsupported major versions and incompatible lengths; tolerate only explicitly understood capabilities. This boot token is session identity, not a stable permanent device identity or phone pairing proof.

### R15T telemetry snapshot (64 bytes)

Every notification/read is a full latest snapshot. Vehicle sampling remains at its existing ~20ms loop; BLE sends at most once per 200 ms (5Hz), with no replay queue and no change-triggered extra burst. Rewriting CCCD cannot bypass the cadence. No CCCD subscription -> no notifications. Subscribe on every reconnect with Android local notification registration and CCCD `0x0001`, await its write callback, then read c014 for a baseline. A current notification may precede the baseline read callback; apply the same ordering rules to both rather than forcing a delayed read over newer data.

| Bytes | Meaning |
| --- | --- |
| 0–3 | ASCII R15T |
| 4, 5 | Version 1, exact length 64 |
| 6 | bit0 return-display heartbeat fresh; bit1 accepted c012 snapshot fresh; bit2 acquisition source fresh; bits3–7 zero |
| 7 | Reserved zero |
| 8–11 | Vehicle boot session, must match c013 |
| 12–15 | Random nonzero current BLE connection token; changes on a new link |
| 16–19 | Transmit/read sequence, uint32 modulo 2^32; increments for both reads and notifications, gaps allowed |
| 20–23 | Sender uptime at snapshot, ms modulo 2^32 |
| 24–25 | Acquisition age ms: 0..65534 saturated; 65535 means no source yet |
| 26–27 | Seconds since last accepted c012 snapshot in this connection: 0..65534 saturated; 65535 means none yet |
| 28, 29 | Gear quality; gear 0=N, 1..6 when Available, otherwise 255 |
| 30, 31 | Left-turn quality; phase bool 0/1 |
| 32, 33 | Right-turn quality; phase bool 0/1 |
| 34, 35 | High-beam quality; bool 0/1 |
| 36–39 | Speed, RPM, fuel, bike-voltage qualities respectively; all Unsupported (2) in this version |
| 40–47 | Unsupported measurement slots, all zero; never interpret as valid speed/RPM/fuel/voltage |
| 48–51, 52–55 | Boot-scoped Trip/Info press counters, uint32 modulo 2^32; baseline only after connect/recovery, no replayed actions |
| 56–59 | Acquisition sample sequence, uint32 modulo 2^32 |
| 60–63 | CRC-32/ISO-HDLC over bytes 0–59 (same parameters as R15G) |

Quality encoding: Available=0, Unavailable=1, Unsupported=2, Invalid=3, Stale=4. Unknown/stale gear is not N. Non-Available bool values are zero and must not imply inactive. At source age >=1000ms all input qualities become Stale; no source means Unavailable, unknown age/counters zero. Unsupported measurement slots stay zero regardless of source age. Left/right values are observed source phase, not intent; do not blink them a second time. These are configured GPIO bench observations, not newly validated/calibrated bike signals.

Metadata-fresh means a valid c012 snapshot was accepted less than 90 seconds ago, even if its individual fields are unavailable. At exactly 90 seconds it is false; accepted metadata resets its age only, never acquisition age. Display-fresh is specifically the return UART heartbeat, not proof of the entire display/render path or forward UART. Phone telemetry remains valid independently of display loss if acquisition remains fresh. No permanent session secrets, phone identity or payload text are exposed.

Receiver rules: bind every callback to the current GATT generation. Compare boot with capabilities and latch the connection token from the first valid current snapshot; cross-check c015 when read. A token change within one GATT is a session fault requiring refresh/reconnect, not permission to accept an old queued callback. For same boot/connection accept only transmit sequence deltas in 1..0x7fffffff; uptime deltas may be zero (multiple reads in one millisecond), otherwise must be <0x80000000. CRC or malformed frames never refresh anything. Source sequence can remain unchanged briefly; receipt of another read/heartbeat never makes an old acquisition fresh. Apply source age, conservative transport/callback age from the timed baseline below and local monotonic elapsed time: inputs expire at 1000ms total age even while BLE is connected. Also mark the stream stale if no valid packet for 1000ms. Use monotonic time for local freshness, not phone/vehicle wall-clock subtraction. On reboot/reconnect clear the old baseline and require current capabilities/subscription/full snapshot. No backlog replay.

### Bounding delayed delivery with a timed baseline

Receipt age alone cannot detect a burst of old callbacks buffered while Android slept. Establish a conservative mapping between vehicle uptime and Android monotonic time using an authenticated c014 read: record local dispatch `t0`, local callback `t1`, and packet sender uptime `uB`. Use the actual GATT dispatch time, not time spent waiting in the operation queue. For this initial handoff accept a timing baseline only when its round trip is <=500ms (an engineering limit to validate on hardware); longer/failed reads leave data timing uncertain and readings stale until a usable baseline arrives. Do not disconnect/erase trust just because timing confidence is absent.

For a same-session packet with unwrapped sender uptime `uP`, a conservative total source-age bound at local monotonic time `t` is `sourceAge + max(0, (t - t0) - (uP - uB))`, plus timestamp-quantization and measured clock-drift uncertainty. The earliest possible local send time is `t0 + (uP - uB)`, so this includes the baseline RTT uncertainty, packet transport/Android callback delay and subsequent local age. Do not use wall-clock subtraction or assume phone/vehicle clock rates are identical. Reject backwards/ambiguous uptime and sequence; short uptime deltas of zero remain valid for reads in the same millisecond. Refresh the timed c014 baseline at least every 30 seconds through the serialized queue; if it expires, timing becomes uncertain/stale until refreshed. Account for measured drift over each baseline interval in the freshness margin, and report unexplained drift/delay during B1 rather than declaring fresh data from receipt alone.

Until a baseline exists, decoded notifications can update diagnostics but must not assert fresh input. Retain at most the newest pending snapshot, never a backlog. A notification newer than the read may arrive before its callback: establish the time anchor from the read without overwriting a newer valid snapshot, then apply the conservative age bound to that newest snapshot. Baseline refresh is clock-confidence maintenance, not a second measurement stream; delayed read results cannot rewind sequence/state. Inputs expire when the conservative total source-age bound reaches 1000 ms or no valid packet has arrived for 1000 ms, whichever is earlier. Metadata/link freshness remains independent.

### R15H health snapshot (80 bytes)

Diagnostic reads are optional, recommended every 10 seconds during B1/B2 only; do not poll per frame. No application notification queue is added. NimBLE `NOTIFY_TX` in the pinned stack reports the attempt result synchronously, not Android receipt or radio-level delivery. One pending-call guard plus bounded NimBLE buffers prevents an application backlog; allocation/send failures count errors, drop that sample and retry a new latest sample at the next cadence. A missing result leaves further notifications blocked, observed as stream expiry; explicitly reconnect rather than creating competing GATT calls.

| Bytes | Meaning |
| --- | --- |
| 0–3, 4, 5 | ASCII R15H; version1; length80 |
| 6–7 | Reserved zero |
| 8–11, 12–15, 16–19 | Boot token, current connection token, sender uptime ms |
| 20–23 | Accepted c012 writes |
| 24–27 | Metadata writes refused by application state/security checks |
| 28–31 | Malformed c012 payloads reaching callback |
| 32–35 | Rate-limited c012 writes |
| 36–39 | c012 expirations |
| 40–43, 44–47 | Successful physical GATT connects; disconnect events (not authenticated session counts) |
| 48–51 | Notification calls accepted by NimBLE |
| 52–55 | Successful NimBLE notify-attempt callbacks; not phone acknowledgements |
| 56–59 | Allocation/send/result/stall errors; not an exact RF loss count |
| 60–63 | Maximum gap ms between accepted metadata writes within a connection |
| 64–67 | Maximum BLE host work-service gap ms |
| 68–71 | Maximum acquisition loop sample gap ms |
| 72–75 | Reserved zero |
| 76–79 | CRC-32/ISO-HDLC over bytes 0–75 |

Counters/maxima are boot-scoped and saturate at uint32 max; reconnect does not clear them, reboot does. Maximum metadata gap excludes time before the first accepted write and disconnect downtime, but includes a >90-second lapse followed by a new accepted write. Current age in c014 exposes an ongoing lapse before it closes. Errors rejected by NimBLE security/ATT before callbacks are not counted in application rejection counters. Observe deltas against each current boot baseline, not absolute expected zeros across unrelated fault exercises. Maxima update when their service/sample resumes. No heap/resource-performance claim follows from these counters.

### Physical acceptance and compatibility

Old app c011/c012 behavior remains supported; no new endpoint is required just to retain that flow. New app must gracefully mark telemetry/health unsupported on older firmware while preserving existing connection/metadata. On the current firmware, validate capabilities before enabling new decoding. The app's Home gear may now use a real Available observation; speed/fuel/bike voltage remain Unsupported, not 0/N/phone battery/GPS substitutions. Broader navigation/GPS/map and Wi-Fi bits are absent; do not expose them as ready.

B1 requires 60-minute untethered locked-screen bidirectional activity: accepted c012 counts/age plus current c014 packets/source age and health counter deltas. B2 adds real bench input changes, invalid/no gear, source/stream expiry, old/duplicate/malformed rejection, notification unsubscribe/resubscribe, board independent resets, reconnect and concurrent UI/input load. Physical results, Android service lifecycle and resource measurements remain pending; builds and normative examples are not acceptance.

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

## Phase 1 map extension — implementation specification

**v1 firmware implementation; physical acceptance pending.** Codecs, display store/page and authenticated vehicle bridge are implemented. Build/preview evidence and remaining hardware gates are in [development](../../docs/development.md#phase-1-map-firmware-integration). Inspect the normative vectors and complete board acceptance before Android work. [Phase 1 handoff](../../docs/map-phase-1.md) owns the workflow, UI, location policy, resources and acceptance. This section owns the implemented v1 bytes. Existing c011–c015, including c013's strict feature mask/version, and existing UART frames are unchanged. No navigation/route/ETA fields exist in this extension.

All multibyte integers below are little-endian; signed values use two's complement. CRC is CRC-32/ISO-HDLC (reflected polynomial `0xEDB88320`, initial/final XOR `0xFFFFFFFF`; check ASCII `123456789` → `0xCBF43926`). SHA-256 is 32 raw bytes in standard digest order. Reserved fields/bits must be zero. Unknown versions, enum values, invalid lengths and noncanonical fields are rejected before application. CRC is corruption detection, not authentication.

### GATT endpoints and downstream readiness

Same c010 service and UUID prefix `813b1240-8fd9-4d67-a53b-9f0a2531`: c016 capability READ, c017 command WRITE WITH RESPONSE, c018 status READ + NOTIFY (CCCD). Each suffix means the full UUID ending `c016`, `c017` or `c018`. All operations, including CCCD, require the current authenticated LE Secure Connections bond and 16-byte key. Never allow enrollment/unsecured map commands. No second connection/GATT owner or write-without-response path.

Negotiate MTU247 before reading c016; actual MTU must be ≥96 for all three map characteristics. c016/c018 reject Read Blob offsets rather than splicing changing snapshots. c017 value limit is `min(240, actualMTU - 3, advertisedMaxValue)` bytes. Do not assume requested MTU succeeded. Subscribe c018 and read a baseline before BEGIN; bounded status reads recover coalesced/lost notifications through the same serialized ATT queue. The vehicle expires a downstream status after2000ms and never substitutes a fabricated display timestamp/ACK. One pending status snapshot is sufficient; status never queues a history of fixes. Missing ATT callback retains the existing eight-second close-before-recovery rule.

`R15K` capability is exactly 64 bytes:

| Offset | Field |
| --- | --- |
| 0 | ASCII `R15K` (4 bytes) |
| 4,5 | Version u8=1, length u8=64 |
| 6,7 | Flags u8: bit0 map-ready, bit1 timed-location supported, bit2 location supported; reserved u8=0 |
| 8,12,16 | Vehicle boot u32, current BLE connection token u32, display boot u32 |
| 20 | Maximum complete package bytes u32, ≤65536 |
| 24,26,27 | Maximum roads u16 ≤2048; maximum points/road u8=8; maximum name bytes u8=23 |
| 28,30 | Maximum c017 value u16 ≤240; minimum MTU u16=96 |
| 32,34 | Bulk encoded-UART bytes/sec u16 ≤4096; minimum location send period ms u16=500 |
| 36,38 | Location TTL ms u16=2000; maximum horizontal accuracy cm u16=5000 |
| 40–59 | Reserved zero |
| 60 | CRC32 of bytes 0–59 |

Flags are readiness, not evidence of a live fix. Readiness requires fresh downstream capability (≤1500ms), supported display firmware and successful active/staging reservation. Unavailable resources clear flags; never promise the host's memory ceiling. Advertised positive resource ceilings may be lower; an unsupported package fails before transfer, without dropping roads. On BLE, vehicle/connection IDs are current/nonzero; display ID can be zero only when unavailable. Every new connection rereads capability. A display reboot or expired return heartbeat immediately clears readiness and invalidates pending transfer/time state.

Display sends this same capability record through the UART tunnel once/second and after resource/boot changes. It reports its own nonzero boot ID and current vehicle ID learned from legacy input; connection token may be zero before phone connection. Such UART announcements describe resources only and never authorize a phone. The vehicle verifies the current display/return-link context and fills its own current boot/secured BLE token for c016; the display still independently checks each command against current legacy vehicle/phone session. Unknown/missing session state rejects commands. No hardcoded true-ready capability.

### Complete regional package: R15B v1

Header exactly 64 bytes, followed by variable road records; total ≤advertised maximum and ≤65536. No trailing bytes, compression, native structs or route data.

| Offset | Field |
| --- | --- |
| 0 | ASCII `R15B` |
| 4,5,6 | Major u8=1, minor u8=0, header length u16=64 |
| 8,12 | Origin latitude E7 i32, longitude E7 i32 |
| 16,20,24,28 | South, west, north, east E7 i32 |
| 32,34 | Road count u16 (1..advertised maximum), flags u16=0 |
| 36 | Body byte count u32 = exact total minus64 |
| 40 | OSM source UTC seconds u32; dataset provenance, never GPS age |
| 44 | CRC32 of complete body |
| 48–63 | Reserved zero |

Road record: kind u8 (`0 Local, 1 Arterial, 2 Water, 3 Path, 4 Service`), point count u8 (2..8), name length u8 (0..23), reserved u8=0; then exactly name-length printable ASCII bytes `0x20..0x7E` (no padding/NUL), then point-count pairs of east i16/north i16 metres. Require a non-space character for a nonempty name, and at least two distinct points. Points must be within ±20000m and the geographic bounds within rounding tolerance; count/length arithmetic must be overflow-safe and exhaust the body exactly.

WGS84 coordinates: latitude within ±85°, longitude within ±180°, south<north/west<east, origin inside bounds; no antimeridian crossing. Bounds must project no more than5000m from origin in either axis. Projection: `east = R*cos(originLatRad)*(lonRad-originLonRad)`, `north = R*(latRad-originLatRad)`, R=`6371008.8m`; nearest integer with exact half away from zero. Allow1m projected boundary rounding tolerance. Both producers/consumers use these rules; never reproject old metre coordinates under a new origin. Full package SHA-256 is carried by BEGIN. Manifest/provenance/ODbL notices accompany distribution but are not arbitrary UART text.

### c017 command: R15X v1

Header32 + body + CRC32, total `36 + bodyLength`:

| Offset | Field |
| --- | --- |
| 0 | ASCII `R15X` |
| 4,5,6,7 | Version u8=1; type u8; header length u8=32; flags u8=0 |
| 8,12,16 | Current vehicle boot, BLE connection token, display boot (u32 each, nonzero) |
| 20 | Map generation u32; nonzero for BEGIN/DATA/COMMIT/CANCEL, zero for other types |
| 24 | Request sequence u32, nonzero |
| 28,30 | Body length u16; reserved u16=0 |
| 32 | Body, then CRC32 over header+body |

| Type | Value | Exact body |
| --- | --- | --- |
| TIME_REQUEST | 1 | Phone dispatch elapsed milliseconds T0 u64 |
| TIME_CONFIRM | 2 | TIME_REQUEST sequence u32 + echoed T0 u64 + phone callback elapsed milliseconds T1 u64 (20 bytes) |
| BEGIN | 3 | Complete package length u32 + complete SHA-256 (36 bytes) |
| DATA | 4 | Contiguous byte offset u32 + 1..200 package bytes, reduced for actual MTU/advertised value |
| COMMIT | 5 | Empty |
| CANCEL | 6 | Empty |
| LOCATION | 7 | Location body below (40 bytes) |
| STOP | 8 | Empty |

Control requests use one increasing sequence stream, LOCATION another. Unsigned forward delta must be1..`0x7FFFFFFF`; use a new nonzero sequence across wrap. Byte-identical retry of the immediately last control request is idempotent and returns its current status; same sequence with different bytes fails. Duplicates never refresh source age or transfer inactivity. Location duplicates/backward packets are discarded. Nonidentical control requests wait for prior display acknowledgement; LOCATION can interleave/coalesce and does not overwrite last-control ACK. Cancel/STOP wins over queued DATA. Generations do not repeat in a connection. On boot/reconnect all sequence/anchor/staging context is invalid; old IDs cannot revive data.

BEGIN is accepted only if idle/previous transfer cancelled and resources/length are valid. A busy different BEGIN reports BUSY, preserving staging. Only display acceptance of a DATA offset advances `nextOffset`; identical retry is harmless, overlapping/conflicting/out-of-order data fails without partial activation. COMMIT requires all bytes and validates SHA, package CRC, fields and decoded scene before atomic activation. Validation is bounded/incremental outside LVGL draw callbacks. BEGIN matching the full stored validated active SHA may return ACTIVE without retransmission, rebinding that content to the new generation/session. CRC alone cannot authorize reuse. CANCEL discards the matching staging generation, retaining active map. STOP clears staging/timing/location and marks sharing stopped, retaining active roads. A fresh TIME_REQUEST after STOP starts a new sharing session; no old fix is eligible.

Location body (all offsets relative to body):

| Offset | Field |
| --- | --- |
| 0,8 | Fix elapsed ms u64; actual phone dispatch elapsed ms u64 |
| 16,20 | Latitude E7 i32; longitude E7 i32 |
| 24 | Horizontal accuracy cm u32 (`0xFFFFFFFF` unknown) |
| 28,30 | Course centidegrees u16 (0..35999); course accuracy centidegrees u16 (0..18000); each `0xFFFF` unknown |
| 32 | Speed cm/s u16 (0..65534), `0xFFFF` unknown |
| 34 | Flags u8: bit0 precise permission, bit1 OS mock fix, bit2 accuracy limited; others zero |
| 35 | Quality u8: 0 Unavailable, 1 Available, 2 Stale, 3 Invalid, 4 Unsupported |
| 36 | Reserved u32=0 |

Available requires valid coordinates/fix≤dispatch, precise flag, nonmock source and known accuracy≤5000cm; source age must already be<2000ms at dispatch. Latitude/longitude zero is valid when Available. Non-Available uses fix=0, coordinates=0, accuracy=`0xFFFFFFFF`, unknown course/course-accuracy/speed; dispatch still contains actual elapsed time. Non-Available promptly removes live marker but never refreshes it. Accuracy-limited observations use quality3 Invalid plus flag bit2, with canonical unknown position fields; bit0 still reports precise access. Bit2 is forbidden with mock/Available/other qualities. The display reports LOW_ACCURACY for this case; other Invalid observations report INVALID. Receiver rechecks age; Available alone cannot prove freshness. Direction arrow additionally requires known course/accuracy≤4500 and speed≥200cm/s; otherwise a neutral position dot. Phone speed never supplies vehicle speed. Reject malformed/unsupported combinations; stale valid packets cannot refresh a marker.

### c018 display status: R15S v1

Exactly80 bytes; read/notify convey the same latest authoritative display snapshot. Before a valid downstream status, return state UNSUPPORTED with zero display fields; vehicle must not invent accepted offsets/activation. A local ingress failure may return an ATT error but cannot advance display ACK. Connection-stale cached status cannot be relabelled as current.

| Offset | Field |
| --- | --- |
| 0 | ASCII `R15S` |
| 4,5,6,7 | Version1, length80, transfer state, error (u8 each) |
| 8,12,16 | Vehicle boot, BLE token, display boot (u32 each) |
| 20,24 | Current/last transfer generation u32; last acknowledged control sequence u32 |
| 28,32 | Next contiguous display-accepted offset u32; declared total bytes u32 |
| 36 | Active map generation u32, zero if none |
| 40 | Current display elapsed ms u64 |
| 48,56 | Echoed phone TIME_REQUEST T0 u64; display elapsed ms DB sampled when handling that request u64 |
| 64 | Last accepted location sequence u32 |
| 68,69 | Location state u8; command credit u8 (0 or1) |
| 70–71 | Reserved zero |
| 72 | Active package body CRC32 u32, diagnostic only; zero if none |
| 76 | CRC32 of bytes0..75 |

Transfer state: `0 UNSUPPORTED, 1 EMPTY, 2 RECEIVING, 3 VERIFYING, 4 ACTIVE, 5 FAILED, 6 CANCELLED`. FAILED/CANCELLED may retain an active map (`activeGeneration` nonzero); transfer state never alone determines visible coverage. Errors: `0 NONE, 1 MALFORMED, 2 UNSUPPORTED, 3 SESSION, 4 RESOURCE, 5 BOUNDS, 6 OFFSET, 7 HASH, 8 TIMEOUT, 9 BUSY, 10 TIMING`. Location state: `0 NO_ANCHOR, 1 NO_FIX, 2 FRESH, 3 STALE, 4 LOW_ACCURACY, 5 OUTSIDE_COVERAGE, 6 STOPPED, 7 INVALID`. A source that cannot qualify may be reported non-Available; UI can use phone permission observations for finer explanation. Credit1 authorizes one control request, never unlimited queued packets; STOP/CANCEL may preempt queued work even with credit0; LOCATION is rate-limited separately. ACK offset progress is received bytes/total, not “map ready”; VERIFYING follows100%, ACTIVE follows full validation and publication. Status updates on transitions/ACK and at least once/second while the link is healthy, coalesces location-only changes to≤2Hz, and is read on a2s map-ACK timeout before byte-identical retry. Transfer inactivity15s measures newly accepted bytes only.

### End-to-end location time anchor

Phone records T0 at actual TIME_REQUEST dispatch (not enqueue). Display records DB when it handles the request, returns echoT0/DB and ACK sequence. Phone records T1 at the matching status callback. Require current IDs, matching pending request, nondecreasing phone elapsed time and `0≤T1−T0≤500ms`. TIME_CONFIRM supplies request sequence/T0/T1; display accepts only its matching pending request within2s of DB. Replay/conflicting confirm fails. Refresh every25s; anchor expires30s after DB. A retry is byte-identical and cannot manufacture a newer DB for an old request.

Accepted anchor `(DB,T1)` intentionally uses the phone callback upper bound. For a fix F and current display time D, conservative age is `max(0, (D−DB) + (T1−F)) + uncertainty`, evaluated with checked signed wide arithmetic (never unsigned underflow). This upper-bounds age because callback occurred after the display sample. Initial uncertainty≥2ms + ceil((D−DB)*1000/1000000), enlarged for observed successive-anchor disagreement; provisional drift requires hardware measurement. If monotonic continuity/uncertainty cannot be established, hide marker. Require conservative age<2000ms AND time since receipt of the last valid newer location<2000ms, fresh link/session/anchor, eligible quality/accuracy and active coverage. Never chain two independent receipt TTLs into a four-second “live” fix. UTC/c012 timestamps are unrelated to this calculation.

### UART tunnel: R15Q v1

Mandatory bounded demultiplexing prevents arbitrary map bytes being parsed as legacy frame magic. Preserve legacy packets byte-for-byte; replace independent parallel byte-feeding with one demultiplexer. Recognized legacy frames follow their existing fixed-length checks; a recognized R15Q record is isolated, decoded or dropped as a whole. Never feed its payload back into legacy parsers, including on CRC failure.

Wire record: ASCII `R15Q`, escaped inner fragment, zero delimiter. Inner fragment: version u8=1; direction u8 (`1 vehicle→display`, `2 display→vehicle`); message sequence u16; complete inner-message length u16 (1..240); fragment offset u16; fragment length u8 (1..48); three reserved zero bytes; payload; CRC32 over those12 header bytes+payload. Message sequence is transport-local, independent of R15X. Fragment offset/length must stay inside total; assembly is contiguous, identical duplicates only, with a fixed240-byte destination and timeout2s. Fragment record timeout500ms, maximum decoded64 bytes, maximum wire133 bytes including prefix/delimiter. Overflow/malformed record is discarded to delimiter with bounded work; no unbounded scan/queue.

Escape every byte after prefix, including header/CRC: raw `0x00`, `0x52` (ASCII R), `0x7D` become `0x7D` followed by byte XOR `0x20`; all others literal. Only those canonical escape pairs are valid. This excludes embedded legacy magic from an encoded payload. Sender never interleaves bytes of two records. Fragments of different message sequences may interleave only into explicitly bounded assembly slots: at most one control/map and one latest LOCATION vehicle→display; one status and one capability display→vehicle. Identify slot from first fragment's inner magic/type before allocation; later fragments require matching direction/sequence/total/offset. No heap allocation from arbitrary totals. Full inner CRC/format/session must also pass after reassembly.

Inner records are R15X commands, R15S statuses or R15K capability announcements above. Direction/type mismatch is invalid. Vehicle does not ACK map bytes before display acceptance. Timeout/drop retries are the original full command, not cross-session fragment resume. Serialize UART writes in one bounded nonblocking owner: legacy acquisition/status/control first, timed LOCATION/control next, map DATA last. Bulk limit4096 encoded UART bytes/sec, ≤one low-priority fragment ahead of critical traffic; current RX-ring/flush limits must be refactored for this framing. No baud/pin change is authorized. Measure acquisition/return-path latency and heap/stack headroom under sustained transfer before asserting readiness. [Firmware implementation/acceptance order](../../docs/map-phase-1.md#6-firmware-first-implementation-order).

Implementation clarifications from the proposed handoff: negotiate MTU before the64-byte c016 read; explicit accuracy-limited flag bit2 lets a non-Available observation explain LOW_ACCURACY without sending a false precise position; periodic display status keeps readiness truthful while idle. These are part of this first implemented v1 and must be reflected in the Android decoder. Normative non-executable examples: [map-phase-1-vectors.txt](../../shared/protocol/map-phase-1-vectors.txt).


## Phase 2 navigation extension — implemented firmware contract

Frozen for the firmware-first implementation on 2026-11-04 at the user's request. Firmware no longer waits for the phone cache spike to define its bytes. The phone still must prove its canonical source/cache adapter and implement this encoder; no MapLibre cache API or completed Android integration is assumed. Existing c011–c018/v1 bytes and security requirements are unchanged. Firmware implementation: `shared/protocol/NavigationProtocol.h`, `shared/application/NavigationStore.h` and `NavigationBridge.h`. Static [normative examples](../../shared/protocol/navigation-v2-vectors.txt) accompany this specification; these are data, not automated tests.

### Discovery, security and resources

Same c010 service and UUID prefix `813b1240-8fd9-4d67-a53b-9f0a2531`:

| Endpoint | Operation |
| --- | --- |
| c019 | READ 64-byte R15J capability |
| c01a | WRITE WITH RESPONSE R15V commands, no write-without-response capability |
| c01b | READ current R15Y STATUS; NOTIFY STATUS, INTEREST and ROUTE_NEED |

All access/CCCD requires the current authenticated LE Secure Connections bond, encryption and 16-byte key. No enrollment bypass, second GATT owner, automatic bond replacement or unauthenticated navigation path. Minimum MTU96. Maximum ATT value `min(240, actualMTU−3)`; MTU96 allows49 DATA bytes, MTU247 allows196. Serialize ATT procedures but pipeline up to the advertised number of downstream-pending DATA commands. A successful ATT write is vehicle admission only.

Advertised encoded-object ceiling32,768 bytes; twelve verified raw-object slots plus one staging slot; four requested tile keys and at most four pending controls;512 road chunks/cell, eight points/chunk,23-byte ASCII road labels;512 points/route object and2,048 aggregate roads/render snapshot. Display map arena is966,712 bytes on this ABI, including the two v1 binary buffers, two shared204,828-byte render scenes and thirteen32KiB raw buffers. It is reserved in PSRAM only; failed allocation advertises unavailable navigation without affecting instruments. V1 and v2 share render snapshots under the single display application/LVGL owner. They cannot concurrently mutate/render the shared arena. No map/GPS/route NVS persistence, assumed SD storage or full arbitrary corridor in ESP32 RAM.

### Primitive rules and R15J capability

Little-endian integers; signed fields two's complement. All reserved bytes/bits zero; unknown version/type/enum/length is invalid. CRC32/ISO-HDLC covers all preceding bytes, including header; CRC occupies the last4 bytes. Object SHA256 covers the complete R15O bytes including its header/body CRC. CRC is not authentication. Full SHA256 identity, not a CRC or short source ID, authorizes reuse.

| Offset | R15J field |
| --- | --- |
| 0..3 | ASCII R15J |
| 4,5 | Version2; length64 |
| 6,7 | Feature flags:0 unavailable or15 (cells/routes/quality-bearing locations/camera); maximum command window4 |
| 8,12,16 | Vehicle boot, current secured connection token, display boot; u32 each |
| 20,24 | Total reserved map arena bytes; maximum encoded object32,768; u32 each |
| 28,29 | Verified cache slots12; maximum interest keys4; u8 each |
| 30,32,34 | Max roads/cell512; aggregate roads2,048; points/route object512; u16 each |
| 36,38 | Max command value240; minimum MTU96; u16 each |
| 40,42,44,46 | Encoded bulk UART cap4,096 B/s; live freshness2,000 ms; anchor lifetime30,000 ms; last-known limit10,000 ms; u16 each |
| 48,52 | Supported style bitmask1 (style1); attribution bitmask3 (IDs1 and2); u32 each |
| 56,57,58..59,60..63 | Maximum selected working cells6; halo targets2; reserved; CRC32 |

The vehicle rewrites the current secured connection token and reseals its capability. Missing/stale display resources return flags0. Capability freshness≤1,500 ms and current return-path health are required for command admission. Never treat presence of a characteristic as readiness.

### Common R15V/R15Y envelope

| Offset | Field |
| --- | --- |
| 0..3 | R15V command or R15Y reply |
| 4,5,6,7 | Version2; opcode; header length40; reserved0 |
| 8,12,16 | Current vehicle/secured connection/display IDs; u32 each |
| 20,24 | Navigation session u32; sequence u32 |
| 28,32 | Object ID u32; byte offset u32 |
| 36,38 | Body length u16; reserved u16=0 |
| 40.. | Body,0..196 bytes |
| final4 | CRC32; total length44+body length |

All command context IDs/session/sequence are nonzero. Object ID is nonzero only for BEGIN/DATA/COMMIT/CANCEL/HAVE; offset is nonzero only for DATA. Object ID is a session-local handle, not content identity, a tile generation or route revision. STATUS uses object/offset for current object and cumulative accepted prefix. INTEREST/ROUTE_NEED use their independent request-generation sequence, object/offset0.

OPEN has control sequence1 and starts a nonzero navigation session. Sessions advance by unsigned forward delta1..0x7FFFFFFF within a secured connection; reconnect may start a new counter because the connection token changed. Retired sessions never restart through an old OPEN. Subsequent controls use exact increment, skipping0 on wrap. LOCATION and PROGRESS each have a separate forward sequence stream and monotonic source-sample validation. STOP, END and CANCEL may jump over cancelled queued control sequences and win over queued DATA. No other out-of-order control consumes the expected sequence; STATUS returns SEQUENCE and the last accepted sequence.

The last four byte-identical controls are idempotent retries. Conflicting same-sequence bytes are rejected. A retry cannot renew source age or DATA inactivity. Semantic failures of an otherwise correctly ordered control consume its sequence and report an error: correct the request using the next sequence. A missing/corrupt control does not consume a sequence: retransmit it and the bounded pending window in order. The vehicle retains up to four full commands until a matching downstream cumulative ACK; it never invents an ACK. Inflight duplicates reuse the slot; an acknowledged retained duplicate returns current status without new admission. No CRC-only or cross-session partial resume.

### Commands and bodies

| Opcode | Name | Body |
| --- | --- | --- |
| 1 | OPEN | 40 bytes: full source descriptor SHA256 at0; style u16 at32=1; attribution u16 at34=1 or2; requested window u8 at36=1..4; reserved37..39 |
| 2 | TIME_REQUEST | Phone dispatch monotonic ms u64 T0 |
| 3 | TIME_CONFIRM | Request sequence u32; T0 u64 at4; callback T1 u64 at12 |
| 4 | BEGIN | Total object length u32 at0,64..32,768; full object SHA256 at4..35; object kind u8 at36=1..3; reserved37..39 |
| 5 | DATA | 1..196 raw object bytes (reduce for actual MTU); prefix offset in envelope |
| 6 | COMMIT | Empty |
| 7 | CANCEL | Empty; matching object only |
| 8 | LOCATION | 40 bytes, below |
| 9 | ACTIVATE | Route ID u64 at0; revision u32 at8; cached overview object ID u32 at12; detail-window ID u32 at16; current-area cell ID u32 at20; all nonzero |
| 10 | PROGRESS | 40 bytes, below |
| 11 | STOP | Empty: stop cluster sharing/source/anchor/guidance; keep complete cached roads/route preview |
| 12 | HAVE | Full object SHA256 at0..31; length u32 at32; reserved36..39 |
| 13 | END | Empty: stop sharing, remove active route/guidance, retain complete map cells |

Source descriptor bytes are producer-defined canonical provenance/configuration, hashed as SHA256; include provider/dataset/snapshot identity, projection/geometry profile, style/attribution and conversion version. Cache identity changes when that descriptor changes. Firmware treats this digest as opaque, never as a MapLibre SDK-cache handle. `sourceId` in object/interest headers is its first4 bytes read little-endian; the full source digest controls cache lifetime, so a short-ID collision does not permit cross-source reuse. Attribution1 means OpenStreetMap contributors/ODbL;2 additionally means OpenMapTiles/OpenFreeMap. Do not claim provider provenance for geometry taken from a different source. Style1 supports bounded roads/water polylines and Latin labels, not raw MVT/raster, arbitrary Unicode glyph downloads or a complete land-use renderer.

OPEN clears anchors, fixes, source sequences, progress and active route, discards partial staging and rebuilds the snapshot from compatible cached cells. A changed full source digest or attribution invalidates all cached raw objects. A cached full SHA/length match in BEGIN returns HAVE with accepted offset=length, after kind/handle validation; COMMIT need not retransmit it. HAVE queries can bind a known complete hash to a fresh object ID. No hit returns IDLE/offset0. A handle already bound to a different cached object conflicts. CANCEL can also unpin a complete candidate route object by its bound ID; it cannot remove the active route geometry. HAVE/new BEGIN while receiving/verifying another object reports BUSY; a matching BEGIN during RECEIVING only reports the accepted prefix and does not renew its timeout.

DATA must append to the accepted prefix. Exact already-accepted bytes are harmless but do not renew inactivity; wrong/conflicting offsets report OFFSET without publishing or modifying that prefix. STATUS's offset is the first missing byte, so retry/recover that contiguous suffix. This implementation intentionally uses a cumulative prefix rather than an arbitrary sparse range bitmap. COMMIT requires complete bytes and incrementally checks SHA256, body CRC, every header/geometry/label/count, then atomically publishes the complete object. Invalid data never draws. Hash/decode work is≤512 bytes/16 records per display service cycle; scene rebuild is≤16 roads/32 route points per cycle. Staging times out after15s without new DATA; old complete objects remain.

Cache eviction chooses an unpinned least-recently-used slot. Active and candidate route objects, at most six selected working cells and cells referenced by an in-progress snapshot build are protected. Target-LOD cells take priority, with bounded coarser/finer fallback; obsolete zoom levels are retired once suitable target cells arrive. Active render snapshots own copied geometry and remain intact while raw-cache selections change, so retiring an old raw cell cannot leave a dangling visible pointer. Replacing the same tile cancels an in-progress staging snapshot first. Publication copies validated geometry into the inactive render scene; a pointer swap occurs only at completion on the LVGL owner. Resource pressure reports RESOURCE without evicting live route/current-area context. The phone must honor aggregate geometry/capacity, send useful current-area content first, keep route windows bounded and avoid flooding remote cells. The full corridor remains on the phone.

ACTIVATE is the route-revision barrier: overview and window must be complete, same route ID/revision/source/style, and the supplied complete current-area cell must cover the eligible fix or current camera when no fix is eligible. Otherwise NOT_READY retains the old route. Route revision is a navigation-session counter advancing by unsigned forward delta even when the route ID changes; an equal revision may only update windows of the same route. Older revisions and conflicting route IDs at the same pending revision cannot activate. New validated route-revision candidates suppress old confident guidance until activation; new route metadata never rides on old geometry. Before beginning an intentional reroute, the phone should also publish PROGRESS quality0 immediately. Activation clears old progress and requires a matching new observation. Each subsequent detail-window replacement uses ACTIVATE with the existing overview/revision plus the new complete window/current cell. A partially downloaded route window is never used.

### R15O object and projection

Every object is64-byte header + body. SHA256 covers the full object; its embedded body CRC covers bytes64..end only.

| Offset | Field |
| --- | --- |
| 0..7 | R15O; version2; kind1 cell/2 overview/3 route window; header64; cell flag bit0=geographically partial (all other bits0; route flags0) |
| 8,12,14 | Source ID u32; style u16=1; attribution u16=OPEN attribution |
| 16,24 | Route ID u64; route revision u32 |
| 28,29,30 | Semantic display-cell z u8; reserved u8=0; record/point count u16 |
| 32,36 | Semantic display-cell x,y; u32 each |
| 40,44 | Route-window start/end along-route metres; u32 each |
| 48,52 | Route total estimated distance metres / duration seconds; u32 each |
| 56,60 | Body CRC32; reserved u32=0 |

Cell: route/revision/range/distance/duration fields all0; z0..22; x,y<2^z; count0..512. Empty complete cells are legitimate known coverage. A clipped regional extract/corridor may set geographic-partial bit0: its fully received, verified geometry can render, but it never counts as full tile/detail coverage or a complete INTEREST HAVE hit. This is unrelated to a partially received object, which never renders. A partial replacement cannot downgrade an already complete tile. Cell body contains exactly `count` roads, each: kind u8 (local0/arterial1/water2/path3/service4); point count u8 2..8; name length u8 0..23; reserved u8=0; printable ASCII name bytes, no terminator and no all-space nonempty name; then `pointCount` pairs u16 u,v. Each coordinate0..4096 inclusive; at least two distinct points/road. Clip/split at tile borders and split long polylines with shared endpoints; never silently truncate a dense tile. Simplification/Latin transliteration is the phone encoder's documented style profile.

Projection is standard Web Mercator, radius6,378,137 m, world width W=2πR. Display-cell z/x/y follow slippy-map indexing: x wraps modulo2^z, y is bounded, origin northwest. A point's world metres are `east=−W/2+(x+u/4096)W/2^z`, `north=W/2−(y+v/4096)W/2^z`. Valid geographic samples are E7 latitude±850,511,287 and longitude±1,800,000,000. Projection uses `east=R·lonRadians`, `north=R·log(tan(π/4+latRadians/2))`; clamp to the Mercator latitude limit, wrap east to[−W/2,W/2). Shared borders use exactly the same extent4096. These are semantic display cells, not a demand that the upstream provider expose the same raw tile zoom. Clamp raw downloads to TileJSON/source zoom limits; derive a requested child by clipping/resampling its canonical cached parent when overscaling. Include that conversion/LOD policy in the source descriptor. Geometry-profile completeness does not promise more detail than the upstream source supplies, and must not trigger repeated404 downloads above its maximum zoom. The render snapshot has a geographic local origin; world deltas wrap to the nearest east displacement and round to integer metres (half away from zero). Metres here are projected units; on-screen ground scale/accuracy circles account for latitude cosine. No equirectangular v1 region assumptions leak into v2.

Route objects: tile z/x/y all0; nonzero route/revision;2..512 points; body is exactly count pairs of signed latE7/lonE7 i32. Overview range is0..totalDistance; window has start<end≤totalDistance. Distance1..100,000,000 m and estimated duration≤31,536,000 s. Split detailed routes into overlapping bounded windows instead of truncation. The overview may be deliberately simplified but retains real endpoints; phone owns fidelity and estimated travel values. Firmware renders the overview and current detailed window together, with the destination taken from the full overview, and skips wrap-discontinuous line segments. It does not calculate a route, road snap, speed, odometer, off-route decision or ETA from bike data.

### Source, progress and time

LOCATION has the same field sizes as v1 but its separately versioned quality semantics retain low-accuracy coordinates:

| Body offset | Field |
| --- | --- |
| 0,8 | Source sample / actual phone dispatch elapsed ms; u64 each |
| 16,20,24 | Latitude E7 i32; longitude E7 i32; horizontal accuracy cm u32 |
| 28,30,32 | Course centidegrees u16; course accuracy centidegrees u16; speed cm/s u16; each0xFFFF unknown |
| 34,35 | Permission/source mode0 unknown/1 precise/2 approximate/3 mock-invalid; source observation0 no fix/1 coordinate fix/2 stale/3 invalid |
| 36..39 | Reserved0 |

A coordinate fix requires mode1 or2, sample≤dispatch, dispatch−sample<2,000 ms, valid bounds and accuracy≤100,000,000 cm. Course0..35,999 or unknown; course accuracy0..18,000 or unknown. Other observation states carry sample/lat/lon0, accuracy0xFFFFFFFF and all course/speed fields0xFFFF; no mock coordinates survive. Dispatch is nonnegative≤INT64_MAX. Reject repeated/backward source times and sequences; no receipt-time-only freshness renewal. Latest LOCATION and PROGRESS are independently coalesced/rate-limited to≥500 ms per type in the vehicle.

Time handshake mirrors the conservative v1 logic: actual dispatch T0, display handling DB, actual callback T1, RTT≤500 ms and matching request/echo/context. TIME_CONFIRM must arrive within2s of DB. Refresh at25s; expire anchor at30s; account for measured drift/quantization. OPEN/TIME_REQUEST/TIME_CONFIRM are serialized barriers, not pipelined behind bulk DATA. Do not use wall clock. An anchor, source time, source age bound and current link are all needed; future/discontinuous/replayed samples cannot turn live later merely by receipt.

Fresh age bound<2s: mode1 and accuracy≤50m gives filled neutral dot, qualified arrow only with course accuracy≤45° and speed≥2m/s; mode2 or accuracy>50m gives a hollow approximate point and dashed uncertainty ring plus actual numeric accuracy. Coverage is independent: missing roads do not hide a valid position. The ring's screen radius is capped but the accuracy label retains the measured bound. A previously eligible observation becoming stale may show a gray last-known point only until source/receipt age10s; no live arrow/guidance. Then hide it. Invalid/mock, stopped, expired anchor or wrong-session data never become a last-known/live point. Cached maps/camera remain usable while disconnected; a cached route is only a preview.

PROGRESS: route ID u64 at0; revision u32 at8; source elapsed ms u64 at12; remaining metres u32 at20; estimated remaining seconds u32 at24; distance to maneuver metres u32 at28; maneuver u8 at32 (continue0,left1,right2,straight3,arrive4,keep-left5,keep-right6,U-turn7); quality u8 at33 (0 unavailable/1 eligible); reserved34..39. Metre/time fields use the same maxima as route objects. Current route/revision, forward sample/sequence, precise fresh location, live anchor, progress age<2s and along-route position inside the active window are all required for confident guidance. Approximate fixes cannot advance confident turns. Estimates remain labelled estimates; these fields never populate instrument speed/gear/odometer.

### Replies, credits and camera requests

STATUS opcode64 is84 bytes (40-byte envelope +40-byte body +CRC). Envelope sequence is cumulative last accepted control, object/offset are current staging/reuse ID and accepted prefix. Body:

| Body offset | Field |
| --- | --- |
| 0,1 | Stage0 closed/1 idle/2 receiving/3 verifying/4 ready/5 stopped/6 have; error enum below |
| 2,3 | Negotiated window1..4; free downstream command slots0..window |
| 4,8 | Current total object bytes u32; active route revision u32 |
| 12,13,14 | Source0 none/1 precise/2 approximate/3 stale/4 invalid/5 stopped/6 no-anchor; coverage0 missing/1 partial/2 ready; camera0 Follow/1 Browse |
| 15 | Geometry-ceiling flag0/1: producer must simplify/budget further; partial coverage cannot be called complete |
| 16,24,32 | Display monotonic now, echoed T0, DB; u64 each |

Errors:0 none,1 sequence,2 session,3 busy,4 bounds,5 hash,6 offset,7 resource,8 timeout,9 not-ready,10 timing,11 conflict,12 unsupported. READY means a complete validated object, not a complete route corridor or fresh GPS. ACTIVE route revision changes only at ACTIVATE. Credit is0 during verify/closed/stopped. The bridge reduces slots by its unacknowledged occupancy; byte credit is explicitly derived as `freeSlots × min(240,actualMTU−3)` ATT-value bytes, at most960, not another unbounded queue. One encoded object may be staged; do not pipeline BEGINs for several objects. Only DATA is pipelined; wait for cumulative DATA ACK before COMMIT, and READY/HAVE before beginning the next object. Clear pending control credit only from a valid current-context display ACK matching an admitted command; a GATT callback does not replenish it.

INTEREST opcode65 is88 bytes, object/offset0, header sequence=request generation. Body: generation u32 at0; source ID u32 at4; z u8 at8; key count u8 at9≤4; HAVE bitmask u8 at10; camera mode u8 at11; four x/y u32 pairs at12..43, unused pairs0. Visible missing keys have priority, then at most the two nearest halo keys. Successive requests cover a viewport requiring more than four keys. HAVE means a complete compatible non-partial geographic cell is cached; when content certainty matters, use full-SHA HAVE. Generation changes coalesce old camera requests; repeat an unfulfilled generation at bounded1s intervals after transport loss. The phone should deduplicate by source/key/content and drop obsolete interests, while prioritizing source/current route over remote browsing. No public Overpass query per pan. Geometry clipping is exposed separately from cache-hit coverage.

Camera is independent north-up Follow/Browse, single-contact drag with6px threshold, +/− six zoom levels for v2 and explicit physical Pan controls. Pan never changes phone camera, route origin/destination or source location. Semantic display-cell z selection targets roughly512..1,024 projected pixels/tile (bounded0..18 for current interests, decoder supports0..22); oversized-world overview deduplicates wrapped keys. A cached parent can still render while current detail is missing; no fake roads or automatic recenter timer. Ground scale includes latitude. Follow is explicit; a geographic marker may leave the viewport in Browse.

ROUTE_NEED opcode66 is68 bytes: route ID u64 at0, revision u32 at8, requested along-route metres u32 at12, current window start/end u32 at16/20. Header sequence is its own generation. With eligible progress, request a detail window when outside the current range or within200m of its end and the full route continues. Repeat outstanding requests at bounded1s intervals until a matching replacement is activated. Phone also proactively prepares upcoming windows from its full cached route; the display never recalculates routing.

Replies and requests share the retained R-safe R15Q framing. Direction1 permits R15X/v1 and R15V/v2; direction2 permits R15K/R15S and R15J/R15Y. Existing raw-fragment version1,48-byte pieces, CRC and escaping of0/R/0x7D remain. Control/capability versus latest-observation reassembly slots are bounded. Bytes inside corrupt/partial map frames never enter legacy input/pairing/metadata sliding decoders. Critical legacy records precede every next lower-priority fragment; LOCATION/PROGRESS precede bulk DATA; encoded bulk cap4,096 B/s and115200 baud/pins remain unchanged. The sole TX worker drains logical messages outside acquisition. Partial/cancelled fragments may be abandoned, but can never activate an object.

### Reset, compatibility and Android handoff

Reconnect/display reset invalidates control/source/anchor/staging context and active guidance. Keep complete RAM content only if its full source and object SHA can be rebound after a new OPEN; no partial suffix resumes across sessions. Cold power loses RAM and requires current-area-first reload. STOP stops sharing while retaining roads/route preview; END may follow STOP in the same session and clears the route; neither disconnects the phone's independent navigation owner. Old APK/v1 remains usable: a valid current-context v1 TIME_REQUEST/BEGIN switches the shared render arena back to v1; v2 OPEN selects v2. Old v1 LOCATION cannot overwrite a v2 session. Metadata/telemetry/pairing remain unchanged. New app on old firmware must explicitly downgrade rather than sending v2 packets to c017.

Native phone work now has a concrete implemented firmware contract: prove canonical raw geometry/cache and renderer resources, encode source/cells/routes to this schema, serialize handshake/controls, pipeline DATA with downstream credit, honor accepted offsets and full-hash reuse, process INTEREST/ROUTE_NEED, and keep source/route/camera/link/coverage states separate. Use OpenFreeMap/Geoapify only under their configured current terms/key limits. Build Android only in GitHub Actions; user manually checks the APK. Firmware host simulation and builds do not establish physical BLE throughput, phone locked-screen continuity, actual touch/RGB behavior or bike readiness.
