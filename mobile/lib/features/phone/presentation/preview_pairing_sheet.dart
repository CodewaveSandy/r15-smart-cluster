import 'package:flutter/material.dart';

import '../application/preview_session.dart';
import '../data/nearby_platform.dart';
import '../data/preview_transport.dart';

class PreviewPairingSheet extends StatefulWidget {
  const PreviewPairingSheet({required this.session, super.key});
  final PreviewSession session;
  @override
  State<PreviewPairingSheet> createState() => _PreviewPairingSheetState();
}

class _PreviewPairingSheetState extends State<PreviewPairingSheet> {
  List<NearbyDevice> _devices = [];
  bool _busy = true;
  String? _error, _code;
  @override
  void initState() {
    super.initState();
    _search();
  }

  Future<void> _search() async {
    setState(() {
      _busy = true;
      _error = null;
      _code = null;
      _devices = [];
    });
    try {
      final devices = await widget.session.discover();
      if (mounted) setState(() => _devices = devices);
    } on Object catch (failure) {
      if (mounted) {
        setState(
          () => _error = failure is PreviewFailure
              ? failure.message
              : 'Cannot search nearby. Check Bluetooth and permissions.',
        );
      }
    }
    if (mounted) setState(() => _busy = false);
  }

  Future<void> _select(NearbyDevice device) async {
    setState(() {
      _busy = true;
      _error = null;
    });
    try {
      final code = await widget.session.selectDevice(device);
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
    setState(() => _busy = true);
    final success = await widget.session.pair(_code!);
    if (!mounted) return;
    if (success) {
      Navigator.pop(context);
      return;
    }
    setState(() {
      _busy = false;
      _error = widget.session.error;
    });
  }

  @override
  Widget build(BuildContext context) => SafeArea(
    child: SingleChildScrollView(
      padding: const EdgeInsets.fromLTRB(24, 0, 24, 24),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          Text(
            _code == null ? 'Nearby clusters' : 'Pair with your cluster?',
            style: Theme.of(context).textTheme.headlineSmall,
          ),
          const SizedBox(height: 16),
          if (_busy) ...[
            const LinearProgressIndicator(),
            const SizedBox(height: 16),
            Text(_code == null ? 'Searching for your cluster…' : 'Pairing…'),
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
              ListTile(
                contentPadding: EdgeInsets.zero,
                leading: const Icon(Icons.two_wheeler),
                title: Text(device.name),
                subtitle: const Text('Nearby via Bluetooth'),
                trailing: TextButton(
                  onPressed: () => _select(device),
                  child: const Text('Connect'),
                ),
              ),
            const SizedBox(height: 16),
            TextButton(onPressed: _search, child: const Text('Search again')),
          ],
          if (_error != null)
            Padding(
              padding: const EdgeInsets.only(top: 12),
              child: Text(
                _error!,
                style: TextStyle(color: Theme.of(context).colorScheme.error),
              ),
            ),
        ],
      ),
    ),
  );
}
