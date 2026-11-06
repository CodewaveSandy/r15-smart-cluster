/// Native observations only. No Bluetooth, notification or widget ownership.
class TurnGuidance {
  const TurnGuidance([this.data = const {}]);
  final Map<String, dynamic> data;
  factory TurnGuidance.fromNative(Object? value) => TurnGuidance(
    value is Map ? Map<String, dynamic>.unmodifiable(value) : const {},
  );
  Map<String, dynamic> get transport => data['transport'] is Map
      ? Map<String, dynamic>.from(data['transport'] as Map)
      : const {};
  bool get enabled => data['enabled'] == true;
  bool get access => data['access'] == true;
  bool get consent => data['consent'] == true;
  bool get listener => data['listener'] == true;
  bool? get mapsInstalled => data['mapsInstalled'] as bool?;
  bool get confirmed => transport['acknowledged'] == true;
  bool get fresh =>
      enabled &&
      access &&
      listener &&
      confirmed &&
      ageSeconds != null &&
      ageSeconds! < 30 &&
      transport['live'] == true &&
      transport['timingReady'] == true &&
      transport['displayQuality'] == 2 &&
      data['truncated'] != true;
  String get instruction => data['instruction'] as String? ?? '';
  String get road => data['road'] as String? ?? '';
  int? get nextMetres => (data['nextMetres'] as num?)?.toInt();
  int? get ageSeconds {
    final ms = (data['ageMs'] as num?)?.toInt();
    return ms == null ? null : ms ~/ 1000;
  }

  String get problem =>
      transport['problem'] as String? ?? data['problem'] as String? ?? '';
  String get status {
    if (!enabled) {
      if (transport['state'] == 'clearing') {
        return 'Clearing guidance on cluster';
      }
      if (data['stopReason'] == 3) {
        return access
            ? 'Access restored — enable guidance again'
            : 'Notification access revoked';
      }
      if (data['stopReason'] == 6) {
        return listener
            ? 'Listener restored — enable guidance again'
            : 'Notification listener disconnected';
      }
      return 'Guidance is off';
    }
    if (!access) return 'Notification access required';
    if (!listener) return 'Notification listener unavailable';
    if (data['replacementPending'] == true) {
      return 'Maps is replacing its notification';
    }
    return switch (transport['state']) {
      'unsupported' => 'Cluster guidance unavailable',
      'waitingDisplay' => 'Waiting for display connection',
      'connecting' => 'Waiting for cluster connection',
      'opening' || 'timing' => 'Preparing cluster guidance',
      'clearing' => 'Clearing the previous instruction',
      'error' => 'Sharing needs retry',
      'lastUpdate' => 'Last Maps update — guidance is neutral',
      'updatePending' => 'Guidance update pending',
      'ageUnknown' => 'Shared text — source age unknown',
      'sourceUnsupported' => 'Unsupported Maps notification',
      'linkLost' => 'Display guidance connection lost',
      _ =>
        confirmed
            ? 'Accepted by cluster'
            : instruction.isEmpty
            ? 'Start navigation in Google Maps'
            : 'Sharing current instruction',
    };
  }

  Map<String, dynamic> get raw => data['raw'] is Map
      ? Map<String, dynamic>.from(data['raw'] as Map)
      : const {};
}
