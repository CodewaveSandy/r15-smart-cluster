import 'package:flutter/material.dart';

import '../application/ble_access_controller.dart';

/// Revalidates OS state; a remembered grant is never treated as current access.
class BleAccessGate extends StatefulWidget {
  const BleAccessGate({
    required this.child,
    this.onReady,
    this.onBlocked,
    super.key,
  });
  final Widget child;
  final VoidCallback? onReady, onBlocked;
  @override
  State<BleAccessGate> createState() => _BleAccessGateState();
}

class _BleAccessGateState extends State<BleAccessGate>
    with WidgetsBindingObserver {
  late final BleAccessController _controller;
  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
    _controller =
        BleAccessController(
            onReady: widget.onReady,
            onBlocked: widget.onBlocked,
          )
          ..addListener(_changed)
          ..check(initial: true);
  }

  void _changed() {
    if (mounted) setState(() {});
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    _controller.removeListener(_changed);
    _controller.dispose();
    super.dispose();
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.resumed) _controller.check();
  }

  @override
  Widget build(BuildContext context) {
    // Keep the existing route subtree alive when access changes.
    return Stack(
      children: [
        Offstage(
          offstage: !_controller.ready || _controller.busy,
          child: TickerMode(
            enabled: _controller.ready && !_controller.busy,
            child: widget.child,
          ),
        ),
        if (!_controller.ready || _controller.busy)
          Scaffold(
            body: SafeArea(
              child: Center(
                child: SingleChildScrollView(
                  padding: const EdgeInsets.all(28),
                  child: ConstrainedBox(
                    constraints: const BoxConstraints(maxWidth: 420),
                    child: Column(
                      mainAxisSize: MainAxisSize.min,
                      crossAxisAlignment: CrossAxisAlignment.stretch,
                      children: [
                        Icon(
                          Icons.bluetooth,
                          size: 56,
                          color: Theme.of(context).colorScheme.primary,
                        ),
                        const SizedBox(height: 24),
                        Text(
                          'Connect with your bike',
                          style: Theme.of(context).textTheme.headlineMedium,
                        ),
                        const SizedBox(height: 16),
                        Text(
                          _controller.error ??
                              (_controller.access?.supported == false
                                  ? 'This device does not support Bluetooth Low Energy.'
                                  : _controller.access?.legacyLocation == true
                                  ? 'Allow Location access and enable Location services so this Android version can scan BLE devices. Bluetooth must also be on.'
                                  : 'Allow Nearby devices access and enable Bluetooth to discover your cluster. We check this each time you open the app.'),
                        ),
                        const SizedBox(height: 24),
                        if (_controller.busy)
                          const Center(child: CircularProgressIndicator())
                        else ...[
                          FilledButton(
                            onPressed: _controller.access?.supported == false
                                ? null
                                : _controller.request,
                            child: Text(
                              _controller.access?.blocked == true
                                  ? 'Open app settings'
                                  : _controller.access?.granted == true &&
                                        _controller.access?.locationEnabled ==
                                            false
                                  ? 'Enable Location services'
                                  : 'Allow Bluetooth access',
                            ),
                          ),
                          TextButton(
                            onPressed: () => _controller.check(),
                            child: const Text('Check again'),
                          ),
                        ],
                      ],
                    ),
                  ),
                ),
              ),
            ),
          ),
      ],
    );
  }
}
