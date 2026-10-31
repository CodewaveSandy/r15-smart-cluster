/// Bike readings stay independent from Bluetooth/session availability.
enum ReadingQuality { available, unavailable, unsupported, invalid, stale }

enum ReadingSource { cluster }

enum ReadingUnit { kilometresPerHour, gear, percent, volts }

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

/// No vehicle-to-phone telemetry service is available in the current contract.
/// These empty readings must never be populated from concept-art samples.
final class BikeTelemetry {
  const BikeTelemetry({
    required this.speed,
    required this.gear,
    required this.fuel,
    required this.battery,
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
      );
  final BikeReading<double> speed;
  final BikeReading<int> gear;
  final BikeReading<double> fuel;
  final BikeReading<double> battery;
}
