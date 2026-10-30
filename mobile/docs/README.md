# Companion documentation

Updated: 2026-10-30. Flutter Android/iOS companion with an implemented Android foundation and development bridge. The approved four-screen visual direction is planned, not implemented. Production accessory services and iOS device validation remain pending.

| Guide | Authority |
| --- | --- |
| [Architecture](architecture.md) | Ownership, dependency direction, native boundaries and build separation |
| [User journey](user-journey.md) | Unified Phone experience, permissions and failure behavior |
| [Implementation plan](implementation-plan.md) | Milestones, evidence levels and acceptance gates |
| [Approved visual direction](visual-design.md) | Concept 01, screen states, fidelity, reusable styling and motion/performance acceptance |
| [Protocol and transport](protocol.md) | Contract requirements and transport/resource decisions still to specify |
| [Development](development.md) | Toolchain, run/check commands, device and Mac-preview workflow |
| [Security and privacy](security.md) | Trust, authentication, secrets, sensitive content and release gates |

Repository [RULES](../../RULES.md) and [system architecture](../../docs/architecture.md) remain authoritative for the cluster. The unified cluster Phone UI and BLE bonding bench are implemented; the app's Mac development session is separate from physical accessory application services. See [mobile README](../README.md) for implemented app status and the [bench contract](../../docs/phone-uart-bench.md) for hardware evidence. Work in this chat is confined to `mobile/`.

[Wi-Fi Mac preview connection](mac-preview-connection.md) documents the implemented development connection and exact run commands.
