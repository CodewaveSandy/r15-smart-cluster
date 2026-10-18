#pragma once
#include <cstdint>
#include <cmath>

namespace cluster {
enum class Quality { Available, Unavailable, Unsupported, Invalid, Stale };
template<class T> struct Signal {
    T value{};
    Quality quality = Quality::Unavailable;
    bool available() const { return quality == Quality::Available; }
};
struct ClusterState {
    Signal<int> speedKph, engineRpm, gear, fuelPercent, phoneBatteryPercent;
    Signal<bool> quickShifter, vva, phoneConnected;
    Signal<double> odometerKm, batteryVoltageV, coolantTemperatureC;
    Signal<bool> left, right, highBeam, absWarning, engineWarning, coolantWarning;
};
// Software sanity limits, not Yamaha specifications. Invalid observations never become zero.
inline bool valid(const ClusterState& s) {
    const auto in = [](const Signal<int>& v, int lo, int hi) {
        return !v.available() || (v.value >= lo && v.value <= hi);
    };
    const auto real = [](const Signal<double>& v, double lo, double hi) {
        return !v.available() || (std::isfinite(v.value) && v.value >= lo && v.value <= hi);
    };
    return in(s.speedKph,0,400) && in(s.engineRpm,0,30000) && in(s.gear,0,6) &&
        in(s.fuelPercent,0,100) && in(s.phoneBatteryPercent,0,100) && real(s.odometerKm,0,1e9) &&
        real(s.batteryVoltageV,0,60) && real(s.coolantTemperatureC,-40,200);
}
inline bool urgent(const ClusterState& s) {
    return (s.engineWarning.available() && s.engineWarning.value) ||
        (s.coolantWarning.available() && s.coolantWarning.value);
}
} // namespace cluster
