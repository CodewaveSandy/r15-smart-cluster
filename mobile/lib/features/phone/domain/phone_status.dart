/// Feature readiness is independent of the physical connection.
enum FeatureReadiness {
  notImplemented,
  unsupported,
  permissionRequired,
  unavailable,
  stale,
  ready,
}

/// An immutable presentation snapshot, not a wire encoding or OS permission.
/// No connected state is supplied until a real session service is implemented.
final class PhoneStatus {
  const PhoneStatus({
    required this.navigation,
    required this.notifications,
    required this.music,
  });

  const PhoneStatus.foundation()
    : navigation = FeatureReadiness.notImplemented,
      notifications = FeatureReadiness.notImplemented,
      music = FeatureReadiness.notImplemented;

  final FeatureReadiness navigation;
  final FeatureReadiness notifications;
  final FeatureReadiness music;
}
