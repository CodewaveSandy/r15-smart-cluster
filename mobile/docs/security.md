# Security, privacy and permissions

The phone can contain sensitive location, caller, message and media data. Minimize collection: notifications opt-in, previews separately opt-in, no message history or private-content diagnostics. Keep only bounded transient content required for presentation. Clear session content on disconnect/forget; define TTL and cancellation before implementation. No analytics or cloud account is required by this scaffold.

BLE bonding alone is not sufficient authorization for every application operation. Review authenticated device enrollment, physical verification, scoped session capabilities and binding the Wi-Fi peer to the BLE-established identity. Choose maintained cryptographic libraries/protocols; do not invent crypto or equate CRC with trust. Development networking must meet explicit authentication and expiry requirements before being enabled.

Keys belong in OS-protected storage on phone and reviewed vehicle-side protected storage. Display preferences never hold phone credentials. No API secrets, signing files, tokens, private messages, real routes or device identifiers in source control/CI logs. Provider keys need platform restrictions and documented quota limits; mobile bundles cannot keep embedded secrets confidential.

Permissions are least-privilege and requested at use. Revocation while running changes feature capability immediately; security exceptions must not crash the app. Android notification listener/media/background and location behavior requires real-device tests. Future iOS capability differences are represented explicitly. No inaccessible call controls or arbitrary message access is promised.

Before release: review enrollment/revocation, session replay, malformed/flood input, secrets at rest, transport encryption, dependency licenses, privacy disclosures, backup exclusions and signed build ownership. Release signing and store distribution remain pending; generated debug signing is never production signing. No automatic vehicle command forwarding exists.

The implemented [development bridge](mac-preview-connection.md) uses a stable Keychain-protected development CA/identity and launcher-distributed bearer bootstrap, BLE public endpoint discovery and cluster-approved enrollment. Android persists the session with Keystore-backed AES-GCM and no-backup atomic storage. No secrets are exposed over BLE. This is restricted to developer previews; production enrollment and protected persistent credentials remain unimplemented.
