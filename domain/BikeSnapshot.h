#pragma once
#include <QMetaType>

// Normalized observations; units are independent of display preferences.
// Range guards are software constraints, not verified Yamaha limits.
struct BikeSnapshot {
    int speedKph = 0;
    int engineRpm = 0;
    int gear = 0;
    int fuelPercent = 0;
    double batteryVoltageV = 0;
    double coolantTemperatureC = 0;
    bool batteryVoltageAvailable = false;
    bool coolantTemperatureAvailable = false;
    bool hazards = false, leftIndicator = false, rightIndicator = false;
    bool highBeam = false, engineWarning = false, quickShifter = false;
    bool phoneConnected = false;
    int phoneBatteryPercent = -1;
    bool absWarning = false, tcsWarning = false, coolantWarning = false;
    bool messagePending = false, incomingCall = false;
};
Q_DECLARE_METATYPE(BikeSnapshot)
