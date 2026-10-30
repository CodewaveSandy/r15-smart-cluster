import 'package:flutter/services.dart';

final class BleAccess {
  const BleAccess({
    required this.supported,
    required this.granted,
    required this.enabled,
    required this.blocked,
    required this.locationEnabled,
    required this.legacyLocation,
  });
  final bool supported,
      granted,
      enabled,
      blocked,
      locationEnabled,
      legacyLocation;
  bool get ready => supported && granted && enabled && locationEnabled;
  static const channel = MethodChannel('dev.r15cluster/nearby');
  static Future<BleAccess> check({bool request = false}) async {
    final data =
        await channel.invokeMapMethod<String, dynamic>(
          request ? 'requestAccess' : 'access',
        ) ??
        {};
    return BleAccess(
      supported: data['supported'] == true,
      granted: data['granted'] == true,
      enabled: data['enabled'] == true,
      blocked: data['blocked'] == true,
      locationEnabled: data['locationEnabled'] == true,
      legacyLocation: data['legacyLocation'] == true,
    );
  }

  static Future<void> openSettings({bool location = false}) => channel
      .invokeMethod<void>(location ? 'locationSettings' : 'accessSettings');
}
