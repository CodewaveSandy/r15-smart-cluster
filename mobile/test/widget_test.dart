import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:r15_companion/app/companion_app.dart';
import 'package:r15_companion/features/phone/domain/phone_status.dart';
import 'package:r15_companion/features/phone/presentation/phone_page.dart';

void main() {
  testWidgets('foundation never claims a working connection or feature', (
    tester,
  ) async {
    await tester.pumpWidget(
      const CompanionApp(phoneStatus: PhoneStatus.foundation()),
    );
    expect(find.text('No cluster connected'), findsOneWidget);
    expect(find.text('Not implemented'), findsNWidgets(3));
    expect(find.text('Ready'), findsNothing);
    expect(find.byType(ElevatedButton), findsNothing);
  });

  testWidgets('permission failure stays independent of other features', (
    tester,
  ) async {
    await tester.pumpWidget(
      const CompanionApp(
        phoneStatus: PhoneStatus(
          navigation: FeatureReadiness.ready,
          notifications: FeatureReadiness.permissionRequired,
          music: FeatureReadiness.unsupported,
        ),
      ),
    );
    expect(find.text('Ready'), findsOneWidget);
    expect(find.text('Permission needed'), findsOneWidget);
    expect(find.text('Not supported on this device'), findsOneWidget);
  });

  for (final brightness in Brightness.values) {
    testWidgets('small screen and large text: $brightness', (tester) async {
      tester.view.physicalSize = const Size(320, 568);
      tester.view.devicePixelRatio = 1;
      tester.platformDispatcher.platformBrightnessTestValue = brightness;
      addTearDown(tester.view.resetPhysicalSize);
      addTearDown(tester.view.resetDevicePixelRatio);
      addTearDown(tester.platformDispatcher.clearPlatformBrightnessTestValue);
      await tester.pumpWidget(
        MaterialApp(
          theme: ThemeData(brightness: brightness),
          builder: (context, child) => MediaQuery(
            data: MediaQuery.of(context)
                .copyWith(textScaler: TextScaler.linear(2)),
            child: child!,
          ),
          home: const PhonePage(status: PhoneStatus.foundation()),
        ),
      );
      await tester.scrollUntilVisible(
        find.text('No phone permissions are requested by this foundation.'),
        200,
      );
      await tester.pumpAndSettle();
      expect(tester.takeException(), isNull);
      expect(
        find.text('No phone permissions are requested by this foundation.'),
        findsOneWidget,
      );
    });
  }
}
