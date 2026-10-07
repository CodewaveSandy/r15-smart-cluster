#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QDir>
#include <QtTest/QTest>
#include "domain/BikeState.h"
#include "data/simulator/SimulatorBikeDataSource.h"
#include "settings/DisplaySettings.h"
#include "domain/RideComputer.h"
#include "app/Fonts.h"
#include "application/AppDiagnostics.h"

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    if (!loadClusterFonts()) return 1;
    app.setOrganizationName("R15Tests"); app.setApplicationName("CockpitTests");
    QTemporaryDir prefs;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, prefs.path());
    BikeState bike;
    SimulatorBikeDataSource simulator;
    QObject::connect(&simulator, &IBikeDataSource::snapshotReady, &bike, &BikeState::applySnapshot);
    QObject::connect(&simulator, &IBikeDataSource::disconnected, &bike, &BikeState::markDisconnected);
    simulator.start();
    DisplaySettings display;
    RideComputer ride(bike, SimulatorBikeDataSource::kDemoOdometerKm);
    AppDiagnostics diagnostics;
    QQmlApplicationEngine engine;
    bool ok = true;
    auto check = [&](bool condition, const char *message) { if (!condition) { qCritical() << message; ok = false; } };
    QObject::connect(&engine, &QQmlEngine::warnings, [&](const QList<QQmlError> &warnings) {
        for (const auto &warning : warnings) qCritical() << warning;
        ok = false;
    });
    engine.setInitialProperties({{"bike", QVariant::fromValue(&bike)}, {"display", QVariant::fromValue(&display)}, {"ride", QVariant::fromValue(&ride)}, {"simulator", QVariant::fromValue(&simulator)}, {"diagnostics", QVariant::fromValue(&diagnostics)}});
    engine.load(QUrl::fromLocalFile(QStringLiteral(PROJECT_SOURCE_DIR "/ui/Main.qml")));
    if (engine.rootObjects().isEmpty()) return 1;
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    if (!window) return 1;
    QTest::qWait(100);
    auto key = [&](Qt::Key key) { QTest::keyClick(window, key); QCoreApplication::processEvents(); };
    auto screen = [&] { return window->property("screen").toString(); };
    key(Qt::Key_Right); key(Qt::Key_Up); key(Qt::Key_3);
    check(bike.speedKph() == 1 && bike.engineRpm() == 100 && bike.gear() == 3, "Dashboard key routing failed");
    key(Qt::Key_Left); key(Qt::Key_Down);
    check(bike.speedKph() == 0 && bike.engineRpm() == 0, "Decrement steps must match increment steps");
    key(Qt::Key_Right); key(Qt::Key_Up);
    const int initialFuel = bike.fuelPercent();
    key(Qt::Key_BracketLeft); check(bike.fuelPercent() == initialFuel - 1, "Fuel decrement must be one percent");
    key(Qt::Key_BracketRight); check(bike.fuelPercent() == initialFuel, "Fuel increment must be one percent");
    key(Qt::Key_Q); key(Qt::Key_P);
    check(bike.quickShifter() && bike.phoneConnected(), "QS / phone keys failed");
    check(window->findChild<QObject *>("qsLamp")->property("visible").toBool(), "QS must display when enabled");
    simulator.adjustRpm(7400);
    check(!window->findChild<QObject *>("vvaLamp")->property("visible").toBool(), "VVA visible at 7500");
    key(Qt::Key_Up);
    check(window->findChild<QObject *>("vvaLamp")->property("visible").toBool(), "VVA missing above 7500");
    key(Qt::Key_Tab);
    check(display.track(), "Track switch failed");
    key(Qt::Key_K); QTest::qWait(25); key(Qt::Key_K);
    check(ride.lapRunning() && ride.latestLapMs() > 0 && ride.fastestLapMs() == ride.latestLapMs(), "Lap record failed");
    QTest::qWait(20); key(Qt::Key_B); check(!ride.lapRunning() && ride.currentLapMs() > 0, "Lap stop must retain elapsed time");
    key(Qt::Key_Return); check(window->property("infoIndex").toInt() == 1, "Info page cycling failed");
    key(Qt::Key_S); check(screen() == "settings", "Settings did not open");
    const int rpm = bike.engineRpm();
    key(Qt::Key_Down); key(Qt::Key_Return);
    check(display.light() && bike.engineRpm() == rpm, "Menu input must change theme without changing RPM");
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
    // Preference writes are asynchronous; wait on persisted state, not an assumed delay.
    QElapsedTimer persistenceWait; persistenceWait.start();
    while (persistenceWait.elapsed() < 2000) {
        QSettings stored;
        if (stored.value("display/track").toBool() && stored.value("display/light").toBool() &&
            stored.value("display/imperial").toBool() && stored.value("display/clockOffset").toLongLong() != 0) break;
        QTest::qWait(10);
    }
    DisplaySettings reloaded;
    check(reloaded.track() && reloaded.light() && reloaded.imperial() && reloaded.now().date() == QDate(2028, 2, 29), "Preferences did not persist");
    display.useSystemClock();
    check(qAbs(display.now().secsTo(QDateTime::currentDateTime())) < 2, "Restore system clock failed");
    key(Qt::Key_Down); key(Qt::Key_Return); check(screen() == "info", "Vehicle info did not open");
    key(Qt::Key_Escape); key(Qt::Key_Escape); check(screen() == "dashboard", "Back navigation failed");
    simulator.reset(); simulator.adjustSpeed(60); ride.resetTrip(0); ride.resetTrip(1);
    ride.advance(60);
    check(qAbs(ride.tripAKm() - 1.0) < 0.0001 && qAbs(ride.tripBKm() - 1.0) < 0.0001, "Distance integration failed");
    const double odo = ride.odometerKm(); ride.resetTrip(0);
    check(ride.tripAKm() == 0 && ride.odometerKm() == odo && ride.tripBKm() > 0, "Trip reset affected other counters");
    ride.resetLaps(); check(ride.latestLapMs() == 0 && ride.fastestLapMs() == 0 && ride.lapNumber() == 0, "Lap reset failed");

    // Render sample states offscreen for visual review without controlling the user's desktop.
    QDir().mkpath("previews");
    auto capture = [&](const QString &name) {
        QTest::qWait(100);
        const QImage image = window->grabWindow();
        check(!image.isNull() && image.save("previews/" + name + ".png"), "Preview render failed");
    };
    simulator.adjustRpm(8500); simulator.toggleQuickShifter(); simulator.togglePhone();
    display.setLight(false); display.setImperial(false); display.setTrack(false);
    capture("street");
    display.setTrack(true); capture("track");
    key(Qt::Key_S); capture("settings");
    window->setProperty("selection", 3); key(Qt::Key_Return); capture("clock");
    key(Qt::Key_Escape); window->setProperty("selection", 4); key(Qt::Key_Return); capture("vehicle");
    key(Qt::Key_Escape); key(Qt::Key_Escape); display.setLight(true); capture("daylight");
    key(Qt::Key_F1); capture("help");
    key(Qt::Key_F1); display.setLight(false); display.setTrack(false);
    simulator.adjustFuel(-65); simulator.adjustRpm(4500);
    simulator.toggleAbs(); simulator.toggleTcs(); simulator.toggleCoolant(); simulator.toggleEngineWarning();
    simulator.toggleMessage(); simulator.toggleCall(); simulator.toggleHighBeam();
    capture("warnings");
    // Exact thresholds, both theme palettes, and deterministic red/normal flashing.
    for (bool light : {false, true}) {
        display.setLight(light);
        simulator.reset(); simulator.adjustRpm(10499);
        check(!window->property("shiftRed").toBool(), "Shift warning below threshold");
        simulator.adjustRpm(1);
        check(window->property("shiftRed").toBool() && !window->property("shiftFlashing").toBool(), "10500 must be steady red");
        capture(light ? "shift-day" : "shift-night");
        simulator.adjustRpm(899);
        check(window->property("shiftRed").toBool() && !window->property("shiftFlashing").toBool(), "11399 must stay steady");
        simulator.adjustRpm(1);
        check(window->property("shiftRed").toBool() && window->property("shiftFlashing").toBool(), "11400 must begin flashing on red");
        QTest::qWait(330);
        check(!window->property("shiftRed").toBool(), "Shift flash must return to normal background");
        QTest::qWait(300);
        check(window->property("shiftRed").toBool(), "Shift flash must return to red");
        simulator.adjustRpm(-1);
        check(window->property("shiftRed").toBool() && !window->property("shiftFlashing").toBool(), "Leaving flash range must return to steady red");
        simulator.adjustRpm(-900);
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
    simulator.toggleHighBeam(); simulator.toggleAbs(); simulator.toggleTcs(); simulator.toggleCoolant(); simulator.togglePhone(); simulator.toggleQuickShifter();
    capture("daylight-badges");
    auto *fuel = window->findChild<QObject *>("fuelGauge");
    check(fuel != nullptr, "Fuel gauge component missing");
    if (!fuel) return 1;
    for (bool track : {false, true}) {
        for (bool light : {false, true}) {
            display.setTrack(track); display.setLight(light);
            simulator.reset(); simulator.adjustFuel(28);
            check(fuel->property("filledSegments").toInt() == 8, "Full fuel must light eight segments");
            simulator.adjustFuel(-80);
            check(!fuel->property("lowFuel").toBool() && fuel->property("warningLit").toBool(), "20 percent must stay steady");
            simulator.adjustFuel(-1);
            check(fuel->property("lowFuel").toBool() && fuel->property("warningLit").toBool(), "Below 20 must immediately begin lit warning");
            QTest::qWait(550);
            check(!fuel->property("warningLit").toBool(), "Low fuel must blink off");
            QTest::qWait(500);
            check(fuel->property("warningLit").toBool(), "Low fuel must continue blinking on");
            const QString suffix = QString(track ? "track" : "street") + (light ? "-day" : "-night");
            capture("fuel-low-" + suffix);
            fuel->setProperty("pulse", false); capture("fuel-low-off-" + suffix);
            simulator.adjustFuel(-9);
            check(fuel->property("lowFuel").toBool() && !fuel->property("criticalFuel").toBool(), "10 percent must blink yellow");
            check(fuel->property("statusColor") == fuel->property("warning"), "10 percent must use yellow warning color");
            simulator.adjustFuel(-1);
            check(fuel->property("lowFuel").toBool() && fuel->property("criticalFuel").toBool(), "Below 10 must keep blinking and turn red");
            check(fuel->property("statusColor") == fuel->property("critical"), "Critical fuel must use red color");
            fuel->setProperty("pulse", true); capture("fuel-critical-" + suffix);
            simulator.adjustFuel(-9);
            check(fuel->property("lowFuel").toBool() && fuel->property("filledSegments").toInt() == 0, "Empty tank must retain blinking icon with empty gauge");
            fuel->setProperty("pulse", true); capture("fuel-empty-" + suffix);
            simulator.adjustFuel(20);
            check(!fuel->property("lowFuel").toBool() && fuel->property("warningLit").toBool(), "Refueling to 20 must stop blinking and restore lit state");
            QTest::qWait(550);
            check(fuel->property("warningLit").toBool(), "Recovered fuel must remain steady");
            simulator.adjustFuel(52); capture("icons-fuel-" + suffix);
        }
    }
    for (bool light : {false, true}) {
        display.setLight(light); display.setTrack(false); simulator.reset();
        const QString suffix = light ? "day" : "night";
        capture("icons-inactive-" + suffix);
        auto *icon = left->findChild<QObject *>("tellTaleIcon");
        check(icon && qAbs(icon->property("opacity").toDouble() - 0.12) < 0.001, "Inactive tell-tale must be barely visible in both themes");
        simulator.toggleLeft(); window->setProperty("blink", true);
        check(icon && icon->property("opacity").toDouble() == 1.0, "Active tell-tale must be fully opaque immediately");
        simulator.toggleHighBeam(); simulator.toggleAbs(); simulator.toggleTcs(); simulator.toggleCoolant();
        simulator.toggleEngineWarning(); simulator.togglePhone(); simulator.toggleMessage(); simulator.toggleCall();
        capture("icons-active-" + suffix);
    }
    simulator.stop();
    check(!bike.available(), "Source stop must invalidate live readings");
    for (bool track : {false, true}) {
        for (bool light : {false, true}) {
            display.setTrack(track); display.setLight(light);
            capture(QString("source-unavailable-") + (track ? "track" : "street") + (light ? "-day" : "-night"));
            check(window->findChild<QObject *>("speedValue")->property("text").toString() == "—", "Unavailable speed must not look like zero");
            check(!fuel->property("available").toBool() && fuel->property("filledSegments").toInt() == 0,
                  "Unavailable fuel must not look like a known remaining level");
        }
    }
    simulator.start();
    check(bike.available(), "Source restart must restore live readings");
    BikeSnapshot missingOptional;
    missingOptional.speedKph = 70; missingOptional.engineRpm = 8000; missingOptional.fuelPercent = 72;
    bike.applySnapshot(missingOptional);
    display.setTrack(false); display.setLight(true);
    check(bike.available() && !bike.batteryVoltageAvailable() && !bike.coolantTemperatureAvailable(), "Optional missing sensors must not invalidate supported instruments");
    capture("optional-sensors-unavailable");
    simulator.reset();
    window->setProperty("screen", "unknown-screen");
    check(screen() == "dashboard" && !diagnostics.uiError().isEmpty(), "Invalid navigation must recover and report diagnostics");
    capture("ui-error-banner");
    QQmlApplicationEngine fallback;
    QObject::connect(&fallback, &QQmlEngine::warnings, [&](const QList<QQmlError> &warnings) {
        for (const auto &warning : warnings) qCritical() << warning;
        ok = false;
    });
    fallback.load(QUrl::fromLocalFile(QStringLiteral(PROJECT_SOURCE_DIR "/ui/ErrorScreen.qml")));
    check(!fallback.rootObjects().isEmpty(), "Independent startup failure screen must load");
    if (!fallback.rootObjects().isEmpty()) {
        auto *failureWindow = qobject_cast<QQuickWindow *>(fallback.rootObjects().first());
        QTest::qWait(100);
        check(failureWindow && failureWindow->grabWindow().save("previews/startup-failure.png"), "Failure screen must render");
    }
    return ok ? 0 : 1;
}
