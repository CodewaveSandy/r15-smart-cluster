import 'package:flutter/material.dart';

import '../features/phone/domain/phone_status.dart';
import '../features/phone/presentation/phone_page.dart';

/// Explicit composition keeps platform adapters out of widgets and domain code.
class CompanionApp extends StatelessWidget {
  const CompanionApp({required this.phoneStatus, super.key});

  final PhoneStatus phoneStatus;

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'R15 Companion',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(colorSchemeSeed: const Color(0xff007757)),
      darkTheme: ThemeData(
        brightness: Brightness.dark,
        colorSchemeSeed: const Color(0xff78e4c1),
      ),
      home: PhonePage(status: phoneStatus),
    );
  }
}
