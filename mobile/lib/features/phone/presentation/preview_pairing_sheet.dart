import 'package:flutter/material.dart';

import 'dart:async';
import 'dart:math' as math;

import 'package:flutter/services.dart';

import '../application/preview_session.dart';
import '../data/nearby_platform.dart';
import '../data/preview_transport.dart';
import 'discovery_radar.dart';
import '../application/ble_connection.dart';

class PreviewPairingSheet extends StatefulWidget {
  const PreviewPairingSheet({this.session, this.ble, super.key});
  final PreviewSession? session;
  final BleConnection? ble;
  @override
  State<PreviewPairingSheet> createState() => _PreviewPairingSheetState();
}

class _PreviewPairingSheetState extends State<PreviewPairingSheet>
    with WidgetsBindingObserver {
  List<NearbyDevice> _devices = [];
  bool _busy = true;
  bool _scanning = true;
  bool _pairing = false;
  String? _error, _code;
  String? _bondedName;
  Timer? _rescan;
  int _generation = 0;
  bool _foreground = true;
  bool _cancelled = false;
  final Map<String, int> _lastSeen = {};
  int _round = 0;
  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
    _search();
  }

  Future<void> _search() async {
    _rescan?.cancel();
    final generation = ++_generation;
    setState(() {
      _busy = false;
      _scanning = true;
      _error = null;
      _code = null;
      _bondedName = null;
    });
    await _scanRound(generation);
  }

  Future<void> _scanRound(int generation) async {
    try {
      final devices =
          await (widget.session?.discover() ?? const NativeNearby().scan())
              .timeout(const Duration(seconds: 15));
      if (!mounted || generation != _generation) return;
      _round++;
      final merged = {for (final device in _devices) device.address: device};
      for (final device in devices.where(
        (device) => device.name.startsWith('R15 Cluster'),
      )) {
        merged[device.address] = device;
        _lastSeen[device.address] = _round;
      }
      merged.removeWhere(
        (address, _) => _round - (_lastSeen[address] ?? 0) >= 2,
      );
      _lastSeen.removeWhere((address, _) => !merged.containsKey(address));
      setState(() => _devices = merged.values.take(32).toList(growable: false));
    } on Object catch (failure) {
      if (mounted && generation == _generation) {
        setState(
          () => _error = failure is PlatformException
              ? failure.message ?? 'Nearby discovery is unavailable.'
              : failure is PreviewFailure
              ? failure.message
              : 'Cannot search nearby. Check Bluetooth and permissions.',
        );
        setState(() => _scanning = false);
      }
      return;
    }
    if (mounted && generation == _generation && _foreground) {
      // Five seconds scanning plus three seconds rest avoids rapid Android
      // scanner restarts. Radar stays animated across these bounded windows.
      _rescan = Timer(const Duration(seconds: 3), () => _scanRound(generation));
    }
  }

  Future<void> _stopSearch() async {
    ++_generation;
    _rescan?.cancel();
    if (mounted) setState(() => _scanning = false);
    try {
      await const NativeNearby().stopScan();
    } on Object {
      /* Native timeout also bounds cleanup. */
    }
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    _foreground = state == AppLifecycleState.resumed;
    if (!_foreground && !_pairing) {
      _stopSearch();
    }
    if (_foreground &&
        !_busy &&
        !_pairing &&
        _code == null &&
        _bondedName == null) {
      _search();
    }
  }

  @override
  void dispose() {
    ++_generation;
    _rescan?.cancel();
    WidgetsBinding.instance.removeObserver(this);
    const NativeNearby().stopScan().catchError((Object _) {});
    super.dispose();
  }

  Future<void> _select(NearbyDevice device) async {
    if (_busy) return;
    setState(() => _busy = true);
    await _stopSearch();
    if (!mounted) return;
    if (!device.previewService) {
      setState(() {
        _busy = true;
        _pairing = true;
        _error = null;
      });
      try {
        if (widget.ble == null) {
          throw StateError('Physical BLE controller unavailable');
        }
        await widget.ble!.select(device);
        final bonded = widget.ble!.live;
        if (!bonded) {
          throw PlatformException(
            code: 'bond',
            message: widget.ble!.error ?? 'Bluetooth link was not confirmed.',
          );
        }
        if (mounted) {
          setState(() {
            if (bonded) {
              _bondedName = device.name;
            } else {
              _error = 'Android did not confirm pairing. Try again.';
            }
          });
        }
      } on Object catch (failure) {
        if (mounted) {
          setState(
            () => _error = failure is PlatformException
                ? failure.message ?? 'Bluetooth pairing failed.'
                : 'Bluetooth pairing is unavailable. Check permissions and try again.',
          );
        }
      }
      if (mounted) {
        setState(() {
          _busy = false;
          _pairing = false;
        });
        if (!_cancelled && widget.ble?.live == true) Navigator.pop(context);
      }
      return;
    }
    if (widget.session == null) {
      setState(() {
        _busy = false;
        _error = 'The Mac preview launcher is required for this development service.';
      });
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
          maxHeight: MediaQuery.sizeOf(context).height * .80,
        ),
        child: SingleChildScrollView(
          padding: const EdgeInsets.all(20),
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
              if (_bondedName != null) ...[
                const Icon(Icons.bluetooth_connected, size: 64),
                const SizedBox(height: 16),
                Text(
                  'Bluetooth link connected with $_bondedName',
                  style: Theme.of(context).textTheme.titleLarge,
                ),
                const SizedBox(height: 12),
                const Text(
                  'Android has saved the Bluetooth bond. Companion data services are not connected yet.',
                ),
              ] else if (_busy && !_scanning) ...[
                const LinearProgressIndicator(),
                const SizedBox(height: 16),
                Text(
                  _code == null
                      ? 'Pairing… Compare the Android code with your cluster and approve both. Open Pair phone on the cluster first.'
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
                const Text(
                  'Searching for R15 Cluster devices… Tap an icon to pair.',
                  textAlign: TextAlign.center,
                ),
                const SizedBox(height: 16),
                _DeviceRadar(
                  devices: _devices,
                  scanning: _scanning,
                  onSelect: _select,
                ),
                if (_devices.isNotEmpty) ...[
                  const SizedBox(height: 8),
                  Text(
                    '${_devices.length} R15 Cluster devices · Positions are illustrative',
                    textAlign: TextAlign.center,
                  ),
                ],
                if (!_scanning && _devices.isEmpty && _error == null)
                  const Text(
                    'No R15 Cluster devices found. Power on your cluster and search again.',
                  ),
                const SizedBox(height: 16),
                const Text(
                  'Open Pair phone on your cluster before tapping its icon.',
                  textAlign: TextAlign.center,
                ),
                if (!_scanning)
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
                onPressed: _pairing
                    ? (_code == null && widget.ble != null
                          ? () async {
                              _cancelled = true;
                              await widget.ble!.disconnect();
                              if (context.mounted) Navigator.pop(context);
                            }
                          : null)
                    : () => Navigator.pop(context),
                child: Text(
                  _pairing && _code == null
                      ? 'Stop connection'
                      : _bondedName == null
                      ? 'Cancel'
                      : 'Done',
                ),
              ),
            ],
          ),
        ),
      ),
    ),
  );
}

class _DeviceRadar extends StatelessWidget {
  const _DeviceRadar({
    required this.devices,
    required this.scanning,
    required this.onSelect,
  });
  final List<NearbyDevice> devices;
  final bool scanning;
  final ValueChanged<NearbyDevice> onSelect;
  @override
  Widget build(BuildContext context) => LayoutBuilder(
    builder: (context, constraints) {
      final side = constraints.maxWidth.clamp(160.0, 360.0);
      var divisions = 2;
      var slots = <Offset>[];
      if (devices.length == 1) {
        slots = [const Offset(.48, .43)];
      } else {
        while (slots.length < devices.length) {
          slots = [];
          for (var row = 0; row < divisions; row++) {
            for (var column = 0; column < divisions; column++) {
              final position = Offset(
                (column + .5) / divisions,
                (row + .5) / divisions,
              );
              if ((position - const Offset(.5, .5)).distance +
                      math.sqrt2 / (2 * divisions) <
                  .49) {
                slots.add(position);
              }
            }
          }
          if (slots.length < devices.length) divisions++;
        }
        slots.shuffle(math.Random(15030));
        final jitter = math.Random(3150);
        slots = slots
            .map(
              (position) =>
                  position +
                  Offset(
                    (jitter.nextDouble() - .5) * .03 / divisions,
                    (jitter.nextDouble() - .5) * .03 / divisions,
                  ),
            )
            .toList(growable: false);
      }
      final extent = devices.length == 1 ? side * .44 : side / divisions * .92;
      return SizedBox(
        height: side,
        child: InteractiveViewer(
          minScale: 1,
          maxScale: 3,
          child: SizedBox.square(
            dimension: side,
            child: Stack(
              children: [
                DiscoveryRadar(scanning: scanning, diameter: side),
                for (var i = 0; i < devices.length; i++)
                  Positioned(
                    left: slots[i].dx * side - extent / 2,
                    top: slots[i].dy * side - extent / 2,
                    width: extent,
                    height: extent,
                    child: _RadarDevice(
                      device: devices[i],
                      extent: extent,
                      onTap: () => onSelect(devices[i]),
                    ),
                  ),
              ],
            ),
          ),
        ),
      );
    },
  );
}

class _RadarDevice extends StatelessWidget {
  const _RadarDevice({
    required this.device,
    required this.onTap,
    required this.extent,
  });
  final NearbyDevice device;
  final VoidCallback onTap;
  final double extent;
  @override
  Widget build(BuildContext context) {
    final name = device.name.replaceAll(RegExp(r'\s+'), ' ').trim();
    var split = name.length ~/ 2;
    final spaces = [
      for (var i = 1; i < name.length; i++)
        if (name[i] == ' ') i,
    ];
    if (spaces.isNotEmpty) {
      spaces.sort((a, b) => (a - split).abs().compareTo((b - split).abs()));
      split = spaces.first;
    }
    final lines = name.length < 2
        ? [name]
        : [name.substring(0, split).trim(), name.substring(split).trim()];
    return Semantics(
      button: true,
      onTap: onTap,
      label: 'Pair with ${device.name}',
      child: ExcludeSemantics(
        child: Tooltip(
          message: device.name,
          child: InkWell(
            onTap: onTap,
            borderRadius: BorderRadius.circular(20),
            child: SizedBox(
              width: extent,
              child: Padding(
                padding: const EdgeInsets.all(1),
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Container(
                      width: extent * .45,
                      height: extent * .45,
                      decoration: BoxDecoration(
                        shape: BoxShape.circle,
                        color: Theme.of(context)
                            .colorScheme
                            .surfaceContainerLow,
                        border: Border.all(
                          color: Theme.of(context).colorScheme.primary,
                        ),
                      ),
                      child: Icon(
                        device.clusterCandidate
                            ? Icons.speed_outlined
                            : Icons.bluetooth,
                        color: Theme.of(context).colorScheme.primary,
                        size: extent * .25,
                      ),
                    ),
                    const SizedBox(height: 2),
                    for (final line in lines)
                      SizedBox(
                        width: extent - 2,
                        height: extent * .15,
                        child: FittedBox(
                          fit: BoxFit.scaleDown,
                          child: Text(
                            line,
                            style: Theme.of(context).textTheme.titleMedium,
                          ),
                        ),
                      ),
                  ],
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }
}
