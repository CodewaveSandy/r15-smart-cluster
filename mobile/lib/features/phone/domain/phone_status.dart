/// Feature readiness is independent of the physical connection.
enum FeatureReadiness {
  notImplemented,
  unsupported,
  permissionRequired,
  unavailable,
  stale,
  ready,
}

enum ClusterConnection { disconnected, connecting, connected, unavailable }

/// An immutable presentation snapshot, not a wire encoding or OS permission.
/// Connected requires a confirmed live observation. bluetoothLinkOnly keeps
/// physical GATT observations separate from authenticated application sessions.
final class PhoneStatus {
  const PhoneStatus({
    this.connection = ClusterConnection.disconnected,
    this.clusterName,
    this.bluetoothLinkOnly = false,
    required this.navigation,
    required this.notifications,
    required this.music,
  });

  const PhoneStatus.foundation()
    : connection = ClusterConnection.disconnected,
      clusterName = null,
      bluetoothLinkOnly = false,
      navigation = FeatureReadiness.notImplemented,
      notifications = FeatureReadiness.notImplemented,
      music = FeatureReadiness.notImplemented;

  final ClusterConnection connection;
  final String? clusterName;
  final bool bluetoothLinkOnly;
  final FeatureReadiness navigation;
  final FeatureReadiness notifications;
  final FeatureReadiness music;
}
