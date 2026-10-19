# Wi-Fi settings UI

Implemented on the native LVGL host, 2026-10-19. Radio behavior is simulated. Vehicle MCU Wi-Fi drivers, provisioning, credential storage and UART commands are pending.

## Screens and controls

Settings → Wi-Fi retains live gear, compact speed, clock and all tell-tales. Midnight and Daylight share the same geometry in Street and Track. The list contains at most three fixture networks plus Search again, with Saved / Protected / Open labels. This bounded host list is not a claim of production scan capacity; paging or a selected scan window needs a later protocol design.

Select a network to request connection. The saved Rider hotspot fixture and open Guest network connect after a simulated delay. The protected, unsaved Garage Wi-Fi first shows “Credentials pending” and an explicit Simulate connection action. Back cancels that notice. No password is collected, sent, logged or persisted; this is a visual placeholder for provisioning, not authentication. The real credential-entry method and phone hotspot/peer role remain unresolved.

Connected status shows the network name, Disconnect, Forget network and Search again. Disconnect preserves the demo's saved network. Forget requires confirmation with Cancel initially focused; completion removes the connected fixture's saved flag and disconnects. Failed requests preserve confirmed connection state. Scanning/connecting/disconnecting/forgetting, failed, empty, unavailable and stale states are distinct. Unknown status offers Back only, never a false disconnected claim.

Arrow keys cycle actions sequentially; Enter selects; Backspace/Esc goes back. Pending scan/connect is cancelled on leaving. Accepted disconnect/forget can finish after leaving; cancellation does not undo them. Speed at/above 20 km/h, stale/unknown speed, urgent warning or leaving Running closes Settings under the existing policy. The threshold is user-selected UI behavior, not a validated safe riding speed.

## Preview

Run `bash scripts/preview.sh`, then **P → S → Right → Right → Enter**. F5 fails the next Wi-Fi request, F6 toggles unavailable status, Space resets fixtures. Network choices are session-only and never enter the Appearance preference file. Search repeats the fixed host fixture; it does not discover real networks.

## Ownership and integration gates

`WifiState` is a bounded, quality-bearing portable observation. It validates list size, enum bounds and terminated 32-byte SSID storage. Unsupported font bytes or names over 24 characters use a visible fallback; names are not stable network identifiers. `WifiMenu` and SettingsNavigation emit semantic request effects; WifiPanel only renders. The single LVGL owner remains unchanged. WifiDemo handles host-only timing and outcomes outside rendering.

Before real connection support, define stable network identifiers (including same-name networks), scan generation/paging, capabilities/security, request correlation, acknowledgement/timeout/cancellation and session recovery. Bind confirmation and network selection to those identities rather than mutable list positions. Define and review provisioning and vehicle-owned credential protection/storage. No native structs are a wire format. Wi-Fi connection does not establish internet, Maps or projection support.

## Verification

Nine host suites pass. Wi-Fi tests cover unknown status, model bounds, requested versus observed state, cancellation, connection completion, confirmation default, failure retaining an established link, forgetting and access interruption. The render suite has 294 scenarios, including 14 Wi-Fi states/interruption captures in each layout/theme. Stable Wi-Fi fixtures produce zero flushes over 100 unchanged updates. Host success is not ESP32 resource or vehicle evidence.
