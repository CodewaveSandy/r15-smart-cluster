import 'dart:async';

import 'package:flutter/material.dart';

import '../features/phone/domain/phone_status.dart';
import '../features/phone/application/preview_session.dart';
import '../features/phone/presentation/phone_page.dart';
import 'widgets/brand_mark.dart';

class CompanionApp extends StatelessWidget {
  const CompanionApp({required this.phoneStatus, this.session, super.key});

  final PhoneStatus phoneStatus;
  final PreviewSession? session;

  @override
  Widget build(BuildContext context) => MaterialApp(
    title: 'R15 Companion',
    debugShowCheckedModeBanner: false,
    theme: _theme(Brightness.light),
    darkTheme: _theme(Brightness.dark),
    home: _Startup(status: phoneStatus, session: session),
  );

  ThemeData _theme(Brightness brightness) => ThemeData(
    brightness: brightness,
    fontFamily: 'Inter',
    colorSchemeSeed: const Color(0xff007757),
    scaffoldBackgroundColor: brightness == Brightness.dark
        ? const Color(0xff090e14)
        : const Color(0xffeff3f5),
    useMaterial3: true,
  );
}

class _Startup extends StatefulWidget {
  const _Startup({required this.status, this.session});
  final PreviewSession? session;
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
      ? widget.session == null
            ? PhonePage(status: widget.status)
            : AnimatedBuilder(
                animation: widget.session!,
                builder: (context, _) => PhonePage(
                  status: widget.session!.status,
                  session: widget.session,
                ),
              )
      : const Scaffold(
          body: SafeArea(child: Center(child: BrandMark())),
        );
}
