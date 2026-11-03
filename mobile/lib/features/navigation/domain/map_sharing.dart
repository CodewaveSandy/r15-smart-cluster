/// Semantic native observations; no Flutter/plugins, raw packets or stored GPS.
final class MapSharing {
  const MapSharing({
    this.state = 'notConnected',
    this.message = 'Connect your cluster to share maps',
    this.sharing = false,
    this.accepted = 0,
    this.total = 0,
    this.clusterActive = false,
    this.clusterLocation,
    this.quality = 'waiting',
    this.latitude,
    this.longitude,
    this.accuracy,
    this.bearing,
    this.insideRegion = false,
    this.error,
  });
  factory MapSharing.fromNative(Map<String, dynamic> value) {
    final raw = value['map'];
    final m = raw is Map ? raw : const <Object?, Object?>{};
    final rawFix = m['location'];
    final fix = rawFix is Map ? rawFix : const <Object?, Object?>{};
    double? number(Object? n) => n is num && n.isFinite ? n.toDouble() : null;
    final quality = fix['quality'] as String? ?? 'waiting';
    final fresh = quality == 'fresh' && value['mapSharing'] == true;
    return MapSharing(
      state: m['state'] as String? ?? 'notConnected',
      message: m['message'] as String? ?? 'Connect your cluster to share maps',
      sharing: value['mapSharing'] == true,
      accepted: (m['acceptedBytes'] as num?)?.toInt() ?? 0,
      total: (m['totalBytes'] as num?)?.toInt() ?? 0,
      clusterActive: m['clusterActive'] == true,
      clusterLocation: (m['clusterLocation'] as num?)?.toInt(),
      quality: quality,
      latitude: fresh ? number(fix['latitude']) : null,
      longitude: fresh ? number(fix['longitude']) : null,
      accuracy: fresh ? number(fix['accuracyMeters']) : null,
      bearing: fresh ? number(fix['bearing']) : null,
      insideRegion: fix['insideRegion'] == true,
      error: value['mapError'] as String?,
    );
  }
  final String state, message, quality;
  final bool sharing, clusterActive, insideRegion;
  final int accepted, total;
  final int? clusterLocation;
  final double? latitude, longitude, accuracy, bearing;
  final String? error;
  String get clusterLocationLabel => switch (clusterLocation) {
    0 => 'Display report · waiting for timing',
    1 => 'Display report · waiting for location',
    2 => 'Display report · location fresh',
    3 => 'Display report · location stale',
    4 => 'Display report · accuracy low',
    5 => 'Display report · outside downloaded area',
    6 => 'Display report · sharing stopped',
    7 => 'Display report · location invalid',
    _ => 'Waiting for display location status',
  };
  double? get progress => total > 0 ? (accepted / total).clamp(0, 1) : null;
  String get locationLabel => switch (quality) {
    'fresh' =>
      insideRegion
          ? 'Current location · ${accuracy?.ceil() ?? '—'} m accuracy'
          : 'Outside downloaded Kolkata area · cluster pointer hidden',
    'accuracyLow' =>
      'Location accuracy low · allow Precise location and move outdoors',
    'stale' => 'Location stale · pointer hidden',
    'permissionRequired' => 'Location permission needed',
    'gpsDisabled' => 'Turn on phone location',
    'providerUnsupported' => 'Google Play services location unavailable',
    'providerUnavailable' => 'Location provider unavailable',
    'invalid' => 'Location invalid or simulated · pointer hidden',
    'stopped' => 'Location sharing stopped',
    _ =>
      sharing
          ? 'Waiting for a fresh location'
          : 'Location is shared only after you start',
  };
}
