import 'package:flutter/material.dart';

import '../domain/phone_status.dart';
import '../application/ble_connection.dart';
import '../application/preview_session.dart';
import '../../../app/theme/companion_theme.dart';
import 'preview_pairing_sheet.dart';
import 'connection_landing.dart';

/// Existing pairing/connection intents, available from Home connection settings.
class PhonePage extends StatelessWidget {
  const PhonePage({
    required this.status,
    this.session,
    this.ble,
    this.embedded = false,
    super.key,
  });
  final PhoneStatus status;
  final PreviewSession? session;
  final BleConnection? ble;
  final bool embedded;

  void _connect(BuildContext context) {
    if (ble?.hasSelection == true) {
      ble!.reconnect();
      return;
    }
    if (session?.hasTrust == true) {
      session!.reconnect();
      return;
    }
    showDialog<void>(
      context: context,
      barrierDismissible: false,
      builder: (_) => Dialog(
        insetPadding: const EdgeInsets.all(24),
        backgroundColor: Theme.of(context).colorScheme.surfaceContainerLow,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(24)),
        child: ConstrainedBox(
          constraints: const BoxConstraints(maxWidth: 420),
          child: PreviewPairingSheet(session: session, ble: ble),
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final connected = status.connection == ClusterConnection.connected;
    final busy =
        status.connection == ClusterConnection.connecting ||
        (session?.busy ?? ble?.busy ?? false);
    final hasSelection = session?.hasTrust == true || ble?.hasSelection == true;
    final error = session?.error ?? ble?.error;
    final colors = Theme.of(context).colorScheme;
    final theme = Theme.of(context).textTheme;
    if (!connected && !hasSelection) {
      return ConnectionLanding(
        onConnect: () => _connect(context),
        busy: busy,
        error: error,
      );
    }
    final content = SingleChildScrollView(
      padding: const EdgeInsets.all(CompanionTokens.pagePadding),
      child: Center(
        child: ConstrainedBox(
          constraints: const BoxConstraints(
            maxWidth: CompanionTokens.contentWidth,
          ),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              const SizedBox(height: 24),
              Icon(
                connected ? Icons.bluetooth_connected : Icons.bluetooth,
                size: 60,
                color: colors.primary,
              ),
              const SizedBox(height: 24),
              Text(
                status.clusterName ?? 'Your cluster',
                textAlign: TextAlign.center,
                style: theme.headlineMedium,
              ),
              const SizedBox(height: 12),
              Text(
                connected
                    ? (status.bluetoothLinkOnly
                          ? 'Bluetooth connected'
                          : 'Cluster connected')
                    : busy
                    ? 'Connecting to your cluster…'
                    : 'Cluster disconnected',
                textAlign: TextAlign.center,
                style: theme.titleMedium,
              ),
              const SizedBox(height: 24),
              Card(
                margin: EdgeInsets.zero,
                child: Padding(
                  padding: const EdgeInsets.all(20),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        ble?.bonded == true || session?.hasTrust == true
                            ? 'Pairing saved'
                            : 'Pairing unavailable',
                        style: theme.titleMedium,
                      ),
                      const SizedBox(height: 8),
                      Text(
                        status.bluetoothLinkOnly
                            ? 'Your saved pairing lets this phone reconnect. Open Navigate to share the Kolkata map and current location. Route guidance, notifications and music are not available yet.'
                            : 'The development preview connection is separate from the physical cluster. Feature availability depends on the active session.',
                        style: theme.bodyMedium?.copyWith(
                          color: colors.onSurfaceVariant,
                        ),
                      ),
                    ],
                  ),
                ),
              ),
              if (connected && status.bluetoothLinkOnly)
                Padding(
                  padding: const EdgeInsets.only(top: 16),
                  child: Text(
                    switch (status.metadata) {
                      PhoneMetadataState.synced => 'Phone name, battery and clock details shared with your cluster.',
                      PhoneMetadataState.syncing => 'Syncing phone details…',
                      PhoneMetadataState.unsupported => 'Phone details are unavailable with this cluster firmware.',
                      _ => 'Phone details are currently unavailable. The Bluetooth link is still connected.',
                    },
                    style: theme.bodyMedium?.copyWith(
                      color: colors.onSurfaceVariant,
                    ),
                  ),
                ),
              if (error != null)
                Padding(
                  padding: const EdgeInsets.only(top: 20),
                  child: Text(error, style: TextStyle(color: colors.error)),
                ),
              const SizedBox(height: 24),
              if (connected)
                OutlinedButton.icon(
                  onPressed: busy
                      ? null
                      : () {
                          if (ble != null) {
                            ble!.disconnect();
                          } else {
                            session?.disconnect();
                          }
                        },
                  icon: const Icon(Icons.link_off),
                  label: const Text('Disconnect'),
                )
              else
                FilledButton.icon(
                  onPressed: busy ? null : () => _connect(context),
                  icon: const Icon(Icons.add_link),
                  label: Text(busy ? 'Connecting…' : 'Connect cluster'),
                ),
              const SizedBox(height: 12),
              TextButton(
                onPressed: busy ? null : () => _forget(context),
                child: Text(
                  ble != null ? 'Forget app selection' : 'Forget preview',
                ),
              ),
            ],
          ),
        ),
      ),
    );
    return embedded ? content : Scaffold(body: SafeArea(child: content));
  }

  Future<void> _forget(BuildContext context) async {
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: Text(
          ble != null ? 'Forget app selection?' : 'Forget Mac preview?',
        ),
        content: Text(
          ble != null
              ? 'This clears only the app selection. To remove trust, also forget this phone on the cluster and remove the cluster in Android Bluetooth settings.'
              : 'Remove this saved pairing? You can find and pair with your cluster again.',
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: const Text('Cancel'),
          ),
          TextButton(
            onPressed: () => Navigator.pop(context, true),
            child: const Text('Forget'),
          ),
        ],
      ),
    );
    if (confirmed == true) {
      if (ble != null) {
        await ble!.forget();
      } else {
        await session?.forget();
      }
    }
  }
}
