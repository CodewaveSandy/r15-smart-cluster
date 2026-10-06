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
    check(bike.speed() == 5 && bike.rpm() == 500 && bike.gear() == 3, "Dashboard key routing failed");
    key(Qt::Key_Q); key(Qt::Key_P);
    check(bike.quickShifter() && bike.phoneConnected(), "QS / phone keys failed");
    check(window->findChild<QObject *>("qsLamp")->property("visible").toBool(), "QS must display when enabled");
    bike.adjustRpm(7000);
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
    return ok ? 0 : 1;
}
