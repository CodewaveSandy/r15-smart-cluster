import 'package:flutter/material.dart';

import '../domain/phone_status.dart';

/// Read-only foundation. Connection actions arrive with the session service.
class PhonePage extends StatelessWidget {
  const PhonePage({required this.status, super.key});

  final PhoneStatus status;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('R15 Companion')),
      body: SafeArea(
        child: Center(
          child: ConstrainedBox(
            constraints: const BoxConstraints(maxWidth: 640),
            child: ListView(
              padding: const EdgeInsets.all(24),
              children: [
                Text(
                  'Phone',
                  style: Theme.of(context).textTheme.headlineMedium,
                ),
                const SizedBox(height: 16),
                const Text('No cluster connected'),
                const SizedBox(height: 8),
                const Text(
                  'App foundation. Phone pairing and the Mac preview '
                  'connection are not implemented yet.',
                ),
                const SizedBox(height: 24),
                _FeatureRow(
                  title: 'Navigation',
                  icon: Icons.navigation_outlined,
                  readiness: status.navigation,
                ),
                _FeatureRow(
                  title: 'Notifications',
                  icon: Icons.notifications_outlined,
                  readiness: status.notifications,
                ),
                _FeatureRow(
                  title: 'Music',
                  icon: Icons.music_note_outlined,
                  readiness: status.music,
                ),
                const SizedBox(height: 24),
                const Text(
                  'No phone permissions are requested by this foundation.',
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}

class _FeatureRow extends StatelessWidget {
  const _FeatureRow({
    required this.title,
    required this.icon,
    required this.readiness,
  });

  final String title;
  final IconData icon;
  final FeatureReadiness readiness;

  String get _label => switch (readiness) {
    FeatureReadiness.notImplemented => 'Not implemented',
    FeatureReadiness.unsupported => 'Not supported on this device',
    FeatureReadiness.permissionRequired => 'Permission needed',
    FeatureReadiness.unavailable => 'Unavailable',
    FeatureReadiness.stale => 'Update overdue',
    FeatureReadiness.ready => 'Ready',
  };

  @override
  Widget build(BuildContext context) {
    return ListTile(
      contentPadding: EdgeInsets.zero,
      leading: Icon(icon),
      title: Text(title),
      subtitle: Text(_label),
    );
  }
}
