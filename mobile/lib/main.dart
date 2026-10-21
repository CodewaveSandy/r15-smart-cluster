import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:flutter/widgets.dart';

import 'app/companion_app.dart';
import 'features/phone/application/preview_session.dart';
import 'features/phone/data/preview_transport.dart';
import 'features/phone/data/nearby_platform.dart';
import 'features/phone/domain/phone_status.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  const nearby = NativeNearby();
  const host = String.fromEnvironment('R15_PREVIEW_HOST');
  const port = int.fromEnvironment('R15_PREVIEW_PORT', defaultValue: 8765);
  const token = String.fromEnvironment('R15_PREVIEW_TOKEN');
  const certificate = String.fromEnvironment('R15_PREVIEW_CERT');
  const id = String.fromEnvironment('R15_PREVIEW_ID');
  Map<String, dynamic>? saved;
  if (kDebugMode) {
    try {
      final value = await nearby.loadTrust();
      if (value != null) saved = jsonDecode(value) as Map<String, dynamic>;
    } on Object {
      saved = null;
    }
  }
  PreviewSession? session;
  if (kDebugMode && (host.isNotEmpty || saved != null)) {
    final useSaved =
        saved != null && saved['v'] == 1 && (id.isEmpty || saved['id'] == id);
    try {
      session = PreviewSession(
        TlsPreviewTransport(
          host: useSaved ? saved['host'] as String : host,
          port: useSaved ? saved['port'] as int : port,
          token: useSaved ? saved['token'] as String : token,
          certificate: useSaved ? saved['certificate'] as String : certificate,
        ),
        nearby: nearby,
        peerId: useSaved ? saved['id'] as String : id,
      );
      if (useSaved) session.restore(saved['session'] as String);
    } on Object {
      session = null;
    }
  }
  runApp(
    CompanionApp(phoneStatus: const PhoneStatus.foundation(), session: session),
  );
}
