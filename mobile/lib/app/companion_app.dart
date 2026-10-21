import 'dart:async';

import 'package:flutter/material.dart';

import '../features/phone/domain/phone_status.dart';
import '../features/phone/presentation/phone_page.dart';
import 'widgets/brand_mark.dart';

class CompanionApp extends StatelessWidget {
  const CompanionApp({required this.phoneStatus, super.key});

  final PhoneStatus phoneStatus;

  @override
  Widget build(BuildContext context) => MaterialApp(
    title: 'R15 Companion',
    debugShowCheckedModeBanner: false,
    theme: _theme(Brightness.light),
    darkTheme: _theme(Brightness.dark),
    home: _Startup(status: phoneStatus),
  );

  ThemeData _theme(Brightness brightness) => ThemeData(
    brightness: brightness,
    colorSchemeSeed: const Color(0xff007757),
    scaffoldBackgroundColor: brightness == Brightness.dark
        ? const Color(0xff090e14)
        : const Color(0xffeff3f5),
    useMaterial3: true,
  );
}

class _Startup extends StatefulWidget {
  const _Startup({required this.status});
  final PhoneStatus status;

  @override
  State<_Startup> createState() => _StartupState();
}

class _StartupState extends State<_Startup> {
  Timer? _timer;
  bool _showHome = false;

  @override
  void initState() {
    super.initState();
    // Brief branding only; this does not claim to initialize phone services.
    _timer = Timer(const Duration(milliseconds: 900), () {
      if (mounted) setState(() => _showHome = true);
    });
  }

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) => _showHome
      ? PhonePage(status: widget.status)
      : const Scaffold(
          body: SafeArea(child: Center(child: BrandMark())),
        );
}
