import 'dart:async';

import 'package:flutter/material.dart';

import '../features/phone/domain/phone_status.dart';
import '../features/phone/application/ble_connection.dart';
import '../features/phone/data/ble_pairing.dart';
import '../features/phone/application/preview_session.dart';
import '../features/phone/presentation/phone_page.dart';
import '../features/phone/presentation/ble_access_gate.dart';
import 'theme/companion_theme.dart';
import 'widgets/opening_screen.dart';

class CompanionApp extends StatelessWidget {
  const CompanionApp({required this.phoneStatus, this.session, super.key});

  final PhoneStatus phoneStatus;
  final PreviewSession? session;

  @override
  Widget build(BuildContext context) => MaterialApp(
    title: 'R15 Companion',
    debugShowCheckedModeBanner: false,
    theme: CompanionTheme.forBrightness(Brightness.light),
    darkTheme: CompanionTheme.forBrightness(Brightness.dark),
    home: _Startup(status: phoneStatus, session: session),
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
  BleConnection? _ble;
  bool _showHome = false;

  @override
  void initState() {
    super.initState();
    if (widget.session == null) _ble = BleConnection(BlePairing());
    // Brief branding only; this does not claim to initialize phone services.
    _timer = Timer(CompanionTokens.openingDuration, () {
      if (mounted) setState(() => _showHome = true);
    });
  }

  @override
  void dispose() {
    _timer?.cancel();
    _ble?.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) => _showHome
      ? BleAccessGate(
          onReady: () {
            _ble?.accessReady();
            if (widget.session?.hasTrust == true) widget.session!.reconnect();
          },
          onBlocked: () {
            _ble?.accessBlocked();
            widget.session?.pauseForAccess();
          },
          child: widget.session == null
              ? AnimatedBuilder(
                  animation: _ble!,
                  builder: (context, _) =>
                      PhonePage(status: _ble!.status, ble: _ble),
                )
              : AnimatedBuilder(
                  animation: widget.session!,
                  builder: (context, _) => PhonePage(
                    status: widget.session!.status,
                    session: widget.session,
                  ),
                ),
        )
      : const OpeningScreen();
}
