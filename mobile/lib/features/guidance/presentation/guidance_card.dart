import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import '../domain/turn_guidance.dart';

class GuidanceCard extends StatefulWidget {
  const GuidanceCard({
    required this.state,
    required this.connected,
    required this.onAction,
    required this.onConnect,
    super.key,
  });
  final TurnGuidance state;
  final bool connected;
  final Future<void> Function(String) onAction;
  final VoidCallback onConnect;
  @override
  State<GuidanceCard> createState() => _GuidanceCardState();
}

class _GuidanceCardState extends State<GuidanceCard> {
  bool _busy = false;
  Future<void> _action(String action) async {
    if (_busy) return;
    if (action == 'access') {
      final accepted = await showDialog<bool>(
        context: context,
        builder: (context) => AlertDialog(
          title: const Text('Google Maps notification access'),
          content: const Text(
            'Android grants access to notifications from all apps. R15 reads only Google Maps notifications from this profile while guidance is enabled. Current instructions stay in memory and are shared only with your connected cluster. No notification history is saved.\n\nGoogle Maps continues to own navigation, location and audio. The first profile supports explicit English instructions; some Maps versions may expose only text or no usable guidance.',
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(context, false),
              child: const Text('Cancel'),
            ),
            FilledButton(
              onPressed: () => Navigator.pop(context, true),
              child: const Text('Open settings'),
            ),
          ],
        ),
      );
      if (accepted != true || !mounted) return;
    }
    setState(() => _busy = true);
    try {
      await widget.onAction(action);
    } on PlatformException catch (error) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text(
              error.message ?? 'Guidance action unavailable. Retry.',
            ),
          ),
        );
      }
    } catch (_) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('Guidance action unavailable. Retry.')),
        );
      }
    } finally {
      if (mounted) setState(() => _busy = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    final s = widget.state;
    final theme = Theme.of(context);
    final colors = theme.colorScheme;
    return Card(
      margin: EdgeInsets.zero,
      child: Padding(
        padding: const EdgeInsets.all(18),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Row(
              children: [
                Icon(Icons.assistant_direction_outlined, color: colors.primary),
                const SizedBox(width: 10),
                Expanded(
                  child: Text(
                    'Google Maps guidance',
                    style: theme.textTheme.titleMedium,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 12),
            Text(
              s.status,
              style: theme.textTheme.bodyLarge?.copyWith(
                color: s.fresh ? colors.primary : colors.onSurface,
              ),
            ),
            if (s.problem.isNotEmpty) ...[
              const SizedBox(height: 6),
              Text(s.problem, style: theme.textTheme.bodySmall),
            ],
            if (s.instruction.isNotEmpty) ...[
              const Divider(height: 24),
              Text(s.instruction, style: theme.textTheme.titleSmall),
              if (s.road.isNotEmpty) Text(s.road),
              Text(
                'Next: ${s.nextMetres == null ? '—' : '${s.nextMetres} m'} · ${s.ageSeconds == null ? 'Age unknown' : 'Last observed instruction change ${s.ageSeconds}s ago'}',
                style: theme.textTheme.bodySmall,
              ),
              if ((s.data['summary'] as String?)?.isNotEmpty == true)
                Text(
                  'Reported: ${s.data['summary']}',
                  style: theme.textTheme.bodySmall,
                ),
              if (s.data['remainingMetres'] is num)
                Text(
                  'Reported remaining: ${s.data['remainingMetres']} m',
                  style: theme.textTheme.bodySmall,
                ),
              if (s.data['remainingSeconds'] is num)
                Text(
                  'Reported time: ${((s.data['remainingSeconds'] as num).toInt() + 59) ~/ 60} min',
                  style: theme.textTheme.bodySmall,
                ),
              if (s.confirmed)
                Text(
                  s.transport['displayVisible'] == true
                      ? 'Current model visible on cluster'
                      : 'Cluster accepted it; its current screen may hide the card',
                  style: theme.textTheme.bodySmall,
                ),
            ],
            const SizedBox(height: 12),
            Wrap(
              spacing: 8,
              runSpacing: 8,
              children: [
                if (!s.access || !s.consent)
                  FilledButton(
                    onPressed: _busy ? null : () => _action('access'),
                    child: const Text('Review notification access'),
                  )
                else if (!s.listener)
                  FilledButton(
                    onPressed: _busy ? null : () => _action('rebind'),
                    child: const Text('Reconnect listener'),
                  )
                else if (!widget.connected)
                  FilledButton(
                    onPressed: widget.onConnect,
                    child: const Text('Connect cluster'),
                  )
                else if (!s.enabled)
                  FilledButton(
                    onPressed: _busy ? null : () => _action('enable'),
                    child: const Text('Enable guidance'),
                  )
                else
                  OutlinedButton(
                    onPressed: _busy ? null : () => _action('stop'),
                    child: const Text('Stop guidance'),
                  ),
                OutlinedButton(
                  onPressed: _busy || s.mapsInstalled != true
                      ? null
                      : () => _action('openMaps'),
                  child: const Text('Open Google Maps'),
                ),
                if (s.enabled)
                  TextButton(
                    onPressed: _busy ? null : () => _action('retry'),
                    child: const Text('Retry sharing'),
                  ),
                if (s.access)
                  TextButton(
                    onPressed: _busy ? null : () => _action('access'),
                    child: const Text('Notification access'),
                  ),
              ],
            ),
            if (_busy)
              const Padding(
                padding: EdgeInsets.only(top: 10),
                child: LinearProgressIndicator(),
              ),
            const SizedBox(height: 10),
            Text(
              s.mapsInstalled != false
                  ? 'Start your route in Google Maps. Unknown or old instructions stay neutral; no map download or API key is needed.'
                  : 'Install or enable Google Maps on this phone to use guidance.',
              style: theme.textTheme.bodySmall,
            ),
            ExpansionTile(
              tilePadding: EdgeInsets.zero,
              title: const Text('Current notification fields'),
              subtitle: const Text(
                'Private on-phone view for compatibility checks',
              ),
              children: [
                Align(
                  alignment: Alignment.centerLeft,
                  child: Text(
                    'Parser: ${s.data['profile'] ?? 'English public text v1'} · Maps: ${s.data['mapsVersion'] ?? 'Unknown'} · Android: ${s.data['androidApi'] ?? 'Unknown'}',
                    style: theme.textTheme.bodySmall,
                  ),
                ),
                const SizedBox(height: 8),
                Align(
                  alignment: Alignment.centerLeft,
                  child: Padding(
                    padding: const EdgeInsets.only(bottom: 12),
                    child: SelectableText(
                      s.raw.isEmpty
                          ? 'Enable guidance and start Maps navigation to inspect its standard fields. Nothing is logged or saved.'
                          : s.raw.entries
                                .map((entry) => '${entry.key}: ${entry.value}')
                                .join('\n\n'),
                      style: theme.textTheme.bodySmall,
                    ),
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
