import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import '../application/preview_session.dart';
import '../data/nearby_platform.dart';
import '../data/preview_transport.dart';
import 'discovery_radar.dart';
import 'cluster_illustration.dart';

class PreviewPairingSheet extends StatefulWidget {
  const PreviewPairingSheet({this.session, super.key});
  final PreviewSession? session;
  @override
  State<PreviewPairingSheet> createState() => _PreviewPairingSheetState();
}

class _PreviewPairingSheetState extends State<PreviewPairingSheet> {
  List<NearbyDevice> _devices = [];
  bool _busy = true;
  bool _scanning = true;
  bool _pairing = false;
  String? _error, _code;
  @override
  void initState() {
    super.initState();
    _search();
  }

  Future<void> _search() async {
    setState(() {
      _busy = true;
      _scanning = true;
      _error = null;
      _code = null;
      _devices = [];
    });
    try {
      final devices =
          await (widget.session?.discover() ?? const NativeNearby().scan())
              .timeout(const Duration(seconds: 15));
      if (mounted) setState(() => _devices = devices);
    } on Object catch (failure) {
      if (mounted) {
        setState(
          () => _error = failure is PlatformException
              ? failure.message ?? 'Nearby discovery is unavailable.'
              : failure is PreviewFailure
              ? failure.message
              : 'Cannot search nearby. Check Bluetooth and permissions.',
        );
      }
    }
    if (mounted) {
      setState(() {
        _busy = false;
        _scanning = false;
      });
    }
  }

  Future<void> _select(NearbyDevice device) async {
    if (widget.session == null) {
      setState(
        () => _error = 'Device discovered. Companion application pairing is not available for this device yet. The current connection adapter requires the Mac preview launcher.',
      );
      return;
    }
    setState(() {
      _busy = true;
      _error = null;
    });
    try {
      final code = await widget.session!.selectDevice(device);
      if (mounted) setState(() => _code = code);
    } on Object catch (failure) {
      if (mounted) {
        setState(
          () => _error = failure is PreviewFailure ? failure.message : 'Cannot connect. Keep your phone and Mac on the same Wi-Fi network.',
        );
      }
    }
    if (mounted) setState(() => _busy = false);
  }

  Future<void> _pair() async {
    setState(() {
      _busy = true;
      _pairing = true;
    });
    final success = await widget.session!.pair(_code!);
    if (!mounted) return;
    if (success) {
      Navigator.pop(context);
      return;
    }
    setState(() {
      _busy = false;
      _pairing = false;
      _error = widget.session!.error;
    });
  }

  @override
  Widget build(BuildContext context) => PopScope(
    canPop: !_pairing,
    child: SafeArea(
      child: ConstrainedBox(
        constraints: BoxConstraints(
          maxHeight: MediaQuery.sizeOf(context).height * .86,
        ),
        child: SingleChildScrollView(
          padding: const EdgeInsets.fromLTRB(24, 0, 24, 24),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              Row(
                children: [
                  Expanded(
                    child: Text(
                      _code == null
                          ? 'Nearby clusters'
                          : 'Pair with your cluster?',
                      style: Theme.of(context).textTheme.headlineSmall,
                    ),
                  ),
                  IconButton(
                    onPressed: _pairing ? null : () => Navigator.pop(context),
                    tooltip: 'Close discovery',
                    icon: const Icon(Icons.close),
                  ),
                ],
              ),
              const SizedBox(height: 16),
              if (_scanning) ...[
                const Text('Searching for nearby clusters…'),
                const SizedBox(height: 24),
                const Center(child: DiscoveryRadar()),
                const SizedBox(height: 24),
                const Text(
                  'Make sure pairing mode is enabled.',
                  textAlign: TextAlign.center,
                ),
              ] else if (_busy) ...[
                const LinearProgressIndicator(),
                const SizedBox(height: 16),
                Text(
                  _code == null
                      ? 'Reading cluster identity…'
                      : 'Waiting for cluster approval…',
                ),
              ] else if (_code != null) ...[
                Text(
                  _code!,
                  textAlign: TextAlign.center,
                  style: Theme.of(context).textTheme.displaySmall,
                ),
                const SizedBox(height: 12),
                const Text(
                  'Check this code on your cluster. Select Allow pairing on the cluster, then tap Pair. Next time you will reconnect automatically.',
                ),
                const SizedBox(height: 24),
                FilledButton(onPressed: _pair, child: const Text('Pair')),
                TextButton(onPressed: _search, child: const Text('Back')),
              ] else ...[
                if (_devices.isEmpty && _error == null)
                  const Text(
                    'No clusters found. Switch on your cluster and keep Bluetooth enabled.',
                  ),
                for (final device in _devices)
                  Card(
                    child: Padding(
                      padding: const EdgeInsets.all(20),
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.stretch,
                        children: [
                          const ClusterIllustration(),
                          const SizedBox(height: 16),
                          Text(
                            device.name,
                            style: Theme.of(context).textTheme.titleLarge,
                          ),
                          const SizedBox(height: 6),
                          const Text('Nearby · Not paired'),
                          const SizedBox(height: 16),
                          FilledButton(
                            onPressed: () => _select(device),
                            child: const Text('Connect'),
                          ),
                        ],
                      ),
                    ),
                  ),
                const SizedBox(height: 16),
                TextButton(
                  onPressed: _search,
                  child: const Text('Search again'),
                ),
              ],
              if (_error != null)
                Padding(
                  padding: const EdgeInsets.only(top: 12),
                  child: Text(
                    _error!,
                    style: TextStyle(
                      color: Theme.of(context).colorScheme.error,
                    ),
                  ),
                ),
              const SizedBox(height: 24),
              OutlinedButton(
                onPressed: _pairing ? null : () => Navigator.pop(context),
                child: const Text('Cancel'),
              ),
            ],
          ),
        ),
      ),
    ),
  );
}
