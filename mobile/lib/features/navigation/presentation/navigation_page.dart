import 'dart:async';
import 'dart:convert';
import 'dart:math' as math;
import 'dart:ui' as ui;

import 'package:flutter/foundation.dart';
import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:maplibre_gl/maplibre_gl.dart';

import '../../phone/application/ble_connection.dart';
import '../domain/navigation_state.dart';

class NavigationPage extends StatefulWidget {
  const NavigationPage({required this.ble, super.key});
  final BleConnection? ble;
  @override
  State<NavigationPage> createState() => _NavigationPageState();
}

class _NavigationPageState extends State<NavigationPage> {
  final _search = TextEditingController();
  String? _error;
  bool _busy = false;
  final _mapKey = GlobalKey<_LiveMapState>();
  NavigationState get _state => widget.ble?.navigation ?? NavigationState(null);
  Future<void> _command(
    String method, [
    Map<String, dynamic> args = const {},
  ]) async {
    if (widget.ble == null) return;
    setState(() {
      _busy = true;
      _error = null;
    });
    try {
      await widget.ble!.navigationCommand(method, args);
    } on PlatformException catch (e) {
      if (mounted) {
        setState(() => _error = e.message ?? 'Navigation unavailable');
      }
    } finally {
      if (mounted) setState(() => _busy = false);
    }
  }

  Future<void> _enable() async {
    final yes = await showDialog<bool>(
      context: context,
      builder: (c) => AlertDialog(
        title: const Text('Maps around you'),
        content: const Text(
          'Allow location to show your position and download nearby roads from OpenFreeMap. Viewed areas are sent to the map provider. Destination searches and route endpoints go to Geoapify when you use those features. Maps stay privately cached on this phone. Navigation or sharing continues while locked until you stop it. Idle browsing stops location shortly after the app closes.',
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(c, false),
            child: const Text('Later'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(c, true),
            child: const Text('Enable maps & location'),
          ),
        ],
      ),
    );
    if (yes == true && mounted) await _command('enable');
  }

  Future<void> _settings() async {
    final input = TextEditingController();
    final saved = await showDialog<bool>(
      context: context,
      builder: (c) => AlertDialog(
        title: const Text('Search & motorcycle routing'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Create a free Geoapify project at myprojects.geoapify.com. Enter its API key here. It is encrypted privately on this device and excluded from backups. Your provider plan and quota apply; maps work without a key.',
            ),
            const SizedBox(height: 16),
            TextField(
              controller: input,
              obscureText: true,
              autocorrect: false,
              enableSuggestions: false,
              decoration: const InputDecoration(
                labelText: 'Geoapify API key',
                hintText: 'Blank removes the saved key',
              ),
            ),
            if (_state.flag('keyConfigured'))
              const Padding(
                padding: EdgeInsets.only(top: 8),
                child: Text('A key is currently saved.'),
              ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(c, false),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(c, true),
            child: const Text('Save'),
          ),
        ],
      ),
    );
    if (saved == true && mounted) {
      await _command('key', {'value': input.text.trim()});
    }
    input.dispose();
  }

  @override
  void dispose() {
    _search.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final s = _state,
        location = _state.location,
        route = _state.route,
        download = _state.download;
    final projection = widget.ble?.projection ?? const <String, dynamic>{};
    final native = !kIsWeb && defaultTargetPlatform == TargetPlatform.android;
    return Column(
      children: [
        Padding(
          padding: const EdgeInsets.fromLTRB(16, 8, 8, 8),
          child: Row(
            children: [
              Expanded(
                child: TextField(
                  controller: _search,
                  enabled:
                      native && s.flag('enabled') && s.flag('keyConfigured'),
                  textInputAction: TextInputAction.search,
                  decoration: InputDecoration(
                    prefixIcon: const Icon(Icons.search),
                    hintText: s.flag('keyConfigured')
                        ? 'Search a destination'
                        : 'Set up search in map settings',
                    border: const OutlineInputBorder(),
                    isDense: true,
                    suffixIcon: _search.text.isEmpty
                        ? null
                        : IconButton(
                            icon: const Icon(Icons.close),
                            onPressed: () {
                              _search.clear();
                              unawaited(_command('search', {'text': ''}));
                            },
                          ),
                  ),
                  onChanged: (text) {
                    setState(() {});
                    unawaited(_command('search', {'text': text}));
                  },
                ),
              ),
              IconButton(
                onPressed: native ? _settings : null,
                tooltip: 'Map settings and provider key',
                icon: const Icon(Icons.tune),
              ),
            ],
          ),
        ),
        if (s.suggestions.isNotEmpty)
          const Text('Search © Geoapify', style: TextStyle(fontSize: 11)),
        if (s.flag('searching') || s.flag('routing'))
          const LinearProgressIndicator(),
        if (s.suggestions.isNotEmpty)
          ConstrainedBox(
            constraints: const BoxConstraints(maxHeight: 190),
            child: ListView(
              shrinkWrap: true,
              children: [
                for (final item in s.suggestions)
                  ListTile(
                    dense: true,
                    leading: const Icon(Icons.place_outlined),
                    title: Text(item['label'] as String),
                    onTap: () {
                      FocusScope.of(context).unfocus();
                      unawaited(_command('select', item));
                    },
                  ),
              ],
            ),
          ),
        Expanded(
          child: Stack(
            children: [
              Positioned.fill(
                child: _LiveMap(
                  key: _mapKey,
                  state: s,
                  data: widget.ble?.navigationMap ?? const {},
                  onCamera: (lat, lon, zoom) {
                    if (s.flag('enabled')) {
                      unawaited(
                        _command('camera', {
                          'lat': lat,
                          'lon': lon,
                          'zoom': zoom,
                        }),
                      );
                    }
                  },
                ),
              ),
              Positioned(
                right: 12,
                top: 12,
                child: Column(
                  children: [
                    FloatingActionButton.small(
                      heroTag: 'follow',
                      onPressed: () {
                        unawaited(_command('resumeBrowse'));
                        _mapKey.currentState?.follow();
                      },
                      tooltip: 'Follow my location',
                      child: const Icon(Icons.my_location),
                    ),
                    const SizedBox(height: 8),
                    FloatingActionButton.small(
                      heroTag: 'overview',
                      onPressed: route.isEmpty
                          ? null
                          : () => _mapKey.currentState?.overview(),
                      tooltip: 'Route overview',
                      child: const Icon(Icons.route),
                    ),
                  ],
                ),
              ),
              if (!s.flag('enabled'))
                Center(
                  child: Card(
                    child: Padding(
                      padding: const EdgeInsets.all(16),
                      child: Column(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          const Text(
                            'Your current location and surrounding roads',
                          ),
                          const SizedBox(height: 12),
                          FilledButton(
                            onPressed: native && !_busy ? _enable : null,
                            child: const Text('Enable maps & location'),
                          ),
                        ],
                      ),
                    ),
                  ),
                ),
              if (s.flag('mapLoading'))
                const Positioned(
                  left: 16,
                  top: 16,
                  child: Chip(
                    avatar: SizedBox(
                      width: 16,
                      height: 16,
                      child: CircularProgressIndicator(strokeWidth: 2),
                    ),
                    label: Text('Loading nearby roads'),
                  ),
                ),
              const Positioned(
                left: 6,
                bottom: 4,
                child: ColoredBox(
                  color: Color(0xddffffff),
                  child: Padding(
                    padding: EdgeInsets.all(3),
                    child: Text(
                      '© OpenStreetMap · OpenMapTiles · OpenFreeMap',
                      style: TextStyle(color: Colors.black87, fontSize: 10),
                    ),
                  ),
                ),
              ),
            ],
          ),
        ),
        ConstrainedBox(
          constraints: BoxConstraints(
            maxHeight: MediaQuery.sizeOf(context).height * .34,
          ),
          child: SingleChildScrollView(
            padding: const EdgeInsets.fromLTRB(16, 10, 16, 10),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                Text(
                  _locationLabel(location),
                  style: Theme.of(context).textTheme.labelLarge,
                ),
                if (location['quality'] == 'permissionRequired' ||
                    location['quality'] == 'disabled' ||
                    location['quality'] == 'gpsDisabled' ||
                    location['quality'] == 'approximate')
                  Wrap(
                    spacing: 8,
                    children: [
                      TextButton(
                        onPressed: () => widget.ble?.appSettings(),
                        child: const Text('App permissions'),
                      ),
                      TextButton(
                        onPressed: () => widget.ble?.locationSettings(),
                        child: const Text('Location settings'),
                      ),
                    ],
                  ),
                for (final error in [
                  _error,
                  s.text('mapError'),
                  s.text('message'),
                  download['error'] as String?,
                ])
                  if (error != null)
                    Padding(
                      padding: const EdgeInsets.only(top: 6),
                      child: Text(
                        error,
                        style: TextStyle(
                          color: Theme.of(context).colorScheme.error,
                        ),
                      ),
                    ),
                if (route.isNotEmpty) ...[
                  const SizedBox(height: 8),
                  Text(
                    route['label'] as String? ?? 'Selected route',
                    maxLines: 2,
                    overflow: TextOverflow.ellipsis,
                    style: Theme.of(context).textTheme.titleSmall,
                  ),
                  Text(
                    '${_distance(route['remaining'] ?? route['distance'])} · estimated ${(((route['remainingSeconds'] ?? route['seconds']) as num? ?? 0) / 60).ceil()} min · motorcycle',
                  ),
                  if (s.flag('navigating') &&
                      (route['offRoute'] == true ||
                          route['instruction'] != null))
                    Text(
                      route['offRoute'] == true
                          ? 'Off route — awaiting a verified replacement'
                          : '${_distance(route['maneuverDistance'])} · ${route['instruction']}',
                    ),
                  if (download['busy'] == true ||
                      download['ready'] == true ||
                      download['error'] != null) ...[
                    const SizedBox(height: 6),
                    LinearProgressIndicator(
                      value: (download['total'] as num? ?? 0) > 0
                          ? ((download['done'] as num? ?? 0) /
                                    (download['total'] as num))
                                .clamp(0, 1)
                          : null,
                    ),
                    Text(
                      download['ready'] == true
                          ? 'Route corridor cached · offline detail through z${download['zoom'] ?? 14}'
                          : '${download['done'] ?? 0}/${download['total'] ?? 0} corridor tiles · ${((download['bytes'] as num? ?? 0) / 1048576).toStringAsFixed(1)} MiB',
                    ),
                    if (download['error'] != null)
                      Wrap(
                        spacing: 8,
                        children: [
                          TextButton(
                            onPressed: () => _command('download', {
                              'zoom': download['zoom'] ?? 14,
                            }),
                            child: const Text('Retry download'),
                          ),
                          TextButton(
                            onPressed: () => _command('download', {
                              'zoom':
                                  ((download['zoom'] as num? ?? 14).toInt() - 1)
                                      .clamp(8, 14),
                            }),
                            child: const Text('Use coarser offline detail'),
                          ),
                        ],
                      ),
                  ],
                  if (s.flag('sharing')) ...[
                    const SizedBox(height: 6),
                    Text(_displayLabel(projection)),
                    if ((projection['total'] as num? ?? 0) > 0)
                      LinearProgressIndicator(
                        value:
                            ((projection['offset'] as num? ?? 0) /
                                    (projection['total'] as num))
                                .clamp(0, 1),
                      ),
                    Text(
                      '${projection['objects'] ?? 0} display objects · ${projection['bytesPerSecond'] ?? 0} B/s · ${projection['offset'] ?? 0}/${projection['total'] ?? 0} bytes',
                    ),
                  ],
                  Wrap(
                    spacing: 8,
                    runSpacing: 4,
                    children: [
                      if (!s.flag('navigating'))
                        FilledButton.icon(
                          onPressed: _busy ? null : () => _command('start'),
                          icon: const Icon(Icons.navigation_outlined),
                          label: const Text('Start navigation'),
                        ),
                      if (!s.flag('sharing'))
                        FilledButton.tonalIcon(
                          onPressed: _busy || widget.ble?.live != true
                              ? null
                              : () => _command('share'),
                          icon: const Icon(Icons.screen_share_outlined),
                          label: const Text('Share route'),
                        ),
                      if (s.flag('sharing'))
                        OutlinedButton(
                          onPressed: () => _command('stopSharing'),
                          child: const Text('Stop sharing'),
                        ),
                      OutlinedButton(
                        onPressed: () => _command('end'),
                        child: Text(
                          s.flag('navigating')
                              ? 'End navigation'
                              : 'Clear route',
                        ),
                      ),
                    ],
                  ),
                  if (widget.ble?.live != true)
                    const Text(
                      'Phone navigation works independently. Connect the cluster to share your route.',
                    ),
                  const Text(
                    'Routing © Geoapify · estimates exclude live traffic. Offline search and rerouting require internet.',
                    style: TextStyle(fontSize: 11),
                  ),
                ],
                if (s.text('mapError') != null)
                  TextButton(
                    onPressed: () => _command('retry'),
                    child: const Text('Retry maps'),
                  ),
                if (s.flag('enabled') && !s.flag('navigating'))
                  Align(
                    alignment: Alignment.centerLeft,
                    child: TextButton(
                      onPressed: () => _command('clearCache'),
                      child: const Text('Clear cached maps'),
                    ),
                  ),
              ],
            ),
          ),
        ),
      ],
    );
  }

  String _locationLabel(Map<String, dynamic> p) {
    final quality = p['quality'] as String? ?? 'waiting';
    final accuracy = p['accuracy'] as num?;
    final suffix = accuracy == null ? '' : ' · ±${accuracy.round()} m';
    return switch (quality) {
      'precise' => 'Current location$suffix',
      'approximate' =>
        'Approximate location$suffix — guidance waits for precise GPS',
      'stale' => 'Location stale$suffix',
      'permissionRequired' => 'Location permission required',
      'stopped' => 'Location stopped · tap Follow to resume browsing',
      'disabled' || 'gpsDisabled' => 'Turn on phone Location services',
      'invalid' => 'Invalid location; no live pointer',
      'providerUnavailable' ||
      'providerUnsupported' => 'Location provider unavailable',
      _ => 'Waiting for a fresh location$suffix',
    };
  }

  String _distance(dynamic value) {
    final m = (value as num? ?? 0).toDouble();
    return m >= 1000 ? '${(m / 1000).toStringAsFixed(1)} km' : '${m.round()} m';
  }

  String _displayLabel(Map<String, dynamic> p) {
    if (p['error'] != null) return p['error'] as String;
    final coverage = switch (p['coverage']) {
      2 => 'roads ready',
      1 => 'partial roads',
      _ => 'waiting for roads',
    };
    final quality = switch (p['quality']) {
      1 => 'precise pointer',
      2 => 'approximate pointer',
      3 => 'stale location',
      _ => 'waiting for live location',
    };
    final stage = switch (p['stage']) {
      2 => 'receiving',
      3 => 'verifying',
      4 => 'object verified',
      6 => 'cache reused',
      5 => 'stopped',
      _ => 'preparing',
    };
    return 'Display: $stage · $coverage · $quality · ${p['camera'] == 1 ? 'free browse' : 'follow'}${p['geometryCeiling'] == true ? ' · zoom in for detail' : ''}';
  }
}

const _empty = <String, dynamic>{
  'type': 'FeatureCollection',
  'features': <Object>[],
};

class _LiveMap extends StatefulWidget {
  const _LiveMap({
    super.key,
    required this.state,
    required this.data,
    required this.onCamera,
  });
  final NavigationState state;
  final Map<String, dynamic> data;
  final void Function(double lat, double lon, double zoom) onCamera;
  @override
  State<_LiveMap> createState() => _LiveMapState();
}

class _LiveMapState extends State<_LiveMap> {
  MapLibreMapController? _controller;
  bool _loaded = false, _updating = false, _again = false, _follow = true;

  int _revision = -1;
  String? _error;
  Timer? _cameraTimer;
  LatLng? _lastFollow;
  LatLng? get _point {
    final p = widget.state.location;
    if (p['lat'] is! num || p['lon'] is! num) return null;
    return LatLng((p['lat'] as num).toDouble(), (p['lon'] as num).toDouble());
  }

  Future<void> _style() async {
    final c = _controller;
    if (c == null) return;
    final night = Theme.of(context).brightness == Brightness.dark;
    try {
      for (final id in ['roads', 'route', 'accuracy', 'position', 'heading']) {
        await c.addGeoJsonSource(id, _empty);
      }
      await c.addLineLayer(
        'roads',
        'water',
        const LineLayerProperties(lineColor: '#649bb6', lineWidth: 5),
        filter: ['==', 'kind', 2],
      );
      await c.addLineLayer(
        'roads',
        'edge',
        LineLayerProperties(
          lineColor: night ? '#1f3144' : '#bac6c7',
          lineWidth: 6,
          lineJoin: 'round',
          lineCap: 'round',
        ),
        filter: ['!=', 'kind', 2],
      );
      await c.addLineLayer(
        'roads',
        'road',
        LineLayerProperties(
          lineColor: night ? '#647484' : '#ffffff',
          lineWidth: 3,
          lineJoin: 'round',
          lineCap: 'round',
        ),
        filter: ['!=', 'kind', 2],
      );
      await c.addLineLayer(
        'route',
        'route-line',
        const LineLayerProperties(
          lineColor: '#3483ed',
          lineWidth: 5,
          lineJoin: 'round',
          lineCap: 'round',
        ),
      );
      await c.addFillLayer(
        'accuracy',
        'accuracy-fill',
        const FillLayerProperties(fillColor: '#3388dd', fillOpacity: .12),
      );
      await c.addLineLayer(
        'accuracy',
        'accuracy-edge',
        const LineLayerProperties(
          lineColor: '#3388dd',
          lineWidth: 1,
          lineDasharray: [3, 3],
        ),
      );
      await c.addCircleLayer(
        'position',
        'point',
        const CircleLayerProperties(
          circleRadius: 7,
          circleColor: ['get', 'color'],
          circleStrokeColor: '#287aca',
          circleStrokeWidth: 3,
        ),
      );
      await c.addFillLayer(
        'heading',
        'direction',
        const FillLayerProperties(
          fillColor: '#1674d1',
          fillOutlineColor: '#ffffff',
        ),
      );
      if (!mounted || c != _controller) return;
      _loaded = true;
      _revision = -1;
      await _update();
    } on Object {
      if (mounted) {
        setState(
          () => _error = 'Map renderer unavailable. Reopen Navigate to retry.',
        );
      }
    }
  }

  Future<void> _update() async {
    if (!_loaded) return;
    if (_updating) {
      _again = true;
      return;
    }
    final c = _controller;
    if (c == null) return;
    _updating = true;
    try {
      final revision = (widget.data['revision'] as num?)?.toInt() ?? -1;
      if (_revision != revision) {
        final roads = widget.data['roads'] is Map
            ? Map<String, dynamic>.from(widget.data['roads'] as Map)
            : _empty;
        await c.setGeoJsonSource('roads', roads);
        await c.setGeoJsonSource(
          'route',
          widget.data['route'] is Map
              ? Map<String, dynamic>.from(widget.data['route'] as Map)
              : _empty,
        );
        await _labels(c, roads);
        _revision = revision;
      }
      final p = _point;
      final location = widget.state.location;
      final precise = location['quality'] == 'precise';
      await c.setGeoJsonSource(
        'position',
        p == null
            ? _empty
            : _features(
                'Point',
                [p.longitude, p.latitude],
                {'color': precise ? '#1674d1' : '#ffffff'},
              ),
      );
      final accuracy = (location['accuracy'] as num?)?.toDouble();
      await c.setGeoJsonSource(
        'accuracy',
        p != null && accuracy != null
            ? _features('Polygon', [_circle(p, accuracy.clamp(1, 3000))])
            : _empty,
      );
      final bearing = location['bearing'] as num?;
      await c.setGeoJsonSource(
        'heading',
        p != null && bearing != null
            ? _features('Polygon', [
                _arrow(p, bearing.toDouble(), c.cameraPosition?.zoom ?? 15),
              ])
            : _empty,
      );
      if (_follow &&
          p != null &&
          (_lastFollow == null ||
              (_lastFollow!.latitude - p.latitude).abs() +
                      (_lastFollow!.longitude - p.longitude).abs() >
                  .00002)) {
        _lastFollow = p;

        await c.moveCamera(
          CameraUpdate.newLatLngZoom(
            p,
            math.max(14, c.cameraPosition?.zoom ?? 15),
          ),
        );
      }
    } on Object {
      if (mounted) {
        setState(
          () => _error =
              'Map update failed. Cached navigation remains available.',
        );
      }
    } finally {
      _updating = false;
      if (_again && mounted) {
        _again = false;
        unawaited(_update());
      }
    }
  }

  Map<String, dynamic> _features(
    String type,
    dynamic coordinates, [
    Map<String, dynamic> props = const {},
  ]) => {
    'type': 'FeatureCollection',
    'features': [
      {
        'type': 'Feature',
        'properties': props,
        'geometry': {'type': type, 'coordinates': coordinates},
      },
    ],
  };
  List<List<double>> _circle(LatLng p, double radius) => List.generate(49, (i) {
    final a = i * math.pi / 24;
    return [
      p.longitude +
          math.cos(a) *
              radius /
              (111195 * math.cos(p.latitude * math.pi / 180)),
      p.latitude + math.sin(a) * radius / 111195,
    ];
  });
  List<List<double>> _arrow(LatLng p, double bearing, double zoom) {
    final scale =
        156543.03392 * math.cos(p.latitude * math.pi / 180) / math.pow(2, zoom);
    final a = bearing * math.pi / 180;
    return [
      const [0.0, 19.0],
      const [-9.0, -11.0],
      const [0.0, -5.0],
      const [9.0, -11.0],
      const [0.0, 19.0],
    ].map((q) {
      final x = q[0] * math.cos(a) + q[1] * math.sin(a),
          y = q[1] * math.cos(a) - q[0] * math.sin(a);
      return [
        p.longitude +
            x * scale / (111195 * math.cos(p.latitude * math.pi / 180)),
        p.latitude + y * scale / 111195,
      ];
    }).toList();
  }

  Future<Uint8List> _labelImage(String text) async {
    final night = Theme.of(context).brightness == Brightness.dark;
    final recorder = ui.PictureRecorder();
    final canvas = Canvas(recorder);
    final painter = TextPainter(
      text: TextSpan(
        text: text,
        style: TextStyle(
          fontFamily: 'Inter',
          fontSize: 12,
          color: night ? Colors.white : const Color(0xff24394c),
        ),
      ),
      textDirection: TextDirection.ltr,
      maxLines: 1,
    )..layout(maxWidth: 220);
    final width = painter.width.ceil() + 12, height = painter.height.ceil() + 8;
    canvas.drawRRect(
      RRect.fromRectAndRadius(
        Rect.fromLTWH(0, 0, width.toDouble(), height.toDouble()),
        const Radius.circular(4),
      ),
      Paint()
        ..color = night ? const Color(0xdd182a3b) : const Color(0xddebf0eb),
    );
    painter.paint(canvas, const Offset(6, 4));
    painter.dispose();
    final picture = recorder.endRecording();
    final image = await picture.toImage(width, height);
    picture.dispose();
    final bytes = await image.toByteData(format: ui.ImageByteFormat.png);
    image.dispose();
    return bytes!.buffer.asUint8List();
  }

  Future<void> _labels(
    MapLibreMapController c,
    Map<String, dynamic> roads,
  ) async {
    await c.clearSymbols();
    await c.setSymbolIconAllowOverlap(false);
    final seen = <String>{};
    var index = 0;
    for (final raw in roads['features'] as List? ?? const []) {
      if (index >= 32 || !mounted) break;
      final feature = raw as Map, props = raw['properties'] as Map;
      final name = props['name'] as String? ?? '';
      if (name.isEmpty || !seen.add(name)) continue;
      final line = feature['geometry']['coordinates'] as List;
      if (line.isEmpty) continue;
      final point = line[line.length ~/ 2] as List;
      final id = 'road-label-${index++}';
      await c.addImage(id, await _labelImage(name));
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
    }
  }

  void follow() {
    _follow = true;
    _lastFollow = null;
    unawaited(_update());
  }

  Future<void> overview() async {
    final route = widget.data['route'];
    if (route is! Map) return;
    final f = route['features'] as List? ?? [];
    if (f.isEmpty) return;
    final points = f.first['geometry']['coordinates'] as List;
    if (points.length < 2) return;
    double south = 90, north = -90, west = 180, east = -180;
    for (final p in points) {
      final lat = (p[1] as num).toDouble(), lon = (p[0] as num).toDouble();
      south = math.min(south, lat);
      north = math.max(north, lat);
      west = math.min(west, lon);
      east = math.max(east, lon);
    }
    _follow = false;
    await _controller?.animateCamera(
      CameraUpdate.newLatLngBounds(
        LatLngBounds(
          southwest: LatLng(south, west),
          northeast: LatLng(north, east),
        ),
        left: 30,
        right: 30,
        top: 30,
        bottom: 30,
      ),
    );
  }

  @override
  void didUpdateWidget(_LiveMap oldWidget) {
    super.didUpdateWidget(oldWidget);
    unawaited(_update());
  }

  @override
  void dispose() {
    _cameraTimer?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final night = Theme.of(context).brightness == Brightness.dark;
    return Stack(
      children: [
        Positioned.fill(
          child: Listener(
            onPointerDown: (_) {
              _follow = false;
            },
            child: MapLibreMap(
              key: ValueKey(night),
              styleString: jsonEncode({
                'version': 8,
                'sources': <String, dynamic>{},
                'layers': [
                  {
                    'id': 'background',
                    'type': 'background',
                    'paint': {
                      'background-color': night ? '#101e2c' : '#e7eee6',
                    },
                  },
                ],
              }),
              gestureRecognizers: {
                Factory<OneSequenceGestureRecognizer>(
                  EagerGestureRecognizer.new,
                ),
              },
              initialCameraPosition: CameraPosition(
                target: _point ?? const LatLng(0, 0),
                zoom: _point == null ? 1 : 15,
              ),
              minMaxZoomPreference: const MinMaxZoomPreference(1, 20),
              trackCameraPosition: true,
              tiltGesturesEnabled: false,
              rotateGesturesEnabled: false,
              myLocationEnabled: false,
              attributionButtonPosition: null,
              onMapCreated: (c) {
                _controller = c;
                _loaded = false;
                _error = null;
              },
              onStyleLoadedCallback: _style,
              onCameraIdle: () {
                final camera = _controller?.cameraPosition;
                if (camera == null) return;
                _cameraTimer?.cancel();
                _cameraTimer = Timer(
                  const Duration(milliseconds: 250),
                  () => widget.onCamera(
                    camera.target.latitude,
                    camera.target.longitude,
                    camera.zoom,
                  ),
                );
              },
            ),
          ),
        ),
        if (_error != null)
          Positioned(
            left: 12,
            bottom: 28,
            right: 12,
            child: Card(
              child: Padding(
                padding: const EdgeInsets.all(8),
                child: Text(_error!),
              ),
            ),
          ),
      ],
    );
  }
}
