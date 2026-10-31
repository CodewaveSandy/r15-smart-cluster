# Nearby discovery and saved phone pairing

Automated test files/runners were removed by user instruction on 2026-10-31. Do not create or restore them until requested. Earlier test results below are historical evidence; current verification uses builds, static analysis, diagnostics and manual preview/bench checks.


Updated 2026-10-21. The cluster now presents one **Phone** category; separate Bluetooth/Wi-Fi pages are removed. Android uses real BLE discovery, then a TLS Wi-Fi session. USB installs/debugs the app and never carries cluster connection traffic.

## First-time journey

1. Start the cluster and app using the commands below. Keep phone and Mac on the same reachable LAN/Wi-Fi for this prototype.
2. Press **P** in the Mac preview to select stationary/neutral simulated telemetry. In Settings, **Phone** sits between Appearance and Navigation.
3. In the app tap **Connect cluster**. Allow Nearby devices access and enable Bluetooth if Android asks. A five-second real BLE scan lists actual advertising R15 devices; there are no seeded results.
4. Select **Connect** next to the cluster. The app reads its public endpoint/identity over GATT, validates the expected development identity, and obtains a pairing offer over pinned TLS. Android can open the system Wi-Fi panel if Wi-Fi is disabled.
5. Both screens show the same code. An incoming request opens the cluster Phone screen only when stationary and Settings access is allowed. Choose **Allow pairing** there (Reject is initially focused), then tap **Pair** in the app. No code typing or terminal enrollment is needed. A premature Pair waits up to 30 seconds for cluster approval.
6. Confirmed connection shows Navigation (maps remain pending), Disconnect and Forget. Android saves trust encrypted with an Android Keystore AES-GCM key in no-backup storage. The Mac stores its CA, identity, bearer credential and saved session in Keychain.

## Later use

Reopening the app restores the saved relationship and attempts reconnect without pairing again. The Mac retains the identity/CA/ticket across preview restarts. If the endpoint changes, the foreground coordinator retries BLE rediscovery without opening permission/radio prompts automatically. BLE addresses are locators; the stable advertised ID and pinned TLS authority establish the expected peer.

Disconnect preserves pairing and pauses this app session's auto-reconnect; reconnect manually or reopen the app. Forget is confirmed, removes saved trust and permits fresh pairing without restarting. Forget on the cluster invalidates the phone ticket; the app removes its saved relationship after the next rejection. Failed requests never optimistically claim successful disconnect/forget. App suspension/background execution is not guaranteed by this slice; heartbeat expiry handles loss.

## Run

Stop old previews, then from the repository root:

```sh
# Terminal 1
bash scripts/preview.sh --phone-bridge
# Terminal 2
bash scripts/preview-mobile.sh --mac-preview
```

The first command compiles a native Swift CoreBluetooth/Keychain helper and starts the TLS bridge and LVGL host. The advertiser is launched as `R15 Nearby.app` so macOS can recognize its Bluetooth permission description. Allow macOS Bluetooth/Keychain/network access when prompted. Advertiser errors are forwarded to the preview terminal. Look for `R15 nearby BLE advertising active.` Android builds use native Kotlin Bluetooth scanning/GATT and Keystore APIs through a typed Flutter platform boundary; no BLE package was added. Trust storage is separate from display Appearance preferences.

If a VPN chooses the wrong interface:

```sh
R15_PREVIEW_HOST=<mac-lan-ipv4> bash scripts/preview.sh --phone-bridge
```

After changing IP, saved apps can rediscover the endpoint over BLE; before first pairing relaunch the mobile script with the active bootstrap. A normal debug launch can restore already saved phone trust even without the flag. The plain host preview has no BLE/TLS listener. macOS BLE permissions must permit the helper; Android Nearby permission denial and Bluetooth/Wi-Fi disabled states remain actionable failures.

## Actual product boundary

This reproduces search → select → first pair → saved reconnect with actual Mac/Android BLE APIs and Wi-Fi transport. It is **application pairing**, not a claim of OS Bluetooth bonding. The GATT characteristic contains only public identity/endpoint metadata; secrets are never advertised or read over unencrypted BLE.

The Mac prototype still uses a shared reachable network. It does not implement the ESP32's own Wi-Fi access-point provisioning or encrypted BLE control/data transport. Those adapters are required for the final router-independent road experience. No outside-road, ESP32 coexistence, iOS discovery, background service or production security readiness is claimed. Navigation/media/notifications remain pending.

Initial Mac trust still comes from the developer launcher bootstrap; this is not a general production enrollment protocol for arbitrary unknown accessories. The first app build is authorized for the selected Mac identity/CA. Production first trust and key rotation need a reviewed accessory provisioning contract. Do not distribute a development APK/bootstrap containing the preview credential.

## Development protocol and ownership

- BLE service UUID `813b1240-8fd9-4d67-a53b-9f0a2531c001`; read-only metadata characteristic `813b1240-8fd9-4d67-a53b-9f0a2531c002`. Metadata is at most 512 bytes: stable hex ID, display name, IPv4 endpoint and port. Scan windows/results and GATT timeouts are bounded. This is a development software contract, not an assertion about purchased hardware.
- TLS pins a stable development CA, with a short-lived server certificate for the current IP. There is no accept-all certificate callback or global trust installation. All HTTPS requests carry the protected development bearer credential. Request/response limits remain 512/2048 bytes and individual network operations time out.
- `/offer` returns a bounded pairing code and saved-pair status; `/pair` requires the code and cluster approval in native preview mode. Offers expire after 60 seconds, enrollment windows after five minutes, invalid code attempts after ten tries. Cluster Pair phone reopens enrollment. The UI guards Pair/Allow at valid stationary speed; warnings/lifecycle/access interruption close Phone and reject a pending request. These are prototype UI policies, not verified bike safety thresholds.
- Saved `/resume`, `/status`, `/disconnect`, `/forget` require the opaque session ticket. Only one saved phone is supported. Credentials persist only through protected adapters; test bridges use isolated in-memory stores. Persistence failure cannot publish a successful saved pairing.
- A private local Unix datagram carries `R15D2:C/D`, little-endian CLOCK_MONOTONIC milliseconds, a flags byte (paired/requested/approved), and six ASCII code digits: 22 bytes total. C++ validates and atomically publishes normalized PhoneState with source/receipt freshness. Legacy 15-byte bridge frames remain for adapter compatibility tests, without Phone enrollment metadata.
- Display actions use bounded, nonblocking local `P/A/R/D/F` commands to arm, approve, reject, disconnect or forget. These are local development IPC, not the future UART command format. Python owns TLS/Keychain/network work; the single LVGL owner only consumes validated state and emits semantic effects.
- Phone heartbeat is two seconds; host marks it disconnected after six seconds. IPC loss becomes unavailable after 1.5 seconds. No old heartbeat can make a stale session live. Local bootstrap/build artifacts are ignored; private key material is staged only in a private temporary directory and Keychain. Logs do not contain credentials or phone content.

## Verification

```sh
cd mobile
flutter analyze --no-pub
```

Mac native BLE advertising readiness and a temporary-item Keychain round trip were exercised. Android native code compiled. Seven Python session tests cover approval, persisted restart/resume, revocation and storage failure. Flutter tests cover discovered results, selection, pair, reconnect, confirmation and faults through injected adapters; the live TLS smoke uses the real Dart client and LVGL host. Actual Android-to-Mac BLE selection/approval and automatic restart reconnect still require the physical-phone run with permissions. This evidence is separate from the user's previously successful Wi-Fi-only bridge connection.
