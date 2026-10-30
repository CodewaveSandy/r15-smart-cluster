import 'dart:async';
import 'dart:convert';

import 'package:flutter/services.dart';

import '../data/nearby_platform.dart';

import 'package:flutter/foundation.dart';

import '../data/preview_transport.dart';
import '../domain/phone_status.dart';

/// Coordinates discovery and confirmed sessions; native adapters own radios and protected storage.
final class PreviewSession extends ChangeNotifier {
  PreviewSession(
    this._transport, {
    this.nearby,
    this.peerId,
    this.heartbeatInterval = const Duration(seconds: 2),
  });
  PreviewTransport _transport;
  final NearbyPlatform? nearby;
  final String? peerId;
  final _clock = Stopwatch()..start();
  int _nextDiscovery = 0;
  Future<List<NearbyDevice>> discover({bool interactive = true}) async {
    if (nearby == null) {
      throw const PreviewFailure(
        'Nearby discovery is unavailable on this platform.',
      );
    }
    try {
      final devices = await nearby!.scan(interactive: interactive);
      // Saved Mac sessions rediscover only their advertised service; a matching
      // display name on another BLE accessory is not an endpoint to probe.
      return interactive
          ? devices
          : devices
                .where((device) => device.previewService)
                .toList(growable: false);
    } on PlatformException catch (failure) {
      throw PreviewFailure(
        failure.message ?? 'Allow Nearby devices access and turn on Bluetooth.',
      );
    }
  }

  Future<void> _endpoint(NearbyDevice device, {bool interactive = true}) async {
    final data = await nearby!.read(device, interactive: interactive);
    if (peerId == null || data['id'] != peerId) {
      throw const PreviewFailure(
        'This is not the trusted development cluster. Start the app with the matching preview launcher.',
      );
    }
    if (_transport is TlsPreviewTransport) {
      _transport = (_transport as TlsPreviewTransport).atEndpoint(
        data['host'] as String,
        data['port'] as int,
      );
    }
  }

  Future<String> selectDevice(NearbyDevice device) async {
    await _endpoint(device);
    final offer = await _transport.request('/offer', {});
    if (offer['v'] != 1 ||
        offer['code'] is! String ||
        !RegExp(r'^\d{6}$').hasMatch(offer['code'] as String) ||
        offer['paired'] is! bool) {
      throw const PreviewFailure('Invalid pairing offer.');
    }
    if (offer['paired'] == true) {
      throw const PreviewFailure(
        'This cluster has a saved phone. Forget it on the cluster before pairing another phone.',
      );
    }
    return offer['code'] as String;
  }

  Future<void> restore(String session, {bool reconnectNow = true}) async {
    if (session.length < 16 || session.length > 64) return;
    _session = session;
    if (reconnectNow) await reconnect();
  }

  Future<void> _saveTrust() async {
    final transport = _transport;
    if (nearby == null ||
        transport is! TlsPreviewTransport ||
        peerId == null ||
        _session == null) {
      return;
    }
    await nearby!.saveTrust(
      jsonEncode({
        'v': 1,
        'id': peerId,
        'session': _session,
        'host': transport.host,
        'port': transport.port,
        'certificate': transport.certificate,
        'token': transport.token,
      }),
    );
  }

  final Duration heartbeatInterval;
  PhoneStatus _status = const PhoneStatus.foundation();
  PhoneStatus get status => _status;
  String? _error;
  String? get error => _error;
  String? _session;
  Timer? _heartbeat;
  Future<void> _queue = Future.value();
  bool _disposed = false;
  bool _busy = false;
  bool get busy => _busy;
  bool _polling = false;
  bool _wanted = false;
  bool _needsSave = false;
  bool get hasTrust => _session != null;

  Future<bool> pair(String code) async {
    if (_disposed) return false;
    if (!RegExp(r'^\d{6}$').hasMatch(code)) {
      _error = 'Enter the six-digit code shown in the Mac preview terminal.';
      notifyListeners();
      return false;
    }
    if (_session != null) {
      await reconnect();
    } else {
      await _command('/pair', {'code': code}, connect: true);
    }
    return status.connection == ClusterConnection.connected;
  }

  Future<void> reconnect() => _command('/resume', {}, connect: true);
  void pauseForAccess() {
    _wanted = false;
    _heartbeat?.cancel();
    _setStatus(ClusterConnection.unavailable);
  }

  Future<void> disconnect() => _command('/disconnect', {});
  Future<void> forget() => _command('/forget', {});

  Future<void> _command(
    String operation,
    Map<String, Object?> extra, {
    bool connect = false,
  }) {
    if (_disposed || (operation != '/status' && busy)) return Future.value();
    if (operation != '/status') {
      _busy = true;
      notifyListeners();
    }
    _queue = _queue.then((_) async {
      if (_disposed) return;
      if (operation != '/status') {
        _heartbeat?.cancel();
        _wanted = connect;
        if (connect) _setStatus(ClusterConnection.connecting);
      }
      try {
        Map<String, dynamic> data;
        final approvalDeadline = _clock.elapsedMilliseconds + 30000;
        while (true) {
          try {
            data = await _transport.request(operation, {
              if (_session != null) 'session': _session,
              ...extra,
            });
            break;
          } on PreviewFailure catch (failure) {
            if (operation != '/pair' ||
                failure.code != 'approval_required' ||
                _clock.elapsedMilliseconds >= approvalDeadline) {
              rethrow;
            }
            await Future<void>.delayed(const Duration(seconds: 1));
            if (_disposed) return;
          }
        }
        if (_disposed) return;
        if (operation == '/status' && _wanted && data['connected'] == false) {
          if (data['paused'] == true) {
            _wanted = false;
            _heartbeat?.cancel();
          } else {
            data = await _transport.request('/resume', {'session': _session});
          }
        }
        _accept(data, forget: operation == '/forget');
        if (operation == '/pair' || operation == '/resume') _needsSave = true;
        if (_needsSave) {
          await _saveTrust();
          _needsSave = false;
        }
        if (operation == '/forget') {
          _needsSave = false;
          await nearby?.clearTrust();
        }
        _error = null;
      } on Object catch (failure) {
        if (_disposed) return;
        _error = failure is PreviewFailure ? failure.message : 'Cannot reach the Mac preview. Check Wi-Fi and that both previews are running.';
        _setStatus(ClusterConnection.unavailable);
        if (failure is PreviewFailure && failure.code == 'session_expired') {
          _session = null;
          _wanted = false;
          try {
            await nearby?.clearTrust();
          } on Object {
            _error = "Pairing was removed on the cluster, but local storage needs attention.";
          }
        } else if (_wanted &&
            _session != null &&
            nearby != null &&
            _clock.elapsedMilliseconds >= _nextDiscovery) {
          _nextDiscovery = _clock.elapsedMilliseconds + 15000;
          try {
            final devices = await discover(interactive: false);
            for (final device in devices.take(3)) {
              try {
                await _endpoint(device, interactive: false);
                break;
              } on Object {
                continue;
              }
            }
          } on Object {
            /* Retry later; keep an explicit unavailable state. */
          }
        }
      }
      if (_disposed) return;
      if (operation != '/status') _busy = false;
      notifyListeners();
      if (_wanted && _session != null) {
        _heartbeat?.cancel();
        _heartbeat = Timer.periodic(heartbeatInterval, (_) {
          if (_polling) return;
          _polling = true;
          _command('/status', {}).whenComplete(() => _polling = false);
        });
      }
    });
    return _queue;
  }

  void _accept(Map<String, dynamic> data, {required bool forget}) {
    final session = data['session'];
    if (data['v'] != 1 ||
        data['connected'] is! bool ||
        data['cluster'] != 'Mac preview' ||
        data['navigation'] != false ||
        (!forget &&
            (session is! String ||
                session.length < 16 ||
                session.length > 64)) ||
        (forget && (session != null || data['connected'] != false)) ||
        (_session != null && !forget && session != _session)) {
      throw const PreviewFailure(
        'Invalid preview response. Connection is unavailable.',
      );
    }
    _session = forget ? null : session as String;
    _setStatus(
      data['connected'] == true
          ? ClusterConnection.connected
          : ClusterConnection.disconnected,
      notify: false,
    );
  }

  void _setStatus(ClusterConnection connection, {bool notify = true}) {
    _status = PhoneStatus(
      connection: connection,
      clusterName: 'Mac preview',
      navigation: FeatureReadiness.notImplemented,
      notifications: FeatureReadiness.notImplemented,
      music: FeatureReadiness.notImplemented,
    );
    if (!_disposed && notify) notifyListeners();
  }

  @override
  void dispose() {
    _disposed = true;
    _heartbeat?.cancel();
    super.dispose();
  }
}
