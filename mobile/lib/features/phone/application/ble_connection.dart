import 'dart:async';

import 'package:flutter/widgets.dart';
import 'package:flutter/services.dart';

import '../data/ble_pairing.dart';
import '../data/nearby_platform.dart';
import '../domain/phone_status.dart';

/// Foreground connection policy. Native Android owns the one physical GATT.
class BleConnection extends ChangeNotifier with WidgetsBindingObserver {
  BleConnection(this._platform) {
    WidgetsBinding.instance.addObserver(this);
    _subscription = _platform.changes.listen(
      _observe,
      onError: (Object _) {
        error = 'Bluetooth status is unavailable. Restore access and retry.';
        _live = false;
        notifyListeners();
      },
    );
  }
  final BlePairing _platform;
  StreamSubscription<Map<String, dynamic>>? _subscription;
  Timer? _retry;
  NearbyDevice? _selected;
  bool _live = false, _bonded = false, _busy = false;
  bool _savedAttempt = false, _retryAllowed = true;
  bool _foreground = true, _access = false, _stopped = false, _disposed = false;
  int _attempt = 0, _generation = 0;
  String? error;
  bool get hasSelection => _selected != null;
  bool get busy => _busy;
  bool get live => _live;
  bool get bonded => _bonded;
  PhoneStatus get status => PhoneStatus(
    connection: _live
        ? ClusterConnection.connected
        : _busy
        ? ClusterConnection.connecting
        : error != null
        ? ClusterConnection.unavailable
        : ClusterConnection.disconnected,
    clusterName: _selected?.name,
    bluetoothLinkOnly: true,
    navigation: FeatureReadiness.notImplemented,
    notifications: FeatureReadiness.notImplemented,
    music: FeatureReadiness.notImplemented,
  );
  void _observe(Map<String, dynamic> value) {
    if (_disposed) return;
    final wasLive = _live;
    final address = value['address'];
    final name = value['name'];
    _selected = address is String && name is String
        ? NearbyDevice(address: address, name: name)
        : null;
    _bonded = value['bonded'] == true;
    _retryAllowed = value['retryAllowed'] != false;
    _live = value['state'] == 'connected';
    error = value['error'] as String?;
    if (_live) {
      _attempt = 0;
      _retry?.cancel();
    }
    notifyListeners();
    if (wasLive && !_live && !_busy) _schedule();
  }

  Future<void> accessReady() async {
    _access = true;
    try {
      _observe(await _platform.status());
    } on Object {
      return;
    }
    onForeground();
  }

  Future<void> onForeground() async {
    if (!_foreground || !_access || _stopped || _busy || _live) return;
    // Refresh OS bond state, including after Android Bluetooth settings edits.
    try {
      _observe(await _platform.status());
    } on Object {
      return;
    }
    if (hasSelection && _bonded && !_live && !_busy) {
      _schedule();
    }
  }

  void accessBlocked() {
    _access = false;
    _retry?.cancel();
    ++_generation;
    _busy = false;
    _platform.disconnect().catchError((Object _) {});
  }

  Future<void> select(NearbyDevice device) async {
    if (_busy) return;
    _retry?.cancel();
    _stopped = false;
    _attempt = 0;
    await _connect(device, saved: false);
  }

  Future<void> reconnect() async {
    if (_busy || !hasSelection) return;
    _stopped = false;
    _attempt = 0;
    _retry?.cancel();
    await _connect(_selected!, saved: true);
  }

  Future<void> _connect(NearbyDevice device, {required bool saved}) async {
    final generation = ++_generation;
    _busy = true;
    _savedAttempt = saved;
    error = null;
    notifyListeners();
    try {
      await _platform.pair(device, saved: saved);
    } on Object catch (failure) {
      if (_disposed || generation != _generation) return;
      error = failure is PlatformException
          ? failure.message
          : 'Bluetooth connection failed.';
    } finally {
      if (!_disposed && generation == _generation) {
        _busy = false;
        notifyListeners();
        if (saved && !_live) _schedule();
      }
    }
  }

  void _schedule() {
    if (_disposed ||
        !_retryAllowed ||
        _stopped ||
        !_foreground ||
        !_access ||
        _busy ||
        _live ||
        !_bonded ||
        !hasSelection ||
        _retry?.isActive == true) {
      return;
    }
    if (_attempt >= 4) {
      error ??= 'Reconnect stopped. Check cluster power and bonds, then tap Retry connection.';
      notifyListeners();
      return;
    }
    final seconds = 1 << _attempt++;
    _retry = Timer(
      Duration(seconds: seconds),
      () => _connect(_selected!, saved: true),
    );
  }

  Future<void> disconnect() async {
    _stopped = true;
    _retry?.cancel();
    ++_generation;
    _busy = false;
    await _platform.disconnect();
  }

  Future<void> forget() async {
    await disconnect();
    await _platform.forgetSelection();
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    _foreground = state == AppLifecycleState.resumed;
    if (_foreground) {
      onForeground();
    } else {
      _retry?.cancel();
      // An OS pairing prompt may pause Flutter: preserve deliberate first pairing.
      if (_busy && _savedAttempt && state != AppLifecycleState.inactive) {
        ++_generation;
        _busy = false;
        _platform.disconnect().catchError((Object _) {});
      }
    }
  }

  @override
  void dispose() {
    _disposed = true;
    ++_generation;
    _retry?.cancel();
    _subscription?.cancel();
    WidgetsBinding.instance.removeObserver(this);
    _platform.disconnect().catchError((Object _) {});
    super.dispose();
  }
}
