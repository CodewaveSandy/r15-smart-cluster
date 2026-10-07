#include "domain/BikeState.h"
#include "data/simulator/SimulatorBikeDataSource.h"
#include <QCoreApplication>
#include <QDebug>
#include <limits>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    BikeState bike;
    SimulatorBikeDataSource simulator;
    QObject::connect(&simulator, &IBikeDataSource::snapshotReady, &bike, &BikeState::applySnapshot);
    QObject::connect(&simulator, &IBikeDataSource::disconnected, &bike, &BikeState::markDisconnected);
    simulator.start();
    int notifications = 0;
    QObject::connect(&bike, &BikeState::changed, [&] { ++notifications; });
    auto check = [](bool condition, const char *message) {
        if (!condition) qCritical() << message;
        return condition;
    };
    bool ok = true;
    simulator.adjustSpeed(-5); simulator.adjustRpm(-500); simulator.adjustFuel(-1000);
    ok &= check(bike.speedKph() == 0 && bike.engineRpm() == 0 && bike.fuelPercent() == 0, "Lower bounds failed");
    simulator.adjustSpeed(1000); simulator.adjustRpm(20000); simulator.adjustFuel(1000);
    ok &= check(bike.speedKph() == 180 && bike.engineRpm() == 13000 && bike.fuelPercent() == 100, "Upper bounds failed");
    simulator.setGear(9);
    ok &= check(bike.gear() == 6, "Gear upper bound failed");
    simulator.setGear(-1);
    ok &= check(bike.gear() == 0, "Neutral lower bound failed");
    simulator.toggleLeft(); simulator.toggleRight(); simulator.toggleHighBeam(); simulator.toggleEngineWarning();
    ok &= check(!bike.leftIndicator() && bike.rightIndicator() && bike.highBeam() && bike.engineWarning(), "Indicator toggles failed");
    simulator.reset();
    ok &= check(bike.speedKph() == 0 && bike.engineRpm() == 0 && bike.gear() == 0 && bike.fuelPercent() == 72
                && !bike.leftIndicator() && !bike.rightIndicator() && !bike.highBeam() && !bike.engineWarning(), "Reset failed");
    ok &= check(notifications == 11, "State changes failed to notify UI");
    simulator.adjustRpm(7500);
    ok &= check(!bike.vva(), "VVA must remain off at 7500 RPM");
    simulator.adjustRpm(1);
    ok &= check(bike.vva(), "VVA must activate above 7500 RPM");
    simulator.adjustRpm(-1);
    ok &= check(!bike.vva(), "VVA must deactivate at threshold");
    simulator.toggleMessage(); simulator.toggleCall();
    ok &= check(!bike.messagePending() && !bike.incomingCall() && bike.phoneBatteryPercent() == -1, "Disconnected phone must not expose a battery or notifications");
    simulator.togglePhone();
    ok &= check(bike.messagePending() && bike.incomingCall() && bike.phoneBatteryPercent() == 84, "Connected phone simulation failed");
    simulator.toggleQuickShifter(); simulator.toggleAbs(); simulator.toggleTcs(); simulator.toggleCoolant();
    simulator.reset();
    ok &= check(!bike.quickShifter() && !bike.absWarning() && !bike.tcsWarning() && !bike.coolantWarning() && !bike.phoneConnected(), "Extended signal reset failed");
    simulator.toggleLeft();
    ok &= check(bike.leftIndicator() && !bike.rightIndicator(), "Left signal must be exclusive");
    simulator.toggleRight();
    ok &= check(!bike.leftIndicator() && bike.rightIndicator(), "Right must cancel left");
    simulator.toggleHazards();
    ok &= check(bike.hazards() && bike.leftIndicator() && bike.rightIndicator(), "Hazards must enable both sides");
    simulator.toggleLeft();
    ok &= check(!bike.hazards() && bike.leftIndicator() && !bike.rightIndicator(), "Left must cancel hazards");
    simulator.toggleHazards(); simulator.toggleRight();
    ok &= check(!bike.hazards() && !bike.leftIndicator() && bike.rightIndicator(), "Right must cancel hazards");
    simulator.toggleRight();
    ok &= check(!bike.leftIndicator() && !bike.rightIndicator(), "Same-side toggle must turn off");
    simulator.toggleHazards(); simulator.toggleHazards();
    ok &= check(!bike.hazards() && !bike.leftIndicator() && !bike.rightIndicator(), "Hazard off must clear both");
    simulator.toggleHazards(); simulator.reset();
    ok &= check(!bike.hazards() && !bike.leftIndicator() && !bike.rightIndicator(), "Reset must clear hazards");
    simulator.adjustSpeed(std::numeric_limits<int>::max());
    simulator.adjustRpm(std::numeric_limits<int>::max());
    simulator.adjustFuel(std::numeric_limits<int>::max());
    ok &= check(bike.speedKph() == 180 && bike.engineRpm() == 13000 && bike.fuelPercent() == 100, "Extreme positive input overflowed simulator bounds");
    simulator.adjustSpeed(std::numeric_limits<int>::min());
    simulator.adjustRpm(std::numeric_limits<int>::min());
    simulator.adjustFuel(std::numeric_limits<int>::min());
    ok &= check(bike.speedKph() == 0 && bike.engineRpm() == 0 && bike.fuelPercent() == 0, "Extreme negative input overflowed simulator bounds");
    return ok ? 0 : 1;
}
