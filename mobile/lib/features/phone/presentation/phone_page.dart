import 'package:flutter/material.dart';

import '../../../app/widgets/brand_mark.dart';
import '../domain/phone_status.dart';
import '../application/ble_connection.dart';
import '../application/preview_session.dart';
import 'preview_pairing_sheet.dart';
import 'connection_landing.dart';

class PhonePage extends StatelessWidget {
  const PhonePage({required this.status, this.session, this.ble, super.key});
  final PhoneStatus status;
  final PreviewSession? session;
  final BleConnection? ble;

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
    final unavailable = status.connection == ClusterConnection.unavailable;
    final colors = Theme.of(context).colorScheme;
    if (!connected && session?.hasTrust != true && ble?.hasSelection != true) {
      return ConnectionLanding(
        onConnect: () => _connect(context),
        busy: busy,
        error:
            session?.error ??
            ble?.error ??
            (unavailable ? 'Cluster connection unavailable' : null),
      );
    }
    return Scaffold(
      body: SafeArea(
        child: Center(
          child: ConstrainedBox(
            constraints: const BoxConstraints(maxWidth: 600),
            child: CustomScrollView(
              slivers: [
                SliverPadding(
                  padding: const EdgeInsets.fromLTRB(24, 24, 24, 0),
                  sliver: SliverToBoxAdapter(
                    child: Row(
                      children: [
                        Icon(Icons.two_wheeler_outlined, color: colors.primary),
                        const SizedBox(width: 12),
                        Expanded(
                          child: Text(
                            'R15 Companion',
                            style: Theme.of(context).textTheme.titleMedium,
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
                SliverFillRemaining(
                  hasScrollBody: false,
                  child: Padding(
                    padding: const EdgeInsets.all(28),
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      crossAxisAlignment: CrossAxisAlignment.stretch,
                      children: [
                        const Center(child: BrandMark()),
                        const SizedBox(height: 40),
                        Text(
                          connected
                              ? (status.bluetoothLinkOnly
                                    ? 'Your cluster is nearby.'
                                    : 'Ready for your next ride.')
                              : status.bluetoothLinkOnly
                              ? 'Your cluster is disconnected.'
                              : 'Your ride. Connected.',
                          textAlign: TextAlign.center,
                          style: Theme.of(context).textTheme.headlineMedium
                              ?.copyWith(fontWeight: FontWeight.w600),
                        ),
                        const SizedBox(height: 16),
                        Text(
                          connected
                              ? '${status.clusterName ?? 'Your cluster'} · ${status.bluetoothLinkOnly ? 'Bluetooth connected' : 'Connected'}'
                              : busy
                              ? 'Connecting to your cluster…'
                              : unavailable
                              ? 'Cluster connection unavailable'
                              : status.bluetoothLinkOnly
                              ? '${status.clusterName ?? 'Your cluster'} · ${ble?.bonded == true ? 'Bond saved' : 'Bond unavailable'} · Disconnected'
                              : 'Connect your cluster to bring navigation to your ride.',
                          textAlign: TextAlign.center,
                          style: Theme.of(context).textTheme.bodyLarge
                              ?.copyWith(color: colors.onSurfaceVariant),
                        ),
                        if (status.bluetoothLinkOnly && connected)
                          const Padding(
                            padding: EdgeInsets.only(top: 12),
                            child: Text(
                              "Bluetooth link only. Application authorization, navigation, notifications and music are pending.",
                              textAlign: TextAlign.center,
                            ),
                          ),
                        const SizedBox(height: 32),
                        if (session != null || ble != null) ...[
                          if (session?.hasTrust == true || ble?.bonded == true)
                            const Text(
                              'PAIRED CLUSTER',
                              textAlign: TextAlign.center,
                            ),
                          if ((session?.error ?? ble?.error) != null)
                            Padding(
                              padding: const EdgeInsets.symmetric(vertical: 12),
                              child: Text(
                                (session?.error ?? ble?.error)!,
                                textAlign: TextAlign.center,
                                style: TextStyle(color: colors.error),
                              ),
                            ),
                          const SizedBox(height: 12),
                        ],
                        if (connected) ...[
                          Card.filled(
                            child: Padding(
                              padding: const EdgeInsets.all(24),
                              child: Column(
                                crossAxisAlignment: CrossAxisAlignment.start,
                                children: [
                                  Icon(
                                    Icons.navigation_outlined,
                                    color: colors.primary,
                                    size: 32,
                                  ),
                                  const SizedBox(height: 16),
                                  Text(
                                    'Navigation',
                                    style: Theme.of(context)
                                        .textTheme
                                        .titleLarge,
                                  ),
                                  const SizedBox(height: 8),
                                  Text(_navigationLabel),
                                  const SizedBox(height: 20),
                                  FilledButton.icon(
                                    onPressed:
                                        status.navigation ==
                                            FeatureReadiness.ready
                                        ? () => Navigator.of(context).push(
                                            MaterialPageRoute<void>(
                                              builder: (context) => Scaffold(
                                                appBar: AppBar(
                                                  title: const Text(
                                                    'Navigation',
                                                  ),
                                                ),
                                                body: const Center(
                                                  child: Padding(
                                                    padding: EdgeInsets.all(24),
                                                    child: Text(
                                                      'Destination search and maps are coming next.',
                                                    ),
                                                  ),
                                                ),
                                              ),
                                            ),
                                          )
                                        : null,
                                    icon: const Icon(Icons.arrow_forward),
                                    label: const Text('Open navigation'),
                                  ),
                                ],
                              ),
                            ),
                          ),
                          if (session != null || ble != null) ...[
                            TextButton(
                              onPressed: busy
                                  ? null
                                  : () {
                                      if (ble != null) {
                                        ble!.disconnect();
                                      } else {
                                        session!.disconnect();
                                      }
                                    },
                              child: const Text('Disconnect'),
                            ),
                            TextButton(
                              onPressed: busy ? null : () => _forget(context),
                              child: Text(
                                ble != null
                                    ? 'Forget app selection'
                                    : 'Forget preview',
                              ),
                            ),
                          ],
                        ] else ...[
                          FilledButton.icon(
                            style: FilledButton.styleFrom(
                              padding: const EdgeInsets.symmetric(
                                vertical: 18,
                                horizontal: 24,
                              ),
                            ),
                            onPressed: busy ? null : () => _connect(context),
                            icon: const Icon(Icons.add_link),
                            label: Text(
                              busy
                                  ? 'Connecting…'
                                  : unavailable
                                  ? 'Retry connection'
                                  : 'Connect cluster',
                            ),
                          ),
                          if (session?.hasTrust == true ||
                              ble?.hasSelection == true)
                            TextButton(
                              onPressed: busy ? null : () => _forget(context),
                              child: Text(
                                ble != null
                                    ? 'Forget app selection'
                                    : 'Forget preview',
                              ),
                            ),
                          const SizedBox(height: 16),
                          Text(
                            'Keep your cluster switched on and nearby.',
                            textAlign: TextAlign.center,
                            style: Theme.of(context).textTheme.bodyMedium
                                ?.copyWith(color: colors.onSurfaceVariant),
                          ),
                        ],
                      ],
                    ),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
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

  String get _navigationLabel => switch (status.navigation) {
    FeatureReadiness.ready => 'Choose where your next ride takes you.',
    FeatureReadiness.notImplemented => 'Navigation is coming next.',
    FeatureReadiness.unsupported =>
      'Navigation is not supported by this cluster.',
    FeatureReadiness.permissionRequired =>
      'Location permission is needed for navigation.',
    FeatureReadiness.unavailable => 'Navigation is currently unavailable.',
    FeatureReadiness.stale => 'Waiting for a fresh navigation update.',
  };
}
