#pragma once
namespace cluster::presentation {
// Approved UI demonstration thresholds, not verified Yamaha specifications.
struct Policy {
    static constexpr int fuelSegments=8, shiftRpm=10500, shiftFlashRpm=11400;
    static constexpr unsigned shiftPeriod=300, fuelPeriod=500, turnPeriod=450;
};
} // namespace cluster::presentation
