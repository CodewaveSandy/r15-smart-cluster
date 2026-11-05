import 'package:flutter/material.dart';

import '../features/home/presentation/my_bike_page.dart';
import '../features/home/domain/bike_telemetry.dart';
import '../features/phone/application/ble_connection.dart';
import '../features/phone/application/preview_session.dart';
import '../features/phone/domain/phone_status.dart';
import '../features/phone/presentation/phone_page.dart';
import '../features/phone/presentation/ble_access_gate.dart';

enum _CompanionPage { bike, connection }

/// Navigation state lives above pages; changing pages never owns/replaces BLE.
class CompanionHome extends StatefulWidget {
  const CompanionHome({
    required this.status,
    this.ble,
    this.session,
    super.key,
  });
  final PhoneStatus status;
  final BleConnection? ble;
  final PreviewSession? session;
  @override
  State<CompanionHome> createState() => _CompanionHomeState();
}

class _CompanionHomeState extends State<CompanionHome> {
  _CompanionPage _page = _CompanionPage.bike;
  _CompanionPage _lastTab = _CompanionPage.bike;
  void _select(_CompanionPage page) => setState(() {
    if (page == _CompanionPage.connection &&
        _page != _CompanionPage.connection) {
      _lastTab = _page;
    }
    _page = page;
  });
  void _back() => _select(
    _page == _CompanionPage.connection ? _lastTab : _CompanionPage.bike,
  );

  @override
  Widget build(BuildContext context) {
    final colors = Theme.of(context).colorScheme;
    final title = switch (_page) {
      _CompanionPage.bike => 'My bike',
      _CompanionPage.connection => 'Cluster connection',
    };
    return PopScope(
      canPop: _page == _CompanionPage.bike,
      onPopInvokedWithResult: (didPop, result) {
        if (!didPop) _back();
      },
      child: Scaffold(
        appBar: AppBar(
          automaticallyImplyLeading: false,
          leading: _page == _CompanionPage.connection
              ? BackButton(onPressed: _back)
              : null,
          title: Text(title, style: Theme.of(context).textTheme.titleLarge),
          backgroundColor: colors.surface,
          surfaceTintColor: Colors.transparent,
          actions: [
            if (_page != _CompanionPage.connection)
              IconButton(
                onPressed: () => _select(_CompanionPage.connection),
                tooltip: 'Cluster connection',
                icon: const Icon(Icons.settings_outlined),
              ),
            const SizedBox(width: 8),
          ],
        ),
        body: SafeArea(
          top: false,
          child: switch (_page) {
            _CompanionPage.bike => MyBikePage(
              status: widget.status,
              telemetry:
                  widget.ble?.telemetry ?? const BikeTelemetry.unavailable(),
              onConnection: () => _select(_CompanionPage.connection),
            ),
            _CompanionPage.connection => BleAccessGate(
              onReady: () {
                widget.ble?.accessReady();
                if (widget.session?.hasTrust == true) {
                  widget.session!.reconnect();
                }
              },
              onBlocked: () {
                widget.ble?.accessBlocked();
                widget.session?.pauseForAccess();
              },
              child: PhonePage(
                status: widget.status,
                ble: widget.ble,
                session: widget.session,
                embedded: true,
              ),
            ),
          },
        ),
      ),
    );
  }
}
