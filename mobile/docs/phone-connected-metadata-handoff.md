# App handoff: connected phone name, battery and time

Date: 2026-10-31. Firmware implementation; physical acceptance pending. Extend the existing working Android connection in `mobile/android/app/src/main/kotlin/dev/r15cluster/r15_companion/BleBond.kt`; preserve its sole GATT owner, saved reconnect, OS numeric comparison and existing UI. Read mobile/AGENTS.md and the mobile rules first. No mobile source changes are included in this firmware work.

## Outcome

On each authenticated BLE connection, send the actual phone label, current battery percentage and current UTC time/local UTC offset. The cluster header shows the phone icon and battery text only while a fresh authenticated connected status is available. Settings → Phone shows the transmitted name and `Battery N%`; missing fields show `Phone name unavailable` / `Battery unavailable`. A saved disconnected bond does not imply a live phone or retain its metadata. No navigation capability is introduced.

The existing enrollment name characteristic `813b1240-8fd9-4d67-a53b-9f0a2531c011` remains unchanged. Continue sending it before a first bond for the pairing modal. It does not supply connected metadata and remains forbidden for saved bonded peers.

## New authenticated write

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

## Android integration sequence

1. Keep the existing first-pair name → numeric comparison flow. For saved peers preserve immediate security/reconnect.
2. Discover the new characteristic on every live connection, including saved reconnect. Negotiate ATT MTU of at least 47 before this write (44-byte value plus 3-byte ATT overhead); serialize MTU/discovery/write operations through real callbacks. Do not interfere with the enrollment name's three-second deadline. An unsupported MTU/endpoint must leave the existing connection functional and report metadata unavailable.
3. Wait for OS bonding/security completion. BONDED alone can precede firmware security completion; if the new write is temporarily refused, retry with bounded delays (for example 1/2/4 seconds) while the same GATT is current. Do not restart pairing or request another enrollment window. A successful authenticated write confirms that this metadata endpoint accepted the secured current connection; it does not establish future navigation/application services.
4. Immediately send a full snapshot after each successful new connection/reconnect. Send every 30 seconds while connected, and on foreground resume, phone time/timezone change or battery change, coalescing events to remain at least one second apart. Use lifecycle-safe Android battery/time observations; handle permission/API limitations explicitly. No new background-service promise is implied.
5. Cancel work, callbacks and queued snapshots on disconnect, Bluetooth loss, selection change or disposal. Guard every callback with the current GATT/generation. Recreate metadata from current phone observations on reconnect; never replay a previous connection's pending snapshot.
6. Older firmware without the endpoint must continue the existing pairing/connection behavior. Do not repeatedly rediscover or fail the connection just because metadata is unsupported.

## Firmware transport and clock behavior

Both MCUs require new firmware. R15P status version 1 now uses bytes 40–43 for a nonzero random per-connection identifier (zero when disconnected); CRC remains at 44. Older strict displays reject this extension, so update both boards together. Pairing R15N and discrete-input R15G framing are unchanged.

New R15M v1 UART frames are 80 bytes: magic at 0–3; version at 4; zero 5–7; vehicle boot session 8–11; BLE connection identifier 12–15; stream sequence 16–19; source uptime 20–23; the 44-byte payload 24–67; zero 68–75; CRC-32/ISO-HDLC over bytes 0–75 at 76–79. Integers are little endian. Fixed receivers validate framing, CRC, padding, bounds, source session, connection identity, sequence and uptime ordering. Frames expire locally after one second. Metadata is sent at most 5 Hz, separately from status/pairing-name frames; 36-byte input plus 80-byte metadata fits the 128-byte TX FIFO. No extra task or LVGL access is added to BLE callbacks.

Vehicle extrapolates accepted UTC using monotonic elapsed time while metadata is fresh. Display synchronizes its existing clock service from current matching frames; automatic timezone follows the phone offset, manual timezone and persisted date/12-hour preferences remain respected. After disconnect/metadata expiry the last validated clock keeps ticking on display uptime. Cold power still requires a new real sync; no RTC retention or timestamp persistence is claimed. USB bench sync remains available; the most recently received valid source applies.

## Manual acceptance with the existing app developer

Build/analyze the affected app using its pinned toolchain; do not restore automated tests. With both updated boards and the real phone, check first pair, saved reconnect after board power cycle, app restart, explicit Disconnect/Connect and Forget. Confirm real name/battery after every connection, battery 0/100/unknown, long/missing/unsupported names, missing/invalid time, time/date rollover, automatic/manual timezone and 12/24-hour formats. Confirm disconnected/lost UART hides header phone indicators and clears live metadata; reconnect cannot show a previous connection's snapshot. Stop updates for over 90 seconds and confirm unavailable metadata while BLE remains connected. Reject malformed/short/oversized/rate-limited writes without bond loss. Inspect Street/Track and Midnight/Daylight, Settings focus/back navigation and simultaneous warnings. Confirm both 2-second lifecycle sequences remain intact.

Do not log phone labels, pairing codes, keys or phone contents. Record device/OS and app/firmware revisions, numeric GATT results and actual observations. Firmware build/preview evidence is not a physical acceptance claim. See [existing connection handoff](esp32-connection-handoff.md) for pairing troubleshooting and ownership.

## Firmware verification recorded here

Vehicle build/package and display build/package passed, including display application/partition fit validation. Host UI build passed. The development preview rendered its dashboard, Settings, Phone and lifecycle scenarios with no final diagnostic failure; the initial settled-row redraw diagnostic was corrected before the final successful run. Visually inspected native 800 × 480 connected name/battery states in Street/Midnight and Track/Daylight, disconnected/unavailable Phone states, simultaneous warnings and unavailable dashboard data. The existing connected BLE preview scene now uses an explicit sample label and 73% battery for visual inspection only.

No automated test files/suites were created or run. No boards were accessed or flashed; no physical BLE payload delivery, Android MTU/security ordering, UART timing, panel behavior, battery/time changes, reconnect or cold-power checks were performed here. These remain required app/bench acceptance. Local build logs/captures/backups are ignored under `build/`.

## Android implementation handoff result (2026-10-31)

The app-side integration is implemented on this workstream without firmware edits. Format/analyze and arm64 Android build/install passed. Samsung SM-M346B / Android 16 reconnected to the saved updated cluster; MTU callback reported 256/status 0 and connected metadata write callback reported status 0. The user confirmed the correct name, phone battery and local time/date on the cluster. Repeated 30-second refresh writes also returned status 0 for over four minutes. Explicit Disconnect cleared app metadata status; Connect and app force-stop/relaunch each negotiated MTU 256 and delivered a new accepted snapshot without another bonding transition. This establishes the core real path; the remaining fault/expiry/board-reset acceptance remains separate. Implementation responsibilities, retries/refresh/fallback and the remaining manual checks are documented in [development](development.md#connected-phone-metadata-integration-2026-10-31). No codes, keys, phone labels or payloads are retained in the record.
