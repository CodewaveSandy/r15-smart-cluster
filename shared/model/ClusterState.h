#pragma once
#include <cstdint>
#include <cmath>
#include "Quality.h"
#include "BluetoothState.h"
#include "WifiState.h"
#include "PhoneState.h"

namespace cluster {

template<class T> struct Signal {
    T value{};
    Quality quality = Quality::Unavailable;
    bool available() const { return quality == Quality::Available; }
};
struct ClusterState {
    bool turnSignalsArePhase=false; // True: observed lamp phase; do not synthesize blinking.
    PhoneState phone;
    BluetoothState bluetooth;
    WifiState wifi;
    Signal<std::uint32_t> currentLapMs, bestLapMs;
    Signal<int> speedKph, engineRpm, gear, fuelPercent, phoneBatteryPercent;
    Signal<bool> quickShifter, vva, phoneConnected, tcsEnabled;
    Signal<double> odometerKm, trip1Km, trip2Km, batteryVoltageV, coolantTemperatureC;
    Signal<double> realtimeFuelLPer100Km, averageFuelLPer100Km, averageSpeedKph;
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
    return valid(s.phone) && valid(s.wifi) && valid(s.bluetooth) && in(s.speedKph,0,400) && in(s.engineRpm,0,30000) && in(s.gear,0,6) &&
        in(s.fuelPercent,0,100) && in(s.phoneBatteryPercent,0,100) && real(s.odometerKm,0,1e9) &&
        real(s.trip1Km,0,1e9) && real(s.trip2Km,0,1e9) && real(s.batteryVoltageV,0,60) && real(s.coolantTemperatureC,-40,200) &&
        real(s.realtimeFuelLPer100Km,0,1000) && real(s.averageFuelLPer100Km,0,1000) && real(s.averageSpeedKph,0,400);
}
inline bool urgent(const ClusterState& s) {
    return (s.engineWarning.available() && s.engineWarning.value) ||
        (s.coolantWarning.available() && s.coolantWarning.value);
}
} // namespace cluster
