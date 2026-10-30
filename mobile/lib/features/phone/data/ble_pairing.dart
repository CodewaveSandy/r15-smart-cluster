import 'package:flutter/services.dart';

import 'nearby_platform.dart';

/// Starts Android bonding; never translates an OS bond into an app session.
class BlePairing {
  static const _channel = MethodChannel('dev.r15cluster/nearby');
  Future<bool> pair(NearbyDevice device) async {
    final result = await _channel.invokeMapMethod<String, dynamic>('bond', {
      'address': device.address,
    });
    return result?['bonded'] == true;
  }
}
