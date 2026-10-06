#include "BikeState.h"
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    BikeState bike;
    int notifications = 0;
    QObject::connect(&bike, &BikeState::changed, [&] { ++notifications; });
    auto check = [](bool condition, const char *message) {
        if (!condition) qCritical() << message;
        return condition;
    };
    bool ok = true;
    bike.adjustSpeed(-5); bike.adjustRpm(-500); bike.adjustFuel(-1000);
    ok &= check(bike.speed() == 0 && bike.rpm() == 0 && bike.fuel() == 0, "Lower bounds failed");
    bike.adjustSpeed(1000); bike.adjustRpm(20000); bike.adjustFuel(1000);
    ok &= check(bike.speed() == 180 && bike.rpm() == 13000 && bike.fuel() == 100, "Upper bounds failed");
    bike.setGear(9);
    ok &= check(bike.gear() == 6, "Gear upper bound failed");
    bike.setGear(-1);
    ok &= check(bike.gear() == 0, "Neutral lower bound failed");
    bike.toggleLeft(); bike.toggleRight(); bike.toggleHighBeam(); bike.toggleEngineWarning();
    ok &= check(bike.leftIndicator() && bike.rightIndicator() && bike.highBeam() && bike.engineWarning(), "Indicator toggles failed");
    bike.reset();
    ok &= check(bike.speed() == 0 && bike.rpm() == 0 && bike.gear() == 0 && bike.fuel() == 72
                && !bike.leftIndicator() && !bike.rightIndicator() && !bike.highBeam() && !bike.engineWarning(), "Reset failed");
    ok &= check(notifications == 13, "State changes failed to notify UI");
    bike.adjustRpm(7500);
    ok &= check(!bike.vva(), "VVA must remain off at 7500 RPM");
    bike.adjustRpm(1);
    ok &= check(bike.vva(), "VVA must activate above 7500 RPM");
    bike.adjustRpm(-1);
    ok &= check(!bike.vva(), "VVA must deactivate at threshold");
    bike.toggleMessage(); bike.toggleCall();
    ok &= check(!bike.messagePending() && !bike.incomingCall() && bike.phoneBattery() == -1, "Disconnected phone must not expose a battery or notifications");
    bike.togglePhone();
    ok &= check(bike.messagePending() && bike.incomingCall() && bike.phoneBattery() == 84, "Connected phone simulation failed");
    bike.toggleQuickShifter(); bike.toggleAbs(); bike.toggleTcs(); bike.toggleCoolant();
    bike.reset();
    ok &= check(!bike.quickShifter() && !bike.absWarning() && !bike.tcsWarning() && !bike.coolantWarning() && !bike.phoneConnected(), "Extended signal reset failed");
    return ok ? 0 : 1;
}
