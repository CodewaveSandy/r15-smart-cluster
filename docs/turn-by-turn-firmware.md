# Google Maps notification guidance — firmware implementation

**Android integration — 2026-11-06:** The notification listener/parser, native sharing session, existing-owner BLE channel and My bike controls are now source-integrated against firmware `50f6bc2`. See the [implementation and manual APK checklist](../mobile/docs/turn-guidance-implementation.md). Local verification is formatting/static analysis/syntax only; APK/Android compilation belongs to GitHub Actions and phone/board acceptance remains pending. Earlier “Android pending/unimplemented” passages describe the previous handoff state.

2026-11-05: **complete firmware source integration**, board builds and manual host-preview verification. No Android listener/parser is implemented yet, no board has been flashed in this work, and target-device notification feasibility/latency/long-straight cadence/locked-screen acceptance remain pending. The user requested firmware first after [map removal](navigation-removal.md); the original G0 phone-observation order was deferred explicitly. Do not confuse implementation with physical acceptance.

The [normative v1 contract](turn-guidance-protocol.md) now supersedes every provisional byte/field in the [replacement plan](turn-by-turn-plan.md). [Static wire examples](../shared/protocol/turn-guidance-v1-vectors.txt) are data, not an executable suite. Android must implement these bytes before the feature works with a real phone; the old map APK has no guidance producer.

## Implemented path and ownership

- `shared/model/TurnGuidanceState.h` is a dependency-free normalized model. `shared/protocol/TurnGuidance.h` validates/encodes explicit bytes, profile/ranges/flags/text padding/context/CRC. `Binary.h`, `FramedUart.h` and `UartRecords.h` isolate generic framing from the feature.
- `shared/application/TurnGuidanceStore.h` is the display owner: atomic complete snapshots, separate content revision/command ordering, monotonic interval anchor, source and transport expiry, new-stream/Stop tombstones, identical retry handling and publication status. No LVGL/RTOS/radio/storage dependency.
- `shared/application/TurnGuidanceBridge.h` is the bounded vehicle admission/relay owner. BlePhone serializes it under the existing cross-task lock, validates current authentication and actual MTU, and owns c01c–c01e in the existing service. Display capability/status are the source of readiness/ACK; ATT admission is not display success. One outstanding ACK barrier; Clear supersedes it. Missing notification completion cannot grow a backlog; status read fallback remains available.
- `RecordWriter` remains the single TX task per output, with unchanged G/P/N/M priority and console write+flush locking. A latest framed mailbox, whole-fragment Clear preemption and1536 encoded B/s token budget bound guidance work. RX rings remain4096 bytes with1536-byte bounded drains. Partial assemblies expire at500 ms; no partial command reaches presentation.
- Display `VehicleUart` receives real complete command timestamps, applies current vehicle/phone context, publishes capability/status and forwards display ACKs. Its LVGL-owning `app_main` passes normalized state into Dashboard, then stamps model publication/actual visibility after the rendering turn. No application/ISR/radio callback touches LVGL.
- `TurnGuidancePresentation` prepares bounded text/numeric fields; `TurnGuidanceCard` draws local vectors using pinned LVGL's line widget. It appears automatically in x28/y392/w502/h86, replacing only trip/info labels. Full lower divider, fuel, gear, tachometer, speed/lap, tell-tales, clock and warnings remain. Street/Track and Midnight/Daylight share the component. Settings/modal, urgent warnings and lifecycle hide it; Trip/Info keep their ordinary behavior. No Navigation menu/full-screen map/touch interaction was added.

## Freshness and failure behavior

Fresh supported instructions display only declared fields. Unknown distance is `-- m`; zero means0 m. Text-only guidance has a neutral information glyph. No countdown, routing, GPS or direction inference runs on either MCU. English printable ASCII is the explicit first profile; malformed/non-ASCII payloads are rejected. Android sends a distinct supported Unsupported state for unrenderable notifications rather than stripping text or guessing arrows. Truncated text never drives an emphasized maneuver.

TimeRequest/Confirm measures phone monotonic dispatch/callback and actual display receipt, with≤500 ms RTT,25 s refresh and30 s expiry. Clock jumps conflicting with an existing anchor are rejected. Unknown baseline age remains Unknown; Presence/retry/summary updates never renew source change time. Same-revision core changes and new revisions containing unchanged core content are rejected. Source age≥30 s mutes instruction/no action or ETA;≥120 s shows update pending. Clock/age uncertainty is neutral; previously expired action text cannot reappear merely because a clock anchor expires. These source thresholds are frozen firmware behavior awaiting validation against the installed Maps version.

No Snapshot/Presence for5 s closes the stream, clears all action/ETA and briefly shows connection loss, then returns normal info. Clear/Stop also handles overtaking an Open that has not reached the display yet, then promptly restores trip/info and fences delayed packets. Removal/revoke/unsupported/listener loss have separate reason codes. A supported arrival lasts at most5 s from the first content revision; retries/Presence do not extend it. A forward Open with a different stream is required after Clear/expiry; command ordering does not reset inside the same secured session. New secured connection/display boot invalidates old context/timing.

## Preserved baseline

c011–c015, strict telemetry feature mask, Secure Connections/numeric approval/stored identity/16-byte key, one phone, metadata/clock, discrete inputs, UART pins/115200 baud, LED-off profile and guarded flashing/partitions/backups remain. Vehicle main task stays8192 bytes with the existing compile guard; the display also compile-checks its existing16384-byte owner stack. No active vehicle command, NVS erase or hidden bond repair is added. Generic demux still isolates retired map payloads; c016–c01b remain absent.

A clean preview caught an icon definition count left over from map removal (18 slots for17 definitions). It could replace Left's valid glyph with a null default entry and assert on construction. The count is corrected and compile-time checked; the normal dashboard/Settings/Phone renderer was rerun after the fix. Do not flash the earlier removal-only images as a substitute for these updated images.

## Resources and build policy

All guidance objects/queues are fixed-size. The host reports Store584 bytes, Bridge512 bytes and one assembler248 bytes (host ABI); MCU guards include the actual board ABI, RecordWriter, receiver/reassembly and additional message/presentation workspaces under4096 bytes per MCU. Existing UART/task/LVGL/font storage is excluded from that guard. No map arena/PSRAM/file/cache/provider dependency returns. Actual free heap, largest block and task margins require bench measurement; smaller binaries are not proof of runtime headroom.

`@GUIDANCE` (vehicle) and `GUIDANCE` (display) diagnostics contain only numeric accepted/rejected/duplicate/expired counts, framing/CRC/timeouts, source age/quality, ACK and encoded-byte counts. Vehicle maximum ACK measures vehicle admission→display model publication, not unobserved Google Maps event latency. Existing UART/heap/stack/source-gap/UI diagnostics remain. No notification/instruction/road text is logged or persisted.

Both pinned build-only scripts package/validate image fit without board access. LVGL line support is enabled in the shared feature profile; the first display build needed a clean generated PlatformIO cache to replace the old library. When changing that profile on another checkout with stale component objects, run:

```sh
~/.platformio/penv/bin/pio run -d firmware/display -e crowpanel-v3 -t clean
bash scripts/flash-display.sh --build-only
bash scripts/flash-vehicle.sh --build-only
```

The clean target affects generated display build outputs, not source or separately verified factory backups. Final hashes/image sizes and remaining bench gates are recorded in [development](development.md#notification-turn-guidance-firmware--2026-11-05).

## Manual preview and physical handoff

```sh
# Build the existing host preview and write explicitly labelled simulations:
bash scripts/preview.sh --guidance-render build/lvgl/turn-guidance-review
# For a headless render with an already built macOS binary:
SDL_VIDEODRIVER=dummy build/lvgl/r15-preview.app/Contents/MacOS/r15-preview --guidance-render build/lvgl/turn-guidance-review
```

`GuidancePreview.h` is a developer rendering tool using the real bridge/store/demux/assembler/codec; it is not a restored automated suite, phone notification parser or target throughput measurement. Captures cover all maneuvers/exit, text-only, zero/unknown/maximum distance, long/truncated text, unsupported/GPS loss/pause/reroute, unknown age, duplicate/conflicting retry, invalid UTF-8/range/CRC/future/late context, partial data, source/clock/transport expiry, Stop/arrival, unavailable instruments, warnings, Settings and lifecycle in all four layout/theme combinations. Scene diagnostics distinguish intentionally rejected samples; no simulated success is called real Maps integration.

Before full app acceptance, flash both matched images manually and first prove stable uptime≥5 min across diagnostics, main/TX margins, pair window/numeric approval, saved reconnect, phone name/battery/time and input telemetry. Then integrate the [Android handoff](../mobile/docs/turn-by-turn.md) in GitHub Actions, observe actual notification fields on the target phone, and measure negotiated MTU, source/receipt/ACK timing, no stale revival, Clear/revoke/reconnect/one-board reset, dropped callbacks/CRC/queue pressure, UI gaps and60 min locked-screen coexistence. No automated suites, Android local builds, flashing or physical checks were performed here.

A verified pre-change source backup is `build/source-backups/before-turn-guidance-20261105-180144/source.tar.gz` with `SHA256.json` (317 source/doc/asset members verified). Build outputs/captures/backups remain ignored. User controls commits, pushes and flashing.
