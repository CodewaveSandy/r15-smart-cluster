import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:r15_companion/app/companion_app.dart';
import 'package:r15_companion/features/phone/domain/phone_status.dart';
import 'package:r15_companion/features/phone/presentation/phone_page.dart';

void main() {
  const connected = PhoneStatus(
    connection: ClusterConnection.connected,
    clusterName: 'Rider cluster',
    navigation: FeatureReadiness.notImplemented,
    notifications: FeatureReadiness.notImplemented,
    music: FeatureReadiness.notImplemented,
  );

  testWidgets('startup leads to disconnected home without login', (
    tester,
  ) async {
    await tester.pumpWidget(
      const CompanionApp(phoneStatus: PhoneStatus.foundation()),
    );
    expect(find.text('R15'), findsOneWidget);
    expect(find.text('Connect cluster'), findsNothing);
    await tester.pump(const Duration(milliseconds: 900));
    expect(find.text('Connect cluster'), findsOneWidget);
    expect(find.text('Navigation'), findsNothing);
    await tester.tap(find.text('Connect cluster'));
    await tester.pumpAndSettle();
    expect(find.text('Connect your cluster'), findsOneWidget);
    expect(find.textContaining('· Connected'), findsNothing);
    await tester.tap(find.text('Got it'));
    await tester.pumpAndSettle();
    expect(find.text('Connect cluster'), findsOneWidget);
  });

  testWidgets(
    'confirmed connection reveals gated navigation; loss restores empty state',
    (tester) async {
      await tester.pumpWidget(
        const MaterialApp(home: PhonePage(status: connected)),
      );
      expect(find.text('Rider cluster · Connected'), findsOneWidget);
      expect(find.text('Navigation'), findsOneWidget);
      expect(
        tester.widget<FilledButton>(find.byType(FilledButton)).onPressed,
        isNull,
      );
      await tester.pumpWidget(
        const MaterialApp(home: PhonePage(status: PhoneStatus.foundation())),
      );
      expect(find.text('Connect cluster'), findsOneWidget);
      expect(find.text('Navigation'), findsNothing);
    },
  );

  testWidgets('disposing startup cancels delayed state update', (tester) async {
    await tester.pumpWidget(
      const CompanionApp(phoneStatus: PhoneStatus.foundation()),
    );
    await tester.pumpWidget(const SizedBox());
    await tester.pump(const Duration(seconds: 2));
    expect(tester.takeException(), isNull);
  });

  for (final connection in ClusterConnection.values) {
    for (final brightness in Brightness.values) {
      testWidgets('small screen large text: $connection $brightness', (
        tester,
      ) async {
        tester.view.physicalSize = const Size(320, 568);
        tester.view.devicePixelRatio = 1;
        addTearDown(tester.view.resetPhysicalSize);
        addTearDown(tester.view.resetDevicePixelRatio);
        await tester.pumpWidget(
          MaterialApp(
            theme: ThemeData(brightness: brightness),
            builder: (context, child) => MediaQuery(
              data: MediaQuery.of(context)
                  .copyWith(textScaler: TextScaler.linear(2)),
              child: child!,
            ),
            home: PhonePage(
              status: PhoneStatus(
                connection: connection,
                navigation: FeatureReadiness.permissionRequired,
                notifications: FeatureReadiness.unavailable,
                music: FeatureReadiness.unsupported,
              ),
            ),
          ),
        );
        await tester.pumpAndSettle();
        await tester.drag(
          find.byType(CustomScrollView),
          const Offset(0, -1000),
        );
        await tester.pumpAndSettle();
        expect(tester.takeException(), isNull);
        if (connection == ClusterConnection.connecting) {
          expect(
            tester.widget<FilledButton>(find.byType(FilledButton)).onPressed,
            isNull,
          );
        }
      });
    }
  }
}
