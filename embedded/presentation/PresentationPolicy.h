#pragma once
namespace cluster::presentation {
// Approved UI demonstration thresholds, not verified Yamaha specifications.
struct Policy {
    // User-supplied nominal capacities; sensor/volume calibration remains vehicle-owned.
    static constexpr int fuelCapacityMl=11000, fuelReserveMl=1760;
    static constexpr int fuelReservePercent=fuelReserveMl*100/fuelCapacityMl;
    static constexpr int fuelSegments=8, shiftRpm=10500, shiftFlashRpm=11400;
    static constexpr unsigned shiftPeriod=300, fuelPeriod=500, turnPeriod=450;
};
} // namespace cluster::presentation
