import 'package:flutter/services.dart';

import 'nearby_platform.dart';

/// Semantic native observations. GATT and Android keys stay on Android.
class BlePairing {
  static const _channel = MethodChannel('dev.r15cluster/nearby');
  static const _events = EventChannel('dev.r15cluster/ble-link');
  Stream<Map<String, dynamic>> get changes => _events
      .receiveBroadcastStream()
      .map((value) => Map<String, dynamic>.from(value as Map));
  Future<Map<String, dynamic>> status() async =>
      await _channel.invokeMapMethod<String, dynamic>('linkStatus') ?? {};
  Future<void> pair(NearbyDevice device, {bool saved = false}) async {
    await _channel.invokeMethod<dynamic>('bond', {
      'address': device.address,
      'name': device.name,
      'saved': saved,
      'diagnostics': const bool.fromEnvironment('R15_LINK_DIAGNOSTICS'),
    });
  }

  Future<dynamic> navigation(
    String method, [
    Map<String, dynamic> args = const {},
  ]) =>
      _channel.invokeMethod<dynamic>('navigation', {'method': method, ...args});

  Future<void> startMapSharing() =>
      _channel.invokeMethod<void>('startMapSharing');
  Future<void> stopMapSharing() =>
      _channel.invokeMethod<void>('stopMapSharing');
  Future<void> accessLost() => _channel.invokeMethod<void>('bleAccessLost');
  Future<void> locationSettings() =>
      _channel.invokeMethod<void>('locationSettings');
  Future<void> appSettings() => _channel.invokeMethod<void>('accessSettings');
  Future<void> disconnect() => _channel.invokeMethod<void>('disconnectLink');
  Future<void> forgetSelection() =>
      _channel.invokeMethod<void>('forgetSelection');
}
