/// Bike readings stay independent from Bluetooth/session availability.
enum ReadingQuality { available, unavailable, unsupported, invalid, stale }

enum ReadingSource { cluster }

enum ReadingUnit {
  kilometresPerHour,
  gear,
  percent,
  volts,
  revolutionsPerMinute,
}

final class BikeReading<T extends num> {
  const BikeReading({
    required this.quality,
    required this.unit,
    this.value,
    this.source = ReadingSource.cluster,
  });
  final T? value;
  final ReadingQuality quality;
  final ReadingUnit unit;
  final ReadingSource source;
}

/// Immutable real cluster observations. Unsupported fields never have values.
final class BikeTelemetry {
  const BikeTelemetry({
    required this.speed,
    required this.gear,
    required this.fuel,
    required this.battery,
    this.rpm = const BikeReading<double>(
      quality: ReadingQuality.unsupported,
      unit: ReadingUnit.revolutionsPerMinute,
    ),
    this.left = const BikeLamp(),
    this.right = const BikeLamp(),
    this.highBeam = const BikeLamp(),
    this.diagnostics = const TelemetryDiagnostics(),
  });

  const BikeTelemetry.unavailable()
    : speed = const BikeReading<double>(
        quality: ReadingQuality.unavailable,
        unit: ReadingUnit.kilometresPerHour,
      ),
      gear = const BikeReading<int>(
        quality: ReadingQuality.unavailable,
        unit: ReadingUnit.gear,
      ),
      fuel = const BikeReading<double>(
        quality: ReadingQuality.unavailable,
        unit: ReadingUnit.percent,
      ),
      battery = const BikeReading<double>(
        quality: ReadingQuality.unavailable,
        unit: ReadingUnit.volts,
      ),
      rpm = const BikeReading<double>(
        quality: ReadingQuality.unavailable,
        unit: ReadingUnit.revolutionsPerMinute,
      ),
      left = const BikeLamp(),
      right = const BikeLamp(),
      highBeam = const BikeLamp(),
      diagnostics = const TelemetryDiagnostics();
  final BikeReading<double> speed;
  final BikeReading<int> gear;
  final BikeReading<double> fuel;
  final BikeReading<double> battery;
  final BikeReading<double> rpm;
  final BikeLamp left, right, highBeam;
  final TelemetryDiagnostics diagnostics;
}

final class BikeLamp {
  const BikeLamp({this.quality = ReadingQuality.unavailable, this.phase});
  final ReadingQuality quality;
  final bool? phase;
  ReadingSource get source => ReadingSource.cluster;
}

final class TelemetryDiagnostics {
  const TelemetryDiagnostics({
    this.state = 'notConnected',
    this.timingReady = false,
    this.sourceAgeBoundMs,
    this.receiptAgeMs,
    this.tripPresses,
    this.infoPresses,
    this.sourceSequence,
    this.displayFresh = false,
    this.metadataFresh = false,
    this.baselineRttMs,
    this.clockUncertaintyMs,
    this.rejected = 0,
    this.maxReceiptGapMs = 0,
    this.health = const {},
  });
  final String state;
  final bool timingReady, displayFresh, metadataFresh;
  final int? sourceAgeBoundMs,
      receiptAgeMs,
      tripPresses,
      infoPresses,
      sourceSequence;
  final int? baselineRttMs, clockUncertaintyMs;
  final int rejected, maxReceiptGapMs;
  final Map<String, int> health;
}

/// Typed semantic channel adapter; bytes/CRC/session ownership remain native.
BikeTelemetry telemetryFromNative(Object? value) {
  if (value is! Map) return const BikeTelemetry.unavailable();
  ReadingQuality quality(String key) {
    final code = value[key];
    return code is int && code >= 0 && code < ReadingQuality.values.length
        ? ReadingQuality.values[code]
        : value['state'] == 'unsupported'
        ? ReadingQuality.unsupported
        : ReadingQuality.unavailable;
  }

  final state = value['state'] as String? ?? 'unavailable';
  final gear = value['gear'];
  final gearQuality = quality('gearQuality');
  final health = value['health'];
  BikeLamp lamp(String key) => BikeLamp(
    quality: quality('${key}Quality'),
    phase:
        quality('${key}Quality') == ReadingQuality.available &&
            value[key] is bool
        ? value[key] as bool
        : null,
  );
  return BikeTelemetry(
    speed: const BikeReading<double>(
      quality: ReadingQuality.unsupported,
      unit: ReadingUnit.kilometresPerHour,
    ),
    gear: BikeReading<int>(
      quality: gearQuality,
      value:
          gearQuality == ReadingQuality.available &&
              gear is int &&
              gear >= 0 &&
              gear <= 6
          ? gear
          : null,
      unit: ReadingUnit.gear,
    ),
    fuel: const BikeReading<double>(
      quality: ReadingQuality.unsupported,
      unit: ReadingUnit.percent,
    ),
    battery: const BikeReading<double>(
      quality: ReadingQuality.unsupported,
      unit: ReadingUnit.volts,
    ),
    left: lamp('left'),
    right: lamp('right'),
    highBeam: lamp('highBeam'),
    diagnostics: TelemetryDiagnostics(
      state: state,
      timingReady: value['timingReady'] == true,
      sourceAgeBoundMs: value['sourceAgeBoundMs'] as int?,
      receiptAgeMs: value['receiptAgeMs'] as int?,
      tripPresses: value['tripPresses'] as int?,
      infoPresses: value['infoPresses'] as int?,
      sourceSequence: value['sourceSequence'] as int?,
      displayFresh: value['displayFresh'] == true,
      metadataFresh: value['metadataFresh'] == true,
      baselineRttMs: value['baselineRttMs'] as int?,
      clockUncertaintyMs: value['clockUncertaintyMs'] as int?,
      rejected: value['rejected'] as int? ?? 0,
      maxReceiptGapMs: value['maxReceiptGapMs'] as int? ?? 0,
      health: Map<String, int>.unmodifiable(
        health is Map ? Map<String, int>.from(health) : <String, int>{},
      ),
    ),
  );
}
