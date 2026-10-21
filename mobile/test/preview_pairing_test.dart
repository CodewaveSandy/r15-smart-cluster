import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:r15_companion/app/companion_app.dart';
import 'package:r15_companion/features/phone/application/preview_session.dart';
import 'package:r15_companion/features/phone/data/preview_transport.dart';
import 'package:r15_companion/features/phone/data/nearby_platform.dart';
import 'package:r15_companion/features/phone/domain/phone_status.dart';

final class PairingTransport implements PreviewTransport {
  bool reject = false;
  @override
  Future<Map<String, dynamic>> request(
    String op,
    Map<String, Object?> data,
  ) async {
    if (op == '/offer') return {'v': 1, 'code': '123456', 'paired': false};
    if (reject) throw const PreviewFailure('That code did not match.');
    return {
      'v': 1,
      'session': op == '/forget' ? null : 'abcdefghijklmnopqrstuvwx',
      'connected': op != '/disconnect' && op != '/forget',
      'cluster': 'Mac preview',
      'navigation': false,
    };
  }
}

final class FakeNearby implements NearbyPlatform {
  @override
  Future<List<NearbyDevice>> scan({bool interactive = true}) async => [
    const NearbyDevice(address: 'test', name: 'R15 Cluster'),
  ];
  @override
  Future<Map<String, dynamic>> read(
    NearbyDevice device, {
    bool interactive = true,
  }) async => {
    'id': 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
    'host': '127.0.0.1',
    'port': 8765,
  };
  @override
  Future<String?> loadTrust() async => null;
  @override
  Future<void> saveTrust(String value) async {}
  @override
  Future<void> clearTrust() async {}
}

void main() {
  for (final brightness in Brightness.values) {
    testWidgets(
      'pair, disconnect, reconnect and confirmed forget: $brightness',
      (tester) async {
        tester.view.physicalSize = const Size(390, 844);
        tester.view.devicePixelRatio = 1;
        addTearDown(tester.view.resetPhysicalSize);
        addTearDown(tester.view.resetDevicePixelRatio);
        final session = PreviewSession(
          PairingTransport(),
          nearby: FakeNearby(),
          peerId: "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        );
        tester.platformDispatcher.platformBrightnessTestValue = brightness;
        addTearDown(tester.platformDispatcher.clearPlatformBrightnessTestValue);
        await tester.pumpWidget(
          CompanionApp(
            phoneStatus: const PhoneStatus.foundation(),
            session: session,
          ),
        );
        await tester.pump(const Duration(seconds: 1));
        await tester.tap(find.text('Connect cluster'));
        await tester.pumpAndSettle();
        expect(find.text('Nearby clusters'), findsOneWidget);
        await tester.tap(find.text('Connect'));
        await tester.pumpAndSettle();
        expect(find.text('Pair with your cluster?'), findsOneWidget);
        await tester.tap(find.text('Pair'));
        await tester.pumpAndSettle();
        expect(find.text('Mac preview · Connected'), findsOneWidget);
        expect(find.text('PAIRED CLUSTER'), findsOneWidget);
        await tester.ensureVisible(find.text('Disconnect'));
        await tester.pumpAndSettle();
        await tester.tap(find.text('Disconnect'));
        await tester.pumpAndSettle();
        expect(find.text('Connect cluster'), findsOneWidget);
        await tester.tap(find.text('Connect cluster'));
        await tester.pumpAndSettle();
        expect(find.text('Mac preview · Connected'), findsOneWidget);
        await tester.ensureVisible(find.text('Forget preview'));
        await tester.pumpAndSettle();
        await tester.tap(find.text('Forget preview'));
        await tester.pumpAndSettle();
        await tester.tap(find.text('Cancel'));
        await tester.pumpAndSettle();
        expect(session.hasTrust, isTrue);
        await tester.tap(find.text('Forget preview'));
        await tester.pumpAndSettle();
        await tester.tap(find.text('Forget'));
        await tester.pumpAndSettle();
        expect(session.hasTrust, isFalse);
        expect(find.text('Connect cluster'), findsOneWidget);
        session.dispose();
        expect(tester.takeException(), isNull);
      },
    );
  }
}
