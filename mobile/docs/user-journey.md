# Phone and riding journey

## Mobile startup and home (implemented UI)

Android startup now verifies BLE permissions and radio readiness before showing the main app and on subsequent foreground resumes. Requestable missing permissions prompt at launch; denial, permanently blocked access, Bluetooth off and legacy Location services remain visible/actionable. See [startup access and discovery](development.md#android-startup-access-and-esp32-discovery-2026-10-30). Navigation GPS and optional phone-service permissions are not requested yet. iOS native behavior remains pending.

No account or login. A centered typographic R15 Companion mark appears for 900 ms before home; this is branding, not a claim that services initialized, and is independent of the cluster's 3.00 s boot contract. The timer is cancelled when the app widget is disposed. The OS-native launch screen remains the generated platform default; this branded screen begins with Flutter's first frame.

Without saved trust or a connected observation, the app shows the [approved first-connection landing](visual-design.md#approved-first-connection-refinement): bike artwork, Your bike. In sync. headline and Connect cluster action. The button opens a bounded native discovery sheet with radar, actual results or an actionable empty/error state. A plain launch can discover the existing development service but cannot establish an application session; the UI explains that limitation on selection. With `--mac-preview`, the existing matching-identity/code/cluster-approval flow follows discovery. Neither mode simulates success. Saved disconnected devices retain their reconnect/forget flow. Confirmed connected observations retain the cluster name and gated Navigation card until the Home milestone is implemented. There is no fake map or route service.

Connecting disables repeated attempts; unavailable shows Retry connection. A lost session must replace the connected observation and restore the empty state. Saved pairing alone never proves a live connection. Runtime starts disconnected; the opt-in Mac preview adapter can supply confirmed connected observations. The Mac adapter persists trust; ESP32 session adapters remain pending. Light/dark system appearance and scrollable large-text layouts are supported. Notifications/music are removed from the primary home until their services are implemented.

## First setup

Cluster Settings will expose Appearance, Phone and Navigation rather than separate rider-facing Bluetooth/Wi-Fi setup. Do not invent a fourth tile to fill the old grid. Review geometry before migration. Phone opens Connect phone; discovery/pairing must be a deliberate, bounded setup action. Pairing access requires stationary valid vehicle state and a separately reviewed policy; the existing below-20-km/h Settings gate is not permission to pair while riding.

On phone: Add cluster -> select stable device identity -> verify physical pairing challenge -> authenticated BLE session -> choose features. Request location for navigation, notification access for notifications, and relevant media/background access when enabled. Denying one optional permission does not block unrelated features. Wi-Fi setup is coordinated over the trusted session; OS approval may still be required. Never promise silent provisioning.

Initially support one active trusted phone. Multiple bond storage, replacement and arbitration require explicit later rules. Disconnect preserves trust; Forget requires confirmation and revokes app/session credentials and relevant device trust. Partial forget failures remain visible, with local revocation preventing automatic reuse.

## Daily ride

Boot instrumentation independently of phone readiness; preserve 3.00 s artwork and 2.00 s shutdown contract. Attempt trusted reconnect subject to OS restrictions. Phone selects destination and starts route; enable the bulk link when needed. Show navigation only when requisite state is valid. No destination keyboard on cluster in the first version. Physical controls support navigation view, back and supported media actions without touch.

A proposed dedicated ride navigation view retains speed, gear and all warnings. Map shows surrounding roads, route, moving position, next maneuver and distance/ETA. Street/Track and Midnight/Daylight behavior and warning priority need visual review at 800 x 480 before implementation. Notifications are temporary bounded banners, previews opt-in, and warnings take priority. Settings and ride navigation are separate flows.

## User-facing status

Phone page shows trusted phone plus Navigation, Notifications and Music capability/permission/readiness. Technical BLE/Wi-Fi information belongs in diagnostics. Connected is not equivalent to all features ready. Retry is explicit after actionable failure; avoid endless setup loops.

| Condition | Required behavior |
| --- | --- |
| Bulk Wi-Fi unavailable, BLE healthy | Small direction updates continue if supported; use valid cached map only; indicate missing coverage |
| BLE unavailable, Wi-Fi healthy | Use only explicitly authenticated/negotiated supported fallback; never assume session continuity |
| GPS stale/invalid | Stop claiming a live position; distinguish last-known location and route from current fix |
| Phone lost/rebooted | Mark phone observations unavailable/stale, invalidate old session and commands; core instruments continue |
| Notifications denied | Navigation remains usable; show permission-needed status for notifications |
| Map cache exhausted/provider offline | Explain unavailable coverage; directions only if routing state remains valid |
| Warning or lifecycle interruption | Immediate instrument/warning presentation; cancel pending setup; no phone wait in shutdown |

Phone retains headset audio routing for music/calls/guidance. Cluster audio streaming, call answering, message replies and voice assistant are separate future decisions, not implied by metadata access.
