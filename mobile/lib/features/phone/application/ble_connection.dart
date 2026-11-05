import 'dart:async';

import 'package:flutter/widgets.dart';
import 'package:flutter/services.dart';

import '../../home/domain/bike_telemetry.dart';
import '../data/ble_pairing.dart';
import '../data/ble_access.dart';
import '../data/nearby_platform.dart';
import '../domain/phone_status.dart';

/// Flutter observes the native service; no page/isolate owns the physical link.
class BleConnection extends ChangeNotifier with WidgetsBindingObserver {
  BleConnection(this._platform) {
    WidgetsBinding.instance.addObserver(this);
    _subscription = _platform.changes.listen(
      _observe,
      onError: (Object _) {
        error = 'Bluetooth status is unavailable. Restore access and retry.';
        _live = false;
        telemetry = const BikeTelemetry.unavailable();
        notifyListeners();
      },
    );
    unawaited(_checkExistingAccess());
  }
  Future<void> _checkExistingAccess() async {
    try {
      final access = await BleAccess.check();
      if (_disposed) return;
      if (access.ready) {
        await accessReady();
      } else {
        _access = false;
      }
    } on Object {
      /* Home remains available while Bluetooth access is unavailable. */
    }
  }

  final BlePairing _platform;
  StreamSubscription<Map<String, dynamic>>? _subscription;
  NearbyDevice? _selected;
  bool _live = false, _bonded = false, _busy = false;
  bool _commandPending = false, _serviceActive = false;
  bool _foreground = true, _access = false, _stopped = false, _disposed = false;
  bool _autoRequested = false;
  int _generation = 0;
  Completer<void>? _connectionResult;
  bool _nativeStarted = false;
  String? error;
  BikeTelemetry telemetry = const BikeTelemetry.unavailable();
  PhoneMetadataState _metadata = PhoneMetadataState.notConnected;
  bool get hasSelection => _selected != null;
  bool get busy => _busy || _commandPending;
  bool get live => _live;
  bool get bonded => _bonded;
  bool get serviceActive => _serviceActive;
  PhoneStatus get status => PhoneStatus(
    connection: _live
        ? ClusterConnection.connected
        : busy
        ? ClusterConnection.connecting
        : error != null
        ? ClusterConnection.unavailable
        : ClusterConnection.disconnected,
    clusterName: _selected?.name,
    bluetoothLinkOnly: true,
    metadata: _metadata,
    navigation: FeatureReadiness.notImplemented,
    notifications: FeatureReadiness.notImplemented,
    music: FeatureReadiness.notImplemented,
  );

  void _observe(Map<String, dynamic> value) {
    if (_disposed) return;
    final address = value['address'];
    final name = value['name'];
    _selected = address is String && name is String
        ? NearbyDevice(address: address, name: name)
        : null;
    _bonded = value['bonded'] == true;
    _live = value['state'] == 'connected';
    _serviceActive = value['serviceActive'] == true;
    _stopped = value['stopped'] == true;
    _busy =
        value['state'] == 'connecting' ||
        value['state'] == 'pairing' ||
        value['reconnecting'] == true;
    _metadata = _live
        ? switch (value['metadataState']) {
            'synced' => PhoneMetadataState.synced,
            'syncing' => PhoneMetadataState.syncing,
            'unsupported' => PhoneMetadataState.unsupported,
            _ => PhoneMetadataState.unavailable,
          }
        : PhoneMetadataState.notConnected;
    telemetry = _live
        ? telemetryFromNative(value['telemetry'])
        : const BikeTelemetry.unavailable();
    error = value['error'] as String?;
    if (_busy || _serviceActive) _nativeStarted = true;
    if (_live ||
        (error != null && value['reconnecting'] != true) ||
        (_nativeStarted && _stopped && !_serviceActive)) {
      final result = _connectionResult;
      if (result != null && !result.isCompleted) result.complete();
    }
    notifyListeners();
  }

  Future<void> accessReady() async {
    _access = true;
    await onForeground();
  }

  Future<void> onForeground() async {
    if (!_foreground || !_access || _disposed) return;
    try {
      _observe(await _platform.status());
    } on Object {
      return;
    }
    // One foreground bootstrap only. All subsequent retry/cooldown policy is
    // native, including while this observer or Flutter engine does not exist.
    if (_foreground &&
        _access &&
        !_disposed &&
        !_autoRequested &&
        !_stopped &&
        !busy &&
        !_live &&
        !_serviceActive &&
        hasSelection &&
        _bonded) {
      _autoRequested = true;
      await _connect(_selected!, saved: true);
    }
  }

  void accessBlocked() {
    _access = false;
    ++_generation;
    _busy = false;
    _commandPending = false;
    telemetry = const BikeTelemetry.unavailable();
    _platform.disconnect().catchError((Object _) {});
  }

  Future<void> select(NearbyDevice device) async {
    if (busy) return;
    await _connect(device, saved: false);
  }

  Future<void> reconnect() async {
    if (busy || !hasSelection) return;
    await _connect(_selected!, saved: true);
  }

  Future<void> _connect(NearbyDevice device, {required bool saved}) async {
    final generation = ++_generation;
    _commandPending = true;
    _nativeStarted = false;
    final outcome = Completer<void>();
    _connectionResult = outcome;
    _stopped = false;
    error = null;
    notifyListeners();
    try {
      // Acknowledges command delivery. Native events establish actual state.
      await _platform.pair(device, saved: saved);
      await outcome.future.timeout(const Duration(seconds: 55));
    } on Object catch (failure) {
      if (_disposed || generation != _generation) return;
      error = failure is PlatformException
          ? failure.message
          : 'Bluetooth connection failed.';
    } finally {
      if (!_disposed && generation == _generation) {
        _commandPending = false;
        _connectionResult = null;
        notifyListeners();
      }
    }
  }

  Future<void> disconnect() async {
    _stopped = true;
    ++_generation;
    _commandPending = false;
    _busy = false;
    await _platform.disconnect();
    _observe(await _platform.status());
  }

  Future<void> forget() async {
    await disconnect();
    await _platform.forgetSelection();
    _observe(await _platform.status());
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    _foreground = state == AppLifecycleState.resumed;
    if (_foreground) unawaited(_checkExistingAccess());
  }

  @override
  void dispose() {
    _disposed = true;
    final result = _connectionResult;
    if (result != null && !result.isCompleted) result.complete();
    ++_generation;
    _subscription?.cancel();
    WidgetsBinding.instance.removeObserver(this);
    // Detaching the UI must never send Disconnect to the started service.
    super.dispose();
  }
}
