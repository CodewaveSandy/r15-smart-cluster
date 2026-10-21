import 'package:flutter/material.dart';

import '../../../app/widgets/brand_mark.dart';
import '../domain/phone_status.dart';
import '../application/preview_session.dart';
import 'preview_pairing_sheet.dart';

class PhonePage extends StatelessWidget {
  const PhonePage({required this.status, this.session, super.key});
  final PhoneStatus status;
  final PreviewSession? session;

  void _connect(BuildContext context) {
    if (session != null) {
      if (session!.hasTrust) {
        session!.reconnect();
        return;
      }
      showModalBottomSheet<void>(
        context: context,
        showDragHandle: true,
        isScrollControlled: true,
        builder: (_) => PreviewPairingSheet(session: session!),
      );
      return;
    }
    showModalBottomSheet<void>(
      context: context,
      showDragHandle: true,
      isScrollControlled: true,
      builder: (context) => SafeArea(
        child: SingleChildScrollView(
          padding: const EdgeInsets.fromLTRB(24, 0, 24, 24),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Text(
                'Connect your cluster',
                style: Theme.of(context).textTheme.headlineSmall,
              ),
              const SizedBox(height: 16),
              const Text(
                'Phone pairing is coming next. You will be able to select '
                'your cluster and securely connect it here.',
              ),
              const SizedBox(height: 24),
              FilledButton(
                onPressed: () => Navigator.pop(context),
                child: const Text('Got it'),
              ),
            ],
          ),
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final connected = status.connection == ClusterConnection.connected;
    final busy =
        status.connection == ClusterConnection.connecting ||
        (session?.busy ?? false);
    final unavailable = status.connection == ClusterConnection.unavailable;
    final colors = Theme.of(context).colorScheme;
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
                              ? 'Ready for your next ride.'
                              : 'Your ride. Connected.',
                          textAlign: TextAlign.center,
                          style: Theme.of(context).textTheme.headlineMedium
                              ?.copyWith(fontWeight: FontWeight.w700),
                        ),
                        const SizedBox(height: 16),
                        Text(
                          connected
                              ? '${status.clusterName ?? 'Your cluster'} · Connected'
                              : busy
                              ? 'Connecting to your cluster…'
                              : unavailable
                              ? 'Cluster connection unavailable'
                              : 'Connect your cluster to bring navigation to your ride.',
                          textAlign: TextAlign.center,
                          style: Theme.of(context).textTheme.bodyLarge
                              ?.copyWith(color: colors.onSurfaceVariant),
                        ),
                        const SizedBox(height: 32),
                        if (session != null) ...[
                          if (session!.hasTrust)
                            const Text(
                              'PAIRED CLUSTER',
                              textAlign: TextAlign.center,
                            ),
                          if (session!.error != null)
                            Padding(
                              padding: const EdgeInsets.symmetric(vertical: 12),
                              child: Text(
                                session!.error!,
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
                          if (session != null) ...[
                            TextButton(
                              onPressed: busy ? null : session!.disconnect,
                              child: const Text('Disconnect'),
                            ),
                            TextButton(
                              onPressed: busy ? null : () => _forget(context),
                              child: const Text('Forget preview'),
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
                          if (session?.hasTrust == true)
                            TextButton(
                              onPressed: busy ? null : () => _forget(context),
                              child: const Text('Forget preview'),
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
        title: const Text('Forget Mac preview?'),
        content: const Text(
          'Remove this saved pairing? You can find and pair with your cluster again.',
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
    if (confirmed == true) await session?.forget();
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
