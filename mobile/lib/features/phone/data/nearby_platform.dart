import 'dart:convert';

import 'package:flutter/services.dart';

final class NearbyDevice {
  const NearbyDevice({required this.address, required this.name});
  final String address;
  final String name;
}

abstract interface class NearbyPlatform {
  Future<List<NearbyDevice>> scan({bool interactive = true});
  Future<Map<String, dynamic>> read(
    NearbyDevice device, {
    bool interactive = true,
  });
  Future<String?> loadTrust();
  Future<void> saveTrust(String value);
  Future<void> clearTrust();
}

final class NativeNearby implements NearbyPlatform {
  const NativeNearby();
  static const _channel = MethodChannel('dev.r15cluster/nearby');
  @override
  Future<List<NearbyDevice>> scan({bool interactive = true}) async {
    final result =
        await _channel.invokeListMethod<dynamic>('scan', {
          'interactive': interactive,
        }) ??
        [];
    if (result.length > 8) {
      throw const FormatException('Too many discovery results');
    }
    return result
        .map((item) {
          final data = Map<String, dynamic>.from(item as Map);
          return NearbyDevice(
            address: data['address'] as String,
            name: data['name'] as String,
          );
        })
        .toList(growable: false);
  }

  @override
  Future<Map<String, dynamic>> read(
    NearbyDevice device, {
    bool interactive = true,
  }) async {
    final text = await _channel.invokeMethod<String>('read', {
      'address': device.address,
      'interactive': interactive,
    });
    if (text == null || utf8.encode(text).length > 512) {
      throw const FormatException('Invalid discovery response');
    }
    final data = jsonDecode(text) as Map<String, dynamic>;
    if (data['id'] is! String ||
        !RegExp(r'^[a-f0-9]{32}$').hasMatch(data['id'] as String) ||
        data['host'] is! String ||
        data['port'] is! int ||
        (data['port'] as int) < 1 ||
        (data['port'] as int) > 65535) {
      throw const FormatException('Invalid cluster identity');
    }
    return data;
  }

  @override
  Future<String?> loadTrust() => _channel.invokeMethod<String>('loadTrust');
  @override
  Future<void> saveTrust(String value) =>
      _channel.invokeMethod<void>('saveTrust', {'value': value});
  @override
  Future<void> clearTrust() => _channel.invokeMethod<void>('clearTrust');
}
