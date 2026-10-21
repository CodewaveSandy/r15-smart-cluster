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
/// Only confirmed session observations may supply a connected state.
final class PhoneStatus {
  const PhoneStatus({
    this.connection = ClusterConnection.disconnected,
    this.clusterName,
    required this.navigation,
    required this.notifications,
    required this.music,
  });

  const PhoneStatus.foundation()
    : connection = ClusterConnection.disconnected,
      clusterName = null,
      navigation = FeatureReadiness.notImplemented,
      notifications = FeatureReadiness.notImplemented,
      music = FeatureReadiness.notImplemented;

  final ClusterConnection connection;
  final String? clusterName;
  final FeatureReadiness navigation;
  final FeatureReadiness notifications;
  final FeatureReadiness music;
}
