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
    ok &= check(bike.speed() == 180 && bike.rpm() == 12000 && bike.fuel() == 100, "Upper bounds failed");
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
    return ok ? 0 : 1;
}
