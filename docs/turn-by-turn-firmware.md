# Notification turn guidance — firmware handoff

**Removal progress — 2026-11-05:** Custom map/navigation source, menu/tab, providers/GPS and c016–c01b are removed in the working tree. Replacement turn guidance remains unimplemented; candidate protocol IDs below remain proposals. User explicitly requested removal first.

Planning only, 2026-11-05. Read [the complete replacement plan](turn-by-turn-plan.md) first. No firmware changes have been implemented by this handoff. The proposed bytes below are a reviewable design, **not a frozen existing API**; G0 source feasibility and G1 contract/visual review precede implementation.

## Prerequisites and retained behavior

Confirm the vehicle main-stack correction on hardware:8192-byte actual generated setting, compile guard, repeated ten-second reports without reboot, observed main/TX margins, stable pair-window and numeric approval. Keep all pending UART/stack changes separate from removal; do not restore old map-era firmware wholesale or drop fixes accidentally. Preserve existing c011–c015, R15G/P/N/M, secure bonding/one phone, current wiring/115200 baud, real inputs/telemetry, metadata/clock, LED-off profile and power/lifecycle ownership. No active vehicle-control feature is added.

Back up/hash the current necessary implementation/assets/docs before deleting map files. Remove MapMemory/scene arenas, MapPanel/MapPage, MapCamera/gesture actions, map Settings tile, map/v2 transport endpoints/handlers/queues/stores and map-only previews/build sources. Extract generic framing/CRC into transport-only headers; no retained dependency on MapScene, NavigationStore or map package sizes. Preserve normative old vectors only in the historical archive. Keep standard dashboard preview and hardware/flash guards.

## Proposed wire contract to freeze at G1

New authenticated READ capability c01c, WRITE WITH RESPONSE command c01d, READ/NOTIFY status c01e under existing service UUID prefix. Do not reuse c016–c01b. Validate IDs/permissions/availability on every connection; metadata/telemetry work when guidance is unsupported. Proposed requested MTU247 and actual minimum227 permit a single224-byte max command. Capability advertises the real lower limit; no fixed assumption from Android request success.

All integers little-endian; u32 Unknown is0xffffffff, which is distinct from0. CRC32 ISO-HDLC over all preceding bytes, footer u32. Full strict length/version/type/reserved/context/range checks. Nonzero boot/secured-connection/display/stream IDs and forward nonzero command sequences; source content revision is a separate stream. Same-sequence retries are byte-identical; conflicting duplicates fail. No old connection/stream/time state survives reconnect, reboot or service change.

Command common envelope40 bytes:

| Offset | Field |
| --- | --- |
| 0 | ASCII R15U |
| 4,5,6,7 | version1, command type, header length40, reserved0 |
| 8,12,16,20 | vehicle boot, secure BLE connection, display boot, guidance stream u32 |
| 24,28 | command sequence u32; source-content revision u32 |
| 32 | source content age at dispatch ms u32, Unknown allowed explicitly |
| 36,38 | body length u16; reserved u16=0 |
| 40 | body, followed by CRC32 |

Proposed commands Open, TimeRequest, TimeConfirm, Snapshot, Presence, Clear/Stop. Exact opcode values and non-snapshot bodies/capability/status layouts must be frozen in the normative protocol document **before encoding**, with static (non-executable) examples. Open authorizes a fresh stream; clock handshake records actual T0/DB/T1 with≤500 ms RTT,25 s refresh/30 s expiry and bounded drift. Require compatible timing before asserting trusted source age. No UTC/last persisted time substitutes for a monotonic anchor.

Proposed Snapshot body180 bytes (total224):

| Offset | Field |
| --- | --- |
| 0 | source state u8: waiting/guiding/rerouting/arrived/unsupported/GPS-lost/paused; frozen values at G1 |
| 1 | maneuver u8: Continue0,Left1,Right2,Straight3,Arrive4,KeepLeft5,KeepRight6,Uturn7,Roundabout8,Unknown255 |
| 2 | validity/quality flags u16, only declared bits permitted |
| 4,8,12 | next-turn metres, remaining metres, remaining seconds u32 (Unknown sentinel) |
| 16 | roundabout exit u8;0 unknown;1..32 explicit only |
| 17,18,19 | instruction/road/summary byte lengths u8,≤96/32/16 |
| 20,28 | meaningful source-change elapsed ms u64; actual dispatch elapsed ms u64 |
| 36..131 | instruction UTF-8 bytes96, unused padding zero |
| 132..163 | road UTF-8 bytes32, unused padding zero |
| 164..179 | optional provider summary/ETA text UTF-8 bytes16, unused padding zero |

Validate field validity flags against values, UTF-8 boundaries/no control characters, zero padding, finite ranges and known glyph profile. Unknown-age snapshots have canonical unknown source timestamp/age, not fictitious timestamp0 interpreted as boot-time observation. Text truncation is explicit/marked; a truncated or unparsed phrase cannot yield a confident maneuver. Phone and display must agree on v1 repertoire; unsupported language never yields random glyphs or a guessed arrow. Do not render or reinterpret arbitrary icon/resource IDs.

ETA/summary-only changes do not change the core instruction revision/time. Presence confirms listener/notification availability only; it references the current content revision and cannot update the source-change time. Clear removes fields/arrow/ETA promptly, preempts a queued snapshot and fences delayed old commands with sequence/stream tombstones. Explicit SourceAbsent, PermissionRevoked, Unsupported, Stop and LinkLost remain distinguishable in normalized state. UART/GATT ACKs do not advance content freshness.

Status must report actual display acceptance/publication, consumed sequence, accepted content revision, source/transport/clock state and reason. A vehicle admission/ATT write is not display success. Expiry remains enforced locally on the display even if the phone disappears. Timing/content unknown or expired means neutral/unavailable, never an old bright arrow. Preserve the content-age vs presence policy and long-straight qualification in the main plan.

## Vehicle ownership and budgets

Add a small guidance ingress/relay in BlePhone and the existing RecordWriter. One outstanding control and one replaceable latest snapshot, priority Clear; bounded data/text storage, no city/route objects, full-SHA stores or unbounded allocation. Legacy acquisition/phone control/metadata precede every new lower-priority fragment. Guiding snapshot cap2/s, Presence every2 s, encoded guidance budget1536 B/s. Reuse generic48-byte escaped R15Q fragments, total224 bytes, bounded≤500 ms reassembly and no duplicate-GATT/RTOS output owner. Cancel/stop may abandon incomplete lower-priority assembly safely; it cannot partially publish.

Retain actual driver resets/CRC/sequence/queue/source-gap diagnostics; replace map-specific counters with numeric guidance observations/admissions/display ACK age/rejections/expired/last source age and maximum event→ACK latency. Do not log strings, keys, coordinates or notification contents. Measure stack/heap after removing map objects; do not infer success from smaller source files. Use explicit compile guards and actual generated Kconfig values.

## Display integration

A dependency-free normalized `TurnGuidanceState` and application store own validation, source/transport expiry and atomic replacement. Presentation generates bounded strings/glyph selection; widgets never decode packets. `Dashboard` owns one compact turn card in the existing lower trip/info area, both layouts/themes, preserving fuel and critical instrumentation. No full navigation screen/menu is retained. Freeze exact geometry at G1 by manual800×480 preview inspection, including no data, waiting, left/right, unknown, rerouting, long text, arrival, stale, disconnected, warnings, Settings and lifecycle.

Remove map-specific Settings focus/touch/button branches. Reindex three Settings tiles without breaking Clock/Phone/Appearance. Existing physical inputs continue their ordinary actions; guidance is automatic and requires no new ride-time touch flow. Clear restores normal trip/info. Opening Settings or a pairing modal temporarily obscures the card without stopping its source; only the current eligible state may appear when returning.

## Required evidence

Build both pinned firmware targets and standard host preview; no automated suites. Manually inspect the new semantic scenarios and static normative vectors for boundaries/corrupt/late/stop/reconnect behavior; label simulated content as simulation. On boards, prove baseline stability first, then actual phone notifications, time/ACK bounds, unchanged c011–c015 behavior and source freshness/stop. Record image hashes, actual MTU, memory freed by map-arena removal, source/UART/UI gaps, task margins and untethered run observations. No completion claim before joint acceptance.
