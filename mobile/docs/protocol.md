# Phone protocol and transport plan

Status: production/firmware protocol requirements only. An isolated [Wi-Fi Mac preview contract](mac-preview-connection.md) is implemented for development; it is not the final BLE/Wi-Fi or UART wire format. No wire format, codec, UUIDs, ports or cryptographic handshake is implemented or frozen. Phone network framing and inter-MCU UART framing are separate contracts sharing normalized semantics; do not require identical envelopes.

## Transport policy

BLE is proposed for authenticated setup, capabilities, health, commands/acknowledgements, GPS/maneuvers, notification summaries and media metadata. Wi-Fi is proposed for bounded map chunks, larger route geometry and optional artwork. Selection follows urgency/size/capability, not a hard per-feature split. Avoid dual-path duplicate commands. Audio remains phone/headset.

Investigate vehicle ESP32 private access point with phone joining via OS-approved provisioning. Confirm supported phone cellular internet routing during local Wi-Fi, background behavior and actual ESP32 coexistence. No claim of automatic hotspot toggling or identical iOS setup. See [Android networking](https://developer.android.com/develop/connectivity/wifi/wifi-infrastructure).

Wi-Fi does not remove the UART bottleneck. The current proposed 115200 8N1 link has about 11,520 bytes/s before framing/telemetry. One 800 x 480 RGB565 frame is 768,000 bytes, about 66.7 seconds even without other traffic. Use compact geometry/chunks and measure the whole path; revised baud/framing requires board validation. Preload route corridors and bound cache/transfer work; don't infer performance from host CPU or PSRAM capacity.

## Required contract review before codec implementation

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

Maintain language-neutral fixtures alongside the first normative specification. Dart and C++ codecs must consume the same positive/negative golden vectors, including oversized/truncated data, unknown versions, corrupt integrity, stale source, duplicate/out-of-order, reboot, reconnect and interleaved transfer cancellation.

## Mac development adapter

Phone and Mac initially share a LAN. A development-only QR/bootstrap connection may identify endpoint and ephemeral trust; it must not contain long-lived secrets or authorize an arbitrary unauthenticated endpoint. Authentication, local bind policy and expiry must be specified before exposing a listener. Opt-in bridge, bounded input, no sensitive logs, no blanket cleartext exceptions. Real bike/vehicle control messages are absent.

Inject recorded routes and faults; simulate UART bandwidth, bounded buffers and latency. These establish behavior, not hardware feasibility. Route validated observations into simulator state outside LVGL, then consume them under its single owner. Host sockets never become dependencies of shared C++ models or firmware code.
