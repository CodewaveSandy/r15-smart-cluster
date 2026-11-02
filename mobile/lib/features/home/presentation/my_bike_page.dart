import 'package:flutter/material.dart';

import '../../../app/theme/companion_theme.dart';
import '../../phone/domain/phone_status.dart';
import '../domain/bike_telemetry.dart';

/// Approved Concept 01 Home. Renders observations and emits navigation intents.
class MyBikePage extends StatelessWidget {
  const MyBikePage({
    required this.status,
    required this.onNavigate,
    required this.onConnection,
    this.telemetry = const BikeTelemetry.unavailable(),
    super.key,
  });
  final PhoneStatus status;
  final BikeTelemetry telemetry;
  final VoidCallback onNavigate, onConnection;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final colors = theme.colorScheme;
    return SingleChildScrollView(
      key: const PageStorageKey('my-bike-scroll'),
      padding: const EdgeInsets.fromLTRB(
        CompanionTokens.pagePadding,
        8,
        CompanionTokens.pagePadding,
        24,
      ),
      child: Center(
        child: ConstrainedBox(
          constraints: const BoxConstraints(
            maxWidth: CompanionTokens.contentWidth,
          ),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              Text('Yamaha R15 V4', style: theme.textTheme.headlineMedium),
              const SizedBox(height: 10),
              Align(
                alignment: Alignment.centerLeft,
                child: _ConnectionPill(status: status, onTap: onConnection),
              ),
              const SizedBox(height: 8),
              AspectRatio(
                aspectRatio: CompanionTokens.homeHeroAspectRatio,
                child: Stack(
                  alignment: Alignment.center,
                  children: [
                    Positioned(
                      left: 30,
                      right: 30,
                      bottom: 10,
                      child: Container(
                        height: 60,
                        decoration: BoxDecoration(
                          gradient: RadialGradient(
                            radius: 1.1,
                            colors: [
                              colors.primary.withValues(alpha: .13),
                              colors.primary.withValues(alpha: 0),
                            ],
                          ),
                          borderRadius: BorderRadius.circular(100),
                        ),
                      ),
                    ),
                    Image.asset(
                      'assets/illustrations/bike-home.png',
                      fit: BoxFit.contain,
                      cacheWidth: 1100,
                      excludeFromSemantics: true,
                      errorBuilder: (context, error, stack) => Icon(
                        Icons.two_wheeler_outlined,
                        size: 140,
                        color: colors.primary,
                      ),
                    ),
                  ],
                ),
              ),
              const SizedBox(height: 8),
              Text(
                'FROM YOUR CLUSTER',
                style: theme.textTheme.labelSmall?.copyWith(
                  letterSpacing: 2.2,
                  color: colors.onSurfaceVariant,
                ),
              ),
              const SizedBox(height: 12),
              LayoutBuilder(
                builder: (context, constraints) {
                  final single =
                      constraints.maxWidth < 280 ||
                      MediaQuery.textScalerOf(context).scale(14) > 22;
                  final tiles = [
                    _TelemetryTile(
                      label: 'SPEED',
                      icon: Icons.speed_outlined,
                      reading: telemetry.speed,
                    ),
                    _TelemetryTile(
                      label: 'GEAR',
                      icon: Icons.settings_outlined,
                      reading: telemetry.gear,
                    ),
                    _TelemetryTile(
                      label: 'FUEL',
                      icon: Icons.local_gas_station_outlined,
                      reading: telemetry.fuel,
                    ),
                    _TelemetryTile(
                      label: 'BIKE BATTERY',
                      icon: Icons.battery_charging_full_outlined,
                      reading: telemetry.battery,
                    ),
                  ];
                  return Wrap(
                    spacing: CompanionTokens.telemetryGap,
                    runSpacing: CompanionTokens.telemetryGap,
                    children: [
                      for (final tile in tiles)
                        SizedBox(
                          width: single
                              ? constraints.maxWidth
                              : (constraints.maxWidth -
                                        CompanionTokens.telemetryGap) /
                                    2,
                          child: tile,
                        ),
                    ],
                  );
                },
              ),
              const SizedBox(height: 10),
              Text(
                status.connection == ClusterConnection.connected
                    ? 'Only supported, fresh cluster readings are shown.'
                    : 'Connect your cluster to receive available bike readings.',
                style: theme.textTheme.bodySmall?.copyWith(
                  color: colors.onSurfaceVariant,
                ),
              ),
              const SizedBox(height: 22),
              Card(
                margin: EdgeInsets.zero,
                clipBehavior: Clip.antiAlias,
                child: InkWell(
                  onTap: onNavigate,
                  child: Padding(
                    padding: const EdgeInsets.all(18),
                    child: Row(
                      children: [
                        Icon(
                          Icons.near_me_rounded,
                          color: colors.primary,
                          size: 34,
                        ),
                        const SizedBox(width: 16),
                        Expanded(
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              Text(
                                'Where to next?',
                                style: theme.textTheme.titleMedium?.copyWith(
                                  fontWeight: FontWeight.w600,
                                ),
                              ),
                              const SizedBox(height: 4),
                              Text(
                                'Explore navigation',
                                style: theme.textTheme.bodyMedium?.copyWith(
                                  color: colors.onSurfaceVariant,
                                ),
                              ),
                            ],
                          ),
                        ),
                        const SizedBox(width: 8),
                        DecoratedBox(
                          decoration: BoxDecoration(
                            color: colors.primary,
                            shape: BoxShape.circle,
                          ),
                          child: Padding(
                            padding: const EdgeInsets.all(8),
                            child: Icon(
                              Icons.chevron_right,
                              color: colors.onPrimary,
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

class _ConnectionPill extends StatelessWidget {
  const _ConnectionPill({required this.status, required this.onTap});
  final PhoneStatus status;
  final VoidCallback onTap;
  @override
  Widget build(BuildContext context) {
    final colors = Theme.of(context).colorScheme;
    final connected = status.connection == ClusterConnection.connected;
    final label = switch (status.connection) {
      ClusterConnection.connected =>
        status.bluetoothLinkOnly ? 'Bluetooth connected' : 'Cluster connected',
      ClusterConnection.connecting => 'Connecting to cluster',
      ClusterConnection.disconnected => 'Cluster disconnected',
      ClusterConnection.unavailable => 'Connection unavailable',
    };
    final accent = connected
        ? CompanionTheme.connectedColor(Theme.of(context).brightness)
        : colors.onSurfaceVariant;
    return Semantics(
      button: true,
      label: '$label. Manage cluster connection',
      child: ExcludeSemantics(
        child: Material(
          color: colors.surfaceContainerLow,
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(10),
            side: BorderSide(color: colors.outlineVariant),
          ),
          child: InkWell(
            onTap: onTap,
            borderRadius: BorderRadius.circular(10),
            child: Padding(
              padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 13),
              child: Row(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Icon(
                    connected ? Icons.circle : Icons.bluetooth,
                    size: 13,
                    color: accent,
                  ),
                  const SizedBox(width: 8),
                  Flexible(
                    child: Text(
                      label,
                      style: Theme.of(context).textTheme.labelMedium
                          ?.copyWith(color: accent),
                    ),
                  ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }
}

class _TelemetryTile extends StatelessWidget {
  const _TelemetryTile({
    required this.label,
    required this.icon,
    required this.reading,
  });
  final String label;
  final IconData icon;
  final BikeReading<num> reading;
  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final colors = theme.colorScheme;
    final available =
        reading.quality == ReadingQuality.available &&
        reading.value != null &&
        reading.value!.isFinite;
    final unit = switch (reading.unit) {
      ReadingUnit.kilometresPerHour => 'km/h',
      ReadingUnit.gear => '',
      ReadingUnit.percent => '%',
      ReadingUnit.volts => 'V',
      ReadingUnit.revolutionsPerMinute => 'rpm',
    };
    final value = available
        ? (reading.unit == ReadingUnit.gear && reading.value == 0
              ? 'N'
              : reading.unit == ReadingUnit.volts
              ? reading.value!.toStringAsFixed(1)
              : reading.value!.toStringAsFixed(0))
        : '—';
    final quality = available
        ? 'From cluster'
        : switch (reading.quality) {
            ReadingQuality.stale => 'Stale',
            ReadingQuality.unsupported => 'Unsupported',
            ReadingQuality.invalid => 'Invalid',
            _ => 'Unavailable',
          };
    return Semantics(
      label: '$label, $value $unit, $quality',
      child: ExcludeSemantics(
        child: Card(
          margin: EdgeInsets.zero,
          child: Padding(
            padding: const EdgeInsets.all(14),
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Icon(icon, color: colors.onSurfaceVariant, size: 25),
                const SizedBox(width: 12),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        label,
                        style: theme.textTheme.labelSmall?.copyWith(
                          color: colors.onSurfaceVariant,
                          letterSpacing: .8,
                        ),
                      ),
                      const SizedBox(height: 5),
                      Text.rich(
                        TextSpan(
                          children: [
                            TextSpan(
                              text: value,
                              style: theme.textTheme.headlineSmall?.copyWith(
                                fontWeight: FontWeight.w600,
                              ),
                            ),
                            if (unit.isNotEmpty)
                              TextSpan(
                                text: ' $unit',
                                style: theme.textTheme.bodySmall,
                              ),
                          ],
                        ),
                      ),
                      const SizedBox(height: 3),
                      Text(
                        quality,
                        style: theme.textTheme.labelSmall?.copyWith(
                          color: colors.onSurfaceVariant,
                        ),
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
