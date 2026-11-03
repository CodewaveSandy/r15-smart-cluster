import 'package:flutter/material.dart';

import '../features/home/presentation/my_bike_page.dart';
import '../features/home/domain/bike_telemetry.dart';
import '../features/phone/application/ble_connection.dart';
import '../features/phone/application/preview_session.dart';
import '../features/phone/domain/phone_status.dart';
import '../features/phone/presentation/phone_page.dart';
import 'theme/companion_theme.dart';
import '../features/navigation/presentation/navigation_page.dart';

enum _CompanionPage { bike, navigate, connection }

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
  void didUpdateWidget(CompanionHome oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (widget.ble?.hasSelection != true &&
        widget.session?.hasTrust != true &&
        widget.status.connection != ClusterConnection.connected) {
      _page = _CompanionPage.bike;
    }
  }

  @override
  Widget build(BuildContext context) {
    final hasCluster =
        widget.ble?.hasSelection == true ||
        widget.session?.hasTrust == true ||
        widget.status.connection == ClusterConnection.connected;
    // Preserve the approved first-pair landing. Losing a link retains Home.
    if (!hasCluster) {
      return PhonePage(
        status: widget.status,
        ble: widget.ble,
        session: widget.session,
      );
    }
    final colors = Theme.of(context).colorScheme;
    final title = switch (_page) {
      _CompanionPage.bike => 'My bike',
      _CompanionPage.navigate => 'Navigate',
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
        bottomNavigationBar: _page == _CompanionPage.connection
            ? null
            : _BottomNavigation(selected: _page, onSelect: _select),
        body: SafeArea(
          top: false,
          child: switch (_page) {
            _CompanionPage.bike => MyBikePage(
              status: widget.status,
              telemetry:
                  widget.ble?.telemetry ?? const BikeTelemetry.unavailable(),
              onNavigate: () => _select(_CompanionPage.navigate),
              onConnection: () => _select(_CompanionPage.connection),
            ),
            _CompanionPage.navigate => NavigationPage(ble: widget.ble),
            _CompanionPage.connection => PhonePage(
              status: widget.status,
              ble: widget.ble,
              session: widget.session,
              embedded: true,
            ),
          },
        ),
      ),
    );
  }
}

class _BottomNavigation extends StatelessWidget {
  const _BottomNavigation({required this.selected, required this.onSelect});
  final _CompanionPage selected;
  final ValueChanged<_CompanionPage> onSelect;

  @override
  Widget build(BuildContext context) {
    final colors = Theme.of(context).colorScheme;
    return SafeArea(
      top: false,
      minimum: const EdgeInsets.fromLTRB(16, 0, 16, 8),
      child: Material(
        color: colors.surfaceContainerLow,
        clipBehavior: Clip.antiAlias,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(CompanionTokens.cardRadius),
          side: BorderSide(color: colors.outlineVariant),
        ),
        child: Row(
          children: [
            _item(
              context,
              _CompanionPage.bike,
              'My bike',
              Icons.two_wheeler_outlined,
            ),
            _item(
              context,
              _CompanionPage.navigate,
              'Navigate',
              Icons.near_me_outlined,
            ),
          ],
        ),
      ),
    );
  }

  Widget _item(
    BuildContext context,
    _CompanionPage page,
    String label,
    IconData icon,
  ) {
    final active = selected == page;
    final colors = Theme.of(context).colorScheme;
    final color = active ? colors.primary : colors.onSurfaceVariant;
    return Expanded(
      child: Semantics(
        selected: active,
        button: true,
        label: label,
        child: ExcludeSemantics(
          child: InkWell(
            onTap: () => onSelect(page),
            child: Padding(
              padding: const EdgeInsets.fromLTRB(12, 10, 12, 4),
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Icon(icon, size: 27, color: color),
                  const SizedBox(height: 4),
                  Text(
                    label,
                    style: Theme.of(context).textTheme.labelMedium
                        ?.copyWith(color: color),
                  ),
                  const SizedBox(height: 5),
                  Container(
                    width: 54,
                    height: 3,
                    decoration: BoxDecoration(
                      color: active ? colors.primary : Colors.transparent,
                      borderRadius: BorderRadius.circular(3),
                    ),
                  ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }
}
