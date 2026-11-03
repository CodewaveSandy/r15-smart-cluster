import 'dart:async';
import 'dart:convert';
import 'dart:ui' as ui;

import 'package:flutter/foundation.dart';
import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:maplibre_gl/maplibre_gl.dart';

import '../../phone/application/ble_connection.dart';
import '../domain/map_sharing.dart';

class NavigationPage extends StatefulWidget {
  const NavigationPage({required this.ble, super.key});
  final BleConnection? ble;
  @override
  State<NavigationPage> createState() => _NavigationPageState();
}

class _NavigationPageState extends State<NavigationPage> {
  String? _error;
  bool _busy = false;
  Future<void> _start() async {
    final allowed = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Share map & location?'),
        content: const Text(
          'Your phone sends the central Kolkata road map and fresh location to your paired cluster, including while locked. Allow Precise location for the cluster pointer. No location history is saved. Stop sharing here or from the notification. This region covers central Kolkata only; route search and directions are not included.',
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(context, true),
            child: const Text('Continue'),
          ),
        ],
      ),
    );
    if (allowed == true && mounted) {
      await _command(() => widget.ble!.startMapSharing());
    }
  }

  Future<void> _command(Future<void> Function() run) async {
    setState(() {
      _busy = true;
      _error = null;
    });
    try {
      await run();
    } on PlatformException catch (e) {
      if (mounted) {
        setState(() => _error = e.message ?? 'Map sharing unavailable');
      }
    } finally {
      if (mounted) setState(() => _busy = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    final map = widget.ble?.mapSharing ?? const MapSharing();
    final native = !kIsWeb && defaultTargetPlatform == TargetPlatform.android;
    return LayoutBuilder(
      builder: (context, constraints) => SingleChildScrollView(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Text(
              'Kolkata · West Bengal',
              style: Theme.of(context).textTheme.headlineSmall,
            ),
            const SizedBox(height: 8),
            const Text('Offline central-area roads · north up'),
            const SizedBox(height: 16),
            SizedBox(
              height: (constraints.maxHeight * .5).clamp(260.0, 480.0),
              child: ClipRRect(
                borderRadius: BorderRadius.circular(16),
                child: RegionalMap(observation: map),
              ),
            ),
            const SizedBox(height: 12),
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    Text(
                      map.message,
                      style: Theme.of(context).textTheme.titleMedium,
                    ),
                    const SizedBox(height: 10),
                    Text(map.locationLabel),
                    if (map.sharing) ...[
                      const SizedBox(height: 8),
                      Text(map.clusterLocationLabel),
                    ],
                    if (map.sharing && !map.clusterActive) ...[
                      const SizedBox(height: 12),
                      LinearProgressIndicator(
                        value: map.state == 'transferring'
                            ? map.progress
                            : null,
                      ),
                      const SizedBox(height: 8),
                      Text(
                        '${map.accepted} / ${map.total} bytes accepted by display${map.state == 'checking' ? ' · validating' : ''}',
                      ),
                    ],
                    if (_error != null || map.error != null) ...[
                      const SizedBox(height: 10),
                      Text(
                        _error ?? map.error!,
                        style: TextStyle(
                          color: Theme.of(context).colorScheme.error,
                        ),
                      ),
                    ],
                    const SizedBox(height: 16),
                    if (map.sharing)
                      OutlinedButton.icon(
                        onPressed: _busy
                            ? null
                            : () =>
                                  _command(() => widget.ble!.stopMapSharing()),
                        icon: const Icon(Icons.stop_circle_outlined),
                        label: const Text('Stop map sharing'),
                      ),
                    if (!map.sharing || map.state == 'failed')
                      FilledButton.icon(
                        onPressed: !_busy && native && widget.ble?.live == true
                            ? _start
                            : null,
                        icon: const Icon(Icons.near_me_outlined),
                        label: Text(
                          map.state == 'failed'
                              ? 'Retry map sharing'
                              : 'Share map & location',
                        ),
                      ),
                    if (!native)
                      const Text(
                        'Phone location sharing is currently available on Android.',
                      ),
                    if (native && widget.ble?.live != true)
                      const Text('Connect your cluster to start sharing.'),
                    if (map.quality == 'gpsDisabled')
                      TextButton(
                        onPressed: widget.ble?.locationSettings,
                        child: const Text('Open location settings'),
                      ),
                    if (map.quality == 'accuracyLow' ||
                        map.quality == 'permissionRequired')
                      TextButton(
                        onPressed: widget.ble?.appSettings,
                        child: const Text('Location permission settings'),
                      ),
                  ],
                ),
              ),
            ),
            TextButton(
              onPressed: () => showDialog<void>(
                context: context,
                builder: (context) => AlertDialog(
                  title: const Text('Map attribution'),
                  content: const SingleChildScrollView(
                    child: SelectableText(
                      '© OpenStreetMap contributors\nODbL 1.0\nhttps://www.openstreetmap.org/copyright\nhttps://opendatacommons.org/licenses/odbl/1-0/\n\nCentral Kolkata extract, 2 November 2026. MapLibre renders the phone map. This is a regional road snapshot, without live traffic or route guidance.',
                    ),
                  ),
                  actions: [
                    TextButton(
                      onPressed: () => Navigator.pop(context),
                      child: const Text('Close'),
                    ),
                  ],
                ),
              ),
              child: const Text('© OpenStreetMap contributors · ODbL'),
            ),
          ],
        ),
      ),
    );
  }
}

Map<String, dynamic> _decodeRoads(String value) =>
    jsonDecode(value) as Map<String, dynamic>;
Future<Map<String, dynamic>>? _roads;

/// Map rendering owns no GPS subscription or BLE work. The service supplies one
/// fresh observation; all road/font resources are local and licensed.
class RegionalMap extends StatefulWidget {
  const RegionalMap({required this.observation, super.key});
  final MapSharing observation;
  @override
  State<RegionalMap> createState() => _RegionalMapState();
}

class _RegionalMapState extends State<RegionalMap> {
  MapLibreMapController? _controller;
  Symbol? _arrow;
  bool _loaded = false, _updating = false, _follow = true;
  String? _failure;
  bool _night = false;
  Future<Uint8List> _image({
    String? text,
    Color color = Colors.white,
    bool arrow = false,
  }) async {
    final recorder = ui.PictureRecorder();
    final canvas = Canvas(recorder);
    final painter = TextPainter(
      text: TextSpan(
        text: text,
        style: TextStyle(fontFamily: 'Inter', fontSize: 14, color: color),
      ),
      textDirection: TextDirection.ltr,
    )..layout(maxWidth: 220);
    final width = arrow ? 40 : painter.width.ceil() + 12;
    final height = arrow ? 40 : painter.height.ceil() + 8;
    if (arrow) {
      final path = Path()
        ..moveTo(20, 2)
        ..lineTo(5, 35)
        ..lineTo(20, 28)
        ..lineTo(35, 35)
        ..close();
      canvas.drawPath(path, Paint()..color = Colors.white);
      canvas.drawPath(
        path,
        Paint()
          ..color = const Color(0xff1667bf)
          ..style = PaintingStyle.stroke
          ..strokeWidth = 3,
      );
    } else {
      canvas.drawRRect(
        RRect.fromRectAndRadius(
          Rect.fromLTWH(0, 0, width.toDouble(), height.toDouble()),
          const Radius.circular(3),
        ),
        Paint()
          ..color = (_night ? const Color(0xff101c29) : const Color(0xffe7ede7))
              .withValues(alpha: .88),
      );
      painter.paint(canvas, const Offset(6, 4));
    }
    final picture = recorder.endRecording();
    final image = await picture.toImage(width, height);
    picture.dispose();
    final bytes = await image.toByteData(format: ui.ImageByteFormat.png);
    image.dispose();
    return bytes!.buffer.asUint8List();
  }

  Future<void> _style() async {
    final c = _controller;
    if (c == null) return;
    try {
      _roads ??= rootBundle
          .loadString('assets/maps/kolkata.geojson')
          .then((raw) => compute(_decodeRoads, raw));
      final roads = await _roads!;
      if (!mounted || c != _controller) return;
      await c.addGeoJsonSource('roads', roads);
      await c.addLineLayer(
        'roads',
        'road-edge',
        LineLayerProperties(
          lineColor: _night ? '#172938' : '#bac7c6',
          lineWidth: 7,
          lineCap: 'round',
          lineJoin: 'round',
        ),
      );
      await c.addLineLayer(
        'roads',
        'road-fill',
        LineLayerProperties(
          lineColor: _night ? '#34495b' : '#ffffff',
          lineWidth: 4,
          lineCap: 'round',
          lineJoin: 'round',
        ),
      );
      await c.addGeoJsonSource('position', const {
        'type': 'FeatureCollection',
        'features': <Object>[],
      });
      await c.addCircleLayer(
        'position',
        'position-dot',
        const CircleLayerProperties(
          circleRadius: 8,
          circleColor: '#1667bf',
          circleStrokeColor: '#ffffff',
          circleStrokeWidth: 3,
        ),
      );
      await c.addImage('heading', await _image(arrow: true));
      _arrow = await c.addSymbol(
        const SymbolOptions(
          geometry: LatLng(22.56, 88.351),
          iconImage: 'heading',
          iconOpacity: 0,
          iconSize: 1,
        ),
      );
      final seen = <String>{};
      var count = 0;
      final labelFeatures = (roads['features'] as List)
          .where((raw) => (raw as Map)['properties']['kind'] == 1)
          .toList();
      double labelDistance(dynamic raw) {
        final coordinates = (raw['geometry']['coordinates'] as List);
        final point = coordinates[coordinates.length ~/ 2] as List;
        final x = (point[0] as num).toDouble() - 88.351;
        final y = (point[1] as num).toDouble() - 22.56;
        return x * x + y * y;
      }

      labelFeatures.sort(
        (a, b) => labelDistance(a).compareTo(labelDistance(b)),
      );
      for (final raw in labelFeatures) {
        final feature = raw as Map;
        final props = feature['properties'] as Map;
        final name = props['name'] as String;
        if (props['kind'] != 1 || name.isEmpty || !seen.add(name)) continue;
        final coords = (feature['geometry'] as Map)['coordinates'] as List;
        final point = coords[coords.length ~/ 2] as List;
        final id = 'road-name-$count';
        await c.addImage(
          id,
          await _image(
            text: name,
            color: _night ? const Color(0xffa8bccd) : const Color(0xff3e565e),
          ),
        );
        await c.addSymbol(
          SymbolOptions(
            geometry: LatLng(
              (point[1] as num).toDouble(),
              (point[0] as num).toDouble(),
            ),
            iconImage: id,
            iconSize: 1,
          ),
        );
        if (++count == 8) break;
      }
      if (!mounted || c != _controller) return;
      _loaded = true;
      await _position();
    } on Object {
      if (mounted && c == _controller) {
        setState(
          () => _failure = 'Offline map renderer unavailable. Sharing status remains available below.',
        );
      }
    }
  }

  Future<void> _position() async {
    if (!_loaded || _updating) return;
    final c = _controller;
    if (c == null) return;
    _updating = true;
    final value = widget.observation;
    try {
      final fresh = value.latitude != null && value.longitude != null;
      final point = LatLng(value.latitude ?? 22.56, value.longitude ?? 88.351);
      await c.setGeoJsonSource('position', {
        'type': 'FeatureCollection',
        'features': [
          if (fresh && value.bearing == null)
            {
              'type': 'Feature',
              'properties': <String, Object>{},
              'geometry': {
                'type': 'Point',
                'coordinates': [point.longitude, point.latitude],
              },
            },
        ],
      });
      if (_arrow != null) {
        await c.updateSymbol(
          _arrow!,
          SymbolOptions(
            geometry: point,
            iconOpacity: fresh && value.bearing != null ? 1 : 0,
            iconRotate: value.bearing ?? 0,
          ),
        );
      }
      if (fresh && _follow && value.insideRegion) {
        await c.moveCamera(CameraUpdate.newLatLng(point));
      }
    } on Object {
      if (mounted && c == _controller) {
        setState(() => _failure = 'Map position display unavailable');
      }
    } finally {
      _updating = false;
      if (mounted &&
          (widget.observation.latitude != value.latitude ||
              widget.observation.longitude != value.longitude ||
              widget.observation.bearing != value.bearing)) {
        unawaited(_position());
      }
    }
  }

  @override
  void didUpdateWidget(RegionalMap oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.observation.latitude != widget.observation.latitude ||
        oldWidget.observation.longitude != widget.observation.longitude ||
        oldWidget.observation.bearing != widget.observation.bearing) {
      unawaited(_position());
    }
  }

  @override
  Widget build(BuildContext context) {
    final night = Theme.of(context).brightness == Brightness.dark;
    if (_night != night) {
      _night = night;
      _loaded = false;
      _arrow = null;
    }
    final style = jsonEncode({
      'version': 8,
      'sources': <String, Object>{},
      'layers': [
        {
          'id': 'background',
          'type': 'background',
          'paint': {'background-color': night ? '#101c29' : '#e7ede7'},
        },
      ],
    });
    return Stack(
      children: [
        Positioned.fill(
          child: MapLibreMap(
            key: ValueKey(night),
            styleString: style,
            gestureRecognizers: {
              Factory<OneSequenceGestureRecognizer>(EagerGestureRecognizer.new),
            },
            initialCameraPosition: const CameraPosition(
              target: LatLng(22.56, 88.351),
              zoom: 14,
            ),
            minMaxZoomPreference: const MinMaxZoomPreference(12, 18),
            rotateGesturesEnabled: false,
            tiltGesturesEnabled: false,
            myLocationEnabled: false,
            compassEnabled: true,
            attributionButtonPosition: null,
            onMapCreated: (value) {
              if (mounted) {
                setState(() {
                  _controller = value;
                  _loaded = false;
                  _failure = null;
                });
              }
            },
            onStyleLoadedCallback: _style,
          ),
        ),
        if (_failure != null)
          Positioned.fill(
            child: ColoredBox(
              color: Theme.of(context).colorScheme.surface,
              child: Center(
                child: Padding(
                  padding: const EdgeInsets.all(20),
                  child: Text(_failure!, textAlign: TextAlign.center),
                ),
              ),
            ),
          ),
        Positioned(
          top: 8,
          left: 8,
          right: 8,
          child: Card(
            child: Padding(
              padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
              child: Text(
                widget.observation.sharing
                    ? 'Phone map · not cluster receipt'
                    : 'Kolkata preview · no live location',
              ),
            ),
          ),
        ),
        Positioned(
          right: 8,
          bottom: 8,
          child: Row(
            mainAxisSize: MainAxisSize.min,
            children: [
              IconButton.filledTonal(
                onPressed: () {
                  _follow = true;
                  unawaited(_position());
                },
                tooltip: 'Follow current location',
                icon: const Icon(Icons.my_location),
              ),
              const SizedBox(width: 8),
              IconButton.filledTonal(
                onPressed: () {
                  _follow = false;
                  _controller?.moveCamera(
                    CameraUpdate.newLatLngBounds(
                      LatLngBounds(
                        southwest: const LatLng(22.546, 88.337),
                        northeast: const LatLng(22.574, 88.365),
                      ),
                      left: 24,
                      right: 24,
                      top: 48,
                      bottom: 56,
                    ),
                  );
                },
                tooltip: 'Kolkata area overview',
                icon: const Icon(Icons.map_outlined),
              ),
            ],
          ),
        ),
      ],
    );
  }
}
