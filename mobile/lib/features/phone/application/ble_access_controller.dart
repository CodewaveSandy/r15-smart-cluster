import 'dart:io';

import 'package:flutter/foundation.dart';

import '../data/ble_access.dart';

/// Owns permission operations; presentation only renders state/emits intents.
class BleAccessController extends ChangeNotifier {
  BleAccessController({this.onReady, this.onBlocked});
  final VoidCallback? onReady, onBlocked;
  BleAccess? access;
  bool busy = false;
  bool ready = false;
  String? error;
  bool _disposed = false;

  Future<void> check({bool initial = false, bool request = false}) async {
    if (_disposed || busy) return;
    if (!Platform.isAndroid) {
      final wasReady = ready;
      ready = true;
      notifyListeners();
      if (!wasReady) onReady?.call();
      return;
    }
    busy = true;
    error = null;
    notifyListeners();
    try {
      var current = await BleAccess.check(request: request);
      if (_disposed) return;
      if (initial && current.supported && !current.ready && !current.blocked) {
        current = await BleAccess.check(request: true);
      }
      if (_disposed) return;
      final wasReady = ready;
      access = current;
      ready = current.ready;
      if (ready && !wasReady) onReady?.call();
      if (!ready) onBlocked?.call();
    } on Object {
      if (_disposed) return;
      ready = false;
      error = 'Bluetooth access could not be checked. Please try again.';
      onBlocked?.call();
    }
    if (!_disposed) {
      busy = false;
      notifyListeners();
    }
  }

  Future<void> request() async {
    if (_disposed || busy) return;
    final current = access;
    if (current?.blocked == true ||
        (current?.granted == true && current?.locationEnabled == false)) {
      try {
        await BleAccess.openSettings(
          location: current!.granted && !current.locationEnabled,
        );
      } on Object {
        if (!_disposed) {
          error =
              'Open the app permissions in Android Settings, then return here.';
          notifyListeners();
        }
      }
    } else {
      await check(request: true);
    }
  }

  @override
  void dispose() {
    _disposed = true;
    super.dispose();
  }
}
