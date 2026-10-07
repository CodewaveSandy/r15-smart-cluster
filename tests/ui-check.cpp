#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QDir>
#include <QtTest/QTest>
#include "BikeState.h"
#include "DisplaySettings.h"
#include "RideComputer.h"
#include "Fonts.h"

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    if (!loadClusterFonts()) return 1;
    app.setOrganizationName("R15Tests"); app.setApplicationName("CockpitTests");
    QTemporaryDir prefs;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, prefs.path());
    BikeState bike;
    DisplaySettings display;
    RideComputer ride(&bike);
    QQmlApplicationEngine engine;
    bool ok = true;
    auto check = [&](bool condition, const char *message) { if (!condition) { qCritical() << message; ok = false; } };
    QObject::connect(&engine, &QQmlEngine::warnings, [&](const QList<QQmlError> &warnings) {
        for (const auto &warning : warnings) qCritical() << warning;
        ok = false;
    });
    engine.setInitialProperties({{"bike", QVariant::fromValue(&bike)}, {"display", QVariant::fromValue(&display)}, {"ride", QVariant::fromValue(&ride)}});
    engine.load(QUrl::fromLocalFile(QStringLiteral(PROJECT_SOURCE_DIR "/ui/Main.qml")));
    if (engine.rootObjects().isEmpty()) return 1;
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    if (!window) return 1;
    QTest::qWait(100);
    auto key = [&](Qt::Key key) { QTest::keyClick(window, key); QCoreApplication::processEvents(); };
    auto screen = [&] { return window->property("screen").toString(); };
    key(Qt::Key_Right); key(Qt::Key_Up); key(Qt::Key_3);
    check(bike.speed() == 1 && bike.rpm() == 100 && bike.gear() == 3, "Dashboard key routing failed");
    key(Qt::Key_Left); key(Qt::Key_Down);
    check(bike.speed() == 0 && bike.rpm() == 0, "Decrement steps must match increment steps");
    key(Qt::Key_Right); key(Qt::Key_Up);
    const int initialFuel = bike.fuel();
    key(Qt::Key_BracketLeft); check(bike.fuel() == initialFuel - 1, "Fuel decrement must be one percent");
    key(Qt::Key_BracketRight); check(bike.fuel() == initialFuel, "Fuel increment must be one percent");
    key(Qt::Key_Q); key(Qt::Key_P);
    check(bike.quickShifter() && bike.phoneConnected(), "QS / phone keys failed");
    check(window->findChild<QObject *>("qsLamp")->property("visible").toBool(), "QS must display when enabled");
    bike.adjustRpm(7400);
    check(!window->findChild<QObject *>("vvaLamp")->property("visible").toBool(), "VVA visible at 7500");
    key(Qt::Key_Up);
    check(window->findChild<QObject *>("vvaLamp")->property("visible").toBool(), "VVA missing above 7500");
    key(Qt::Key_Tab);
    check(display.track(), "Track switch failed");
    key(Qt::Key_K); QTest::qWait(25); key(Qt::Key_K);
    check(ride.lapRunning() && ride.latestLap() > 0 && ride.fastestLap() == ride.latestLap(), "Lap record failed");
    QTest::qWait(20); key(Qt::Key_B); check(!ride.lapRunning() && ride.currentLap() > 0, "Lap stop must retain elapsed time");
    key(Qt::Key_Return); check(window->property("infoIndex").toInt() == 1, "Info page cycling failed");
    key(Qt::Key_S); check(screen() == "settings", "Settings did not open");
    const int rpm = bike.rpm();
    key(Qt::Key_Down); key(Qt::Key_Return);
    check(display.light() && bike.rpm() == rpm, "Menu input must change theme without changing RPM");
    key(Qt::Key_Down); key(Qt::Key_Return); check(display.imperial(), "Units toggle failed");
    key(Qt::Key_Down); key(Qt::Key_Return); check(screen() == "clock", "Clock editor did not open");
    const auto previousClock = display.now();
    key(Qt::Key_Up); key(Qt::Key_Escape);
    check(screen() == "settings" && qAbs(previousClock.secsTo(display.now())) < 2, "Clock cancel altered clock");
    check(!display.setClock(2026, 2, 30, 10, 0), "Invalid date accepted");
    check(display.setClock(2028, 2, 29, 10, 0), "Valid leap date rejected");
    check(display.now().date() == QDate(2028, 2, 29), "Clock date not applied");
    key(Qt::Key_Return); // Reopen the selected clock row.
    window->setProperty("clockDraft", QVariantList{2028, 2, 29, 10, 0});
    window->setProperty("clockField", 5); key(Qt::Key_Return);
    check(screen() == "settings" && display.now().date() == QDate(2028, 2, 29), "Clock save through UI failed");
    DisplaySettings reloaded;
    check(reloaded.track() && reloaded.light() && reloaded.imperial() && reloaded.now().date() == QDate(2028, 2, 29), "Preferences did not persist");
    display.useSystemClock();
    check(qAbs(display.now().secsTo(QDateTime::currentDateTime())) < 2, "Restore system clock failed");
    key(Qt::Key_Down); key(Qt::Key_Return); check(screen() == "info", "Vehicle info did not open");
    key(Qt::Key_Escape); key(Qt::Key_Escape); check(screen() == "dashboard", "Back navigation failed");
    bike.reset(); bike.adjustSpeed(60); ride.resetTrip(0); ride.resetTrip(1);
    ride.advance(60);
    check(qAbs(ride.tripA() - 1.0) < 0.0001 && qAbs(ride.tripB() - 1.0) < 0.0001, "Distance integration failed");
    const double odo = ride.odometer(); ride.resetTrip(0);
    check(ride.tripA() == 0 && ride.odometer() == odo && ride.tripB() > 0, "Trip reset affected other counters");
    ride.resetLaps(); check(ride.latestLap() == 0 && ride.fastestLap() == 0 && ride.lapNumber() == 0, "Lap reset failed");

    // Render sample states offscreen for visual review without controlling the user's desktop.
    QDir().mkpath("previews");
    auto capture = [&](const QString &name) {
        QTest::qWait(100);
        const QImage image = window->grabWindow();
        check(!image.isNull() && image.save("previews/" + name + ".png"), "Preview render failed");
    };
    bike.adjustRpm(8500); bike.toggleQuickShifter(); bike.togglePhone();
    display.setLight(false); display.setImperial(false); display.setTrack(false);
    capture("street");
    display.setTrack(true); capture("track");
    key(Qt::Key_S); capture("settings");
    window->setProperty("selection", 3); key(Qt::Key_Return); capture("clock");
    key(Qt::Key_Escape); window->setProperty("selection", 4); key(Qt::Key_Return); capture("vehicle");
    key(Qt::Key_Escape); key(Qt::Key_Escape); display.setLight(true); capture("daylight");
    key(Qt::Key_F1); capture("help");
    key(Qt::Key_F1); display.setLight(false); display.setTrack(false);
    bike.adjustFuel(-65); bike.adjustRpm(4500);
    bike.toggleAbs(); bike.toggleTcs(); bike.toggleCoolant(); bike.toggleEngineWarning();
    bike.toggleMessage(); bike.toggleCall(); bike.toggleHighBeam();
    capture("warnings");
    // Exact thresholds, both theme palettes, and deterministic red/normal flashing.
    for (bool light : {false, true}) {
        display.setLight(light);
        bike.reset(); bike.adjustRpm(10499);
        check(!window->property("shiftRed").toBool(), "Shift warning below threshold");
        bike.adjustRpm(1);
        check(window->property("shiftRed").toBool() && !window->property("shiftFlashing").toBool(), "10500 must be steady red");
        capture(light ? "shift-day" : "shift-night");
        bike.adjustRpm(899);
        check(window->property("shiftRed").toBool() && !window->property("shiftFlashing").toBool(), "11399 must stay steady");
        bike.adjustRpm(1);
        check(window->property("shiftRed").toBool() && window->property("shiftFlashing").toBool(), "11400 must begin flashing on red");
        QTest::qWait(330);
        check(!window->property("shiftRed").toBool(), "Shift flash must return to normal background");
        QTest::qWait(300);
        check(window->property("shiftRed").toBool(), "Shift flash must return to red");
        bike.adjustRpm(-1);
        check(window->property("shiftRed").toBool() && !window->property("shiftFlashing").toBool(), "Leaving flash range must return to steady red");
        bike.adjustRpm(-900);
        check(!window->property("shiftRed").toBool(), "Dropping below 10500 must restore theme");
    }
    key(Qt::Key_Z);
    check(bike.hazards(), "Hazard shortcut failed");
    auto *left = window->findChild<QObject *>("leftSignal");
    auto *right = window->findChild<QObject *>("rightSignal");
    window->setProperty("blink", true);
    check(left->property("active").toBool() && right->property("active").toBool(), "Hazards must illuminate together");
    window->setProperty("blink", false);
    check(!left->property("active").toBool() && !right->property("active").toBool(), "Hazards must extinguish together");
    key(Qt::Key_L); check(bike.leftIndicator() && !bike.rightIndicator() && !bike.hazards(), "Left key must cancel hazard");
    key(Qt::Key_R); check(!bike.leftIndicator() && bike.rightIndicator(), "Right key must cancel left");
    bike.toggleHighBeam(); bike.toggleAbs(); bike.toggleTcs(); bike.toggleCoolant(); bike.togglePhone(); bike.toggleQuickShifter();
    capture("daylight-badges");
    auto *fuel = window->findChild<QObject *>("fuelGauge");
    check(fuel != nullptr, "Fuel gauge component missing");
    if (!fuel) return 1;
    for (bool track : {false, true}) {
        for (bool light : {false, true}) {
            display.setTrack(track); display.setLight(light);
            bike.reset(); bike.adjustFuel(28);
            check(fuel->property("filledSegments").toInt() == 8, "Full fuel must light eight segments");
            bike.adjustFuel(-80);
            check(!fuel->property("lowFuel").toBool() && fuel->property("warningLit").toBool(), "20 percent must stay steady");
            bike.adjustFuel(-1);
            check(fuel->property("lowFuel").toBool() && fuel->property("warningLit").toBool(), "Below 20 must immediately begin lit warning");
            QTest::qWait(550);
            check(!fuel->property("warningLit").toBool(), "Low fuel must blink off");
            QTest::qWait(500);
            check(fuel->property("warningLit").toBool(), "Low fuel must continue blinking on");
            const QString suffix = QString(track ? "track" : "street") + (light ? "-day" : "-night");
            capture("fuel-low-" + suffix);
            fuel->setProperty("pulse", false); capture("fuel-low-off-" + suffix);
            bike.adjustFuel(-9);
            check(fuel->property("lowFuel").toBool() && !fuel->property("criticalFuel").toBool(), "10 percent must blink yellow");
            check(fuel->property("statusColor") == fuel->property("warning"), "10 percent must use yellow warning color");
            bike.adjustFuel(-1);
            check(fuel->property("lowFuel").toBool() && fuel->property("criticalFuel").toBool(), "Below 10 must keep blinking and turn red");
            check(fuel->property("statusColor") == fuel->property("critical"), "Critical fuel must use red color");
            fuel->setProperty("pulse", true); capture("fuel-critical-" + suffix);
            bike.adjustFuel(-9);
            check(fuel->property("lowFuel").toBool() && fuel->property("filledSegments").toInt() == 0, "Empty tank must retain blinking icon with empty gauge");
            fuel->setProperty("pulse", true); capture("fuel-empty-" + suffix);
            bike.adjustFuel(20);
            check(!fuel->property("lowFuel").toBool() && fuel->property("warningLit").toBool(), "Refueling to 20 must stop blinking and restore lit state");
            QTest::qWait(550);
            check(fuel->property("warningLit").toBool(), "Recovered fuel must remain steady");
            bike.adjustFuel(52); capture("icons-fuel-" + suffix);
        }
    }
    for (bool light : {false, true}) {
        display.setLight(light); display.setTrack(false); bike.reset();
        const QString suffix = light ? "day" : "night";
        capture("icons-inactive-" + suffix);
        auto *icon = left->findChild<QObject *>("tellTaleIcon");
        check(icon && qAbs(icon->property("opacity").toDouble() - 0.12) < 0.001, "Inactive tell-tale must be barely visible in both themes");
        bike.toggleLeft(); window->setProperty("blink", true);
        check(icon && icon->property("opacity").toDouble() == 1.0, "Active tell-tale must be fully opaque immediately");
        bike.toggleHighBeam(); bike.toggleAbs(); bike.toggleTcs(); bike.toggleCoolant();
        bike.toggleEngineWarning(); bike.togglePhone(); bike.toggleMessage(); bike.toggleCall();
        capture("icons-active-" + suffix);
    }
    return ok ? 0 : 1;
}
