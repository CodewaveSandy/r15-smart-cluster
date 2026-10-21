import 'dart:async';

import 'package:flutter_test/flutter_test.dart';
import 'package:r15_companion/features/phone/application/preview_session.dart';
import 'package:r15_companion/features/phone/data/preview_transport.dart';
import 'package:r15_companion/features/phone/domain/phone_status.dart';

final class FakeTransport implements PreviewTransport {
  final calls = <String>[];
  bool fail = false;
  bool malformed = false;
  bool restarted = false;
  bool paused = false;
  Completer<void>? gate;
  @override
  Future<Map<String, dynamic>> request(
    String op,
    Map<String, Object?> data,
  ) async {
    calls.add(op);
    if (op == '/resume') {
      restarted = false;
      paused = false;
    }
    if (gate != null) await gate!.future;
    if (fail) throw const PreviewFailure('Rejected');
    return {
      'v': 1,
      'session': op == '/forget' ? null : '123456789012345678901234',
      'connected':
          op != '/disconnect' &&
          op != '/forget' &&
          !(op == '/status' && (restarted || paused)),
      'paused': paused,
      'cluster': 'Mac preview',
      'navigation': malformed ? true : false,
    };
  }
}

void main() {
  test('pair only on confirmed response, disconnect preserves trust, forget clears', () async {
    final transport = FakeTransport();
    final session = PreviewSession(transport);
    addTearDown(session.dispose);
    expect(await session.pair('123'), isFalse);
    expect(transport.calls, isEmpty);
    expect(await session.pair('123456'), isTrue);
    expect(session.hasTrust, isTrue);
    expect(session.status.navigation, FeatureReadiness.notImplemented);
    await session.disconnect();
    expect(session.status.connection, ClusterConnection.disconnected);
    expect(session.hasTrust, isTrue);
    await session.reconnect();
    expect(session.status.connection, ClusterConnection.connected);
    await session.forget();
    expect(session.hasTrust, isFalse);
    expect(session.status.connection, ClusterConnection.disconnected);
  });

  test('rejection and malformed capabilities never assert success', () async {
    final transport = FakeTransport()..fail = true;
    final session = PreviewSession(transport);
    addTearDown(session.dispose);
    expect(await session.pair('123456'), isFalse);
    expect(session.hasTrust, isFalse);
    transport.fail = false;
    transport.malformed = true;
    expect(await session.pair('123456'), isFalse);
    expect(session.status.connection, ClusterConnection.unavailable);
    expect(session.hasTrust, isFalse);
  });

  testWidgets(
    'heartbeat failure masks connection and recovers without re-pair',
    (tester) async {
      final transport = FakeTransport();
      final session = PreviewSession(transport);
      await session.pair('123456');
      transport.fail = true;
      await tester.pump(const Duration(seconds: 2));
      await tester.pump();
      expect(session.status.connection, ClusterConnection.unavailable);
      expect(session.hasTrust, isTrue);
      transport.fail = false;
      await tester.pump(const Duration(seconds: 2));
      await tester.pump();
      expect(session.status.connection, ClusterConnection.connected);
      session.dispose();
    },
  );

  testWidgets(
    'host restart resumes saved session, explicit pause stops retries',
    (tester) async {
      final transport = FakeTransport();
      final session = PreviewSession(transport);
      await session.pair('123456');
      transport.restarted = true;
      await tester.pump(const Duration(seconds: 2));
      await tester.pump();
      expect(session.status.connection, ClusterConnection.connected);
      expect(transport.calls.last, '/resume');
      expect(transport.calls.where((op) => op == '/pair').length, 1);
      transport.paused = true;
      await tester.pump(const Duration(seconds: 2));
      await tester.pump();
      expect(session.status.connection, ClusterConnection.disconnected);
      final count = transport.calls.length;
      await tester.pump(const Duration(seconds: 5));
      expect(transport.calls.length, count);
      session.dispose();
    },
  );

  test('bounded duplicate requests and dispose ignore late result', () async {
    final transport = FakeTransport()..gate = Completer<void>();
    final session = PreviewSession(transport);
    final pending = session.pair('123456');
    await Future<void>.delayed(Duration.zero);
    await session.reconnect(); // Duplicate manual work is ignored while busy.
    expect(transport.calls, ['/pair']);
    session.dispose();
    transport.gate!.complete();
    await pending;
    expect(session.status.connection, ClusterConnection.connecting);
    expect(session.hasTrust, isFalse);
  });
}
