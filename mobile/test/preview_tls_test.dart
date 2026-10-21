import 'dart:convert';
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:r15_companion/features/phone/data/preview_transport.dart';

void main() {
  final configPath = Platform.environment['R15_TLS_TEST_CONFIG'];
  test(
    'actual Dart TLS client pairs with live Mac preview bridge',
    () async {
      final config = jsonDecode(
        File(configPath!).readAsStringSync(),
      ) as Map<String, dynamic>;
      final transport = TlsPreviewTransport(
        host: config['R15_PREVIEW_HOST'] as String,
        port: int.parse(config['R15_PREVIEW_PORT'] as String),
        token: config['R15_PREVIEW_TOKEN'] as String,
        certificate: config['R15_PREVIEW_CERT'] as String,
      );
      final paired = await transport.request('/pair', {
        'code': Platform.environment['R15_TLS_TEST_CODE'],
      });
      expect(paired['connected'], isTrue);
      expect(paired['navigation'], isFalse);
      final session = paired['session'] as String;
      expect(
        (await transport.request('/status', {'session': session}))['connected'],
        isTrue,
      );
      expect(
        (await transport.request('/disconnect', {
          'session': session,
        }))['connected'],
        isFalse,
      );
      expect(
        (await transport.request('/resume', {'session': session}))['connected'],
        isTrue,
      );
      expect(
        (await transport.request('/forget', {'session': session}))['session'],
        isNull,
      );
      await expectLater(
        transport.request('/status', {'session': session}),
        throwsA(isA<PreviewFailure>()),
      );
    },
    skip: configPath == null
        ? 'Run tests/host/phone-preview-smoke.py for live TLS integration.'
        : false,
  );
}
