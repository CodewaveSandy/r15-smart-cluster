#include <QCoreApplication>
#include <QTemporaryDir>
#include <QSettings>
#include <QFile>
#include <QDir>
#include <QSignalSpy>
#include <QtTest/QTest>
#include <functional>
#include <limits>
#include <memory>
#include "domain/BikeState.h"
#include "domain/RideComputer.h"
#include "settings/DisplaySettings.h"
#include "application/AppDiagnostics.h"
#include "diagnostics/DiagnosticLog.h"

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    app.setOrganizationName("R15Tests"); app.setApplicationName("BoundaryTests");
    bool ok = true;
    auto check = [&](bool value, const char *message) { if (!value) { qCritical() << message; ok = false; } };
    auto waitUntil = [](const std::function<bool()> &condition) {
        QElapsedTimer elapsed; elapsed.start();
        while (!condition() && elapsed.elapsed() < 2000) QTest::qWait(10);
        return condition();
    };
    BikeState bike;
    check(!bike.available(), "State must start unavailable before a source publishes");
    BikeSnapshot good; good.speedKph = 42; good.fuelPercent = 72;
    check(bike.applySnapshot(good) && bike.available(), "Valid normalized snapshot rejected");
    QSignalSpy aggregate(&bike, &BikeState::changed);
    QSignalSpy instruments(&bike, &BikeState::instrumentsChanged);
    QSignalSpy lamps(&bike, &BikeState::tellTalesChanged);
    bike.applySnapshot(good);
    check(aggregate.isEmpty(), "Repeated identical heartbeats must not notify UI");
    auto faster = good; faster.speedKph = 43;
    bike.applySnapshot(faster);
    check(instruments.count() == 1 && lamps.isEmpty(), "Instrument change must not notify unrelated lamps");
    const std::function<void(BikeSnapshot &)> invalidCases[] = {
        [](auto &s) { s.batteryVoltageV = std::numeric_limits<double>::quiet_NaN(); },
        [](auto &s) { s.coolantTemperatureC = std::numeric_limits<double>::infinity(); },
        [](auto &s) { s.batteryVoltageV = 61; }, [](auto &s) { s.coolantTemperatureC = 201; },
        [](auto &s) { s.speedKph = -1; }, [](auto &s) { s.speedKph = 401; },
        [](auto &s) { s.engineRpm = -1; }, [](auto &s) { s.engineRpm = 30001; },
        [](auto &s) { s.fuelPercent = -1; }, [](auto &s) { s.fuelPercent = 101; },
        [](auto &s) { s.gear = -1; }, [](auto &s) { s.gear = 7; },
        [](auto &s) { s.phoneConnected = true; s.phoneBatteryPercent = -1; },
        [](auto &s) { s.phoneBatteryPercent = 101; },
        [](auto &s) { s.leftIndicator = s.rightIndicator = true; }
    };
    for (const auto &mutate : invalidCases) {
        auto bad = good; mutate(bad);
        check(!bike.applySnapshot(bad) && bike.availability() == BikeState::Availability::Invalid && bike.speedKph() == 43,
              "Invalid packet must be rejected, preserve observations, and mark unavailable");
        bike.applySnapshot(faster);
        check(bike.available(), "Valid data must recover after invalid packet");
    }
    auto repeatedInvalid = good; repeatedInvalid.fuelPercent = 101;
    bike.applySnapshot(repeatedInvalid);
    const int invalidNotifications = aggregate.count();
    repeatedInvalid.fuelPercent = 102; bike.applySnapshot(repeatedInvalid);
    check(aggregate.count() == invalidNotifications, "Repeated invalid frames must not spam notifications");
    bike.applySnapshot(faster);
    auto maximum = good; maximum.speedKph = 400; maximum.engineRpm = 30000; maximum.fuelPercent = 100; maximum.gear = 6;
    check(bike.applySnapshot(maximum), "Software validation maxima rejected");
    bike.markDisconnected("Test source disconnected");
    RideComputer ride(bike);
    const auto odo = ride.odometerKm(); ride.advance(60);
    check(ride.odometerKm() == odo, "Disconnected speed must not accumulate distance");
    bike.applySnapshot(good);
    ride.advance(std::numeric_limits<double>::infinity()); ride.advance(-1); ride.advance(1e300);
    check(ride.odometerKm() == odo, "Invalid durations must not mutate ride state");
    ride.advance(60); check(ride.odometerKm() > odo, "Valid duration must still accumulate distance");
    const auto trip = ride.tripAKm(); ride.resetTrip(7); check(ride.tripAKm() == trip, "Unknown trip index must be rejected");
    BikeState shortTimeout(nullptr, 40);
    shortTimeout.applySnapshot(good);
    check(waitUntil([&] { return shortTimeout.availability() == BikeState::Availability::Stale; }), "Missing heartbeat must mark stale");
    shortTimeout.applySnapshot(good); check(shortTimeout.available(), "Fresh snapshot must recover stale state");
    auto temporaryState = std::make_unique<BikeState>(); temporaryState->applySnapshot(good);
    RideComputer lifetimeRide(*temporaryState); temporaryState.reset(); lifetimeRide.advance(60);
    check(lifetimeRide.odometerKm() == 0, "Destroyed source must not be dereferenced");

    QTemporaryDir preferences;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, preferences.path());
    {
        QSettings persisted;
        persisted.setValue("display/light", "malformed");
        persisted.setValue("display/clockOffset", std::numeric_limits<qint64>::max()); persisted.sync();
        DisplaySettings settings;
        check(!settings.light() && qAbs(settings.now().secsTo(QDateTime::currentDateTime())) < 2, "Corrupt preferences must be validated");
        QSignalSpy writes(&settings, &DisplaySettings::persistenceFinished);
        settings.setLight(true);
        check(waitUntil([&] { return writes.count() == 1; }) && settings.persistenceError().isEmpty(), "Async preference write failed");
        settings.setLight(true); QTest::qWait(20);
        check(writes.count() == 1, "No-op preferences must not create writes");
        DisplaySettings restored; check(restored.light(), "Async persisted preference not restored");
        check(!settings.setClock(2026, 2, 30, 12, 0), "Invalid calendar date accepted");
    }
    {
        DisplaySettings burst;
        for (int i = 0; i < 100; ++i) burst.setTrack(i % 2 == 0);
        burst.setTrack(true); burst.setLight(false);
        // Destruction must drain both in-flight and coalesced writes without an event loop tick.
    }
    { DisplaySettings restored; check(restored.track() && !restored.light(), "Shutdown must flush latest coalesced preferences"); }
    {
        const QString blocked = preferences.path() + "/blocked";
        QFile file(blocked); check(file.open(QIODevice::WriteOnly), "Cannot set up failed-write fixture"); file.close();
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, blocked);
        DisplaySettings settings; QSignalSpy writes(&settings, &DisplaySettings::persistenceFinished);
        settings.setLight(true);
        check(waitUntil([&] { return writes.count() > 0; }) && !settings.persistenceError().isEmpty() && settings.light(),
              "Write failure must be visible while keeping in-memory preferences usable");
    }
    AppDiagnostics diagnostics;
    diagnostics.reportUiError("Injected runtime QML failure");
    check(!diagnostics.uiError().isEmpty(), "UI diagnostic must be observable");
    {
        QTemporaryDir logs;
        const QString logPath = logs.path() + "/cluster.log";
        {
            DiagnosticLog logger(logs.path(), 1024);
            for (int i = 0; i < 30; ++i) qWarning().noquote() << "Boundary log marker" << i << QString(100, 'x');
        }
        QFile file(logPath);
        check(file.open(QIODevice::ReadOnly) && file.readAll().contains("Boundary log marker"), "Diagnostic log must drain to disk on shutdown");
        check(QFile::exists(logPath + ".1"), "Diagnostic log must rotate");
        for (const auto &name : {logPath, logPath + ".1", logPath + ".2"}) check(QFile(name).size() <= 1024, "Rotating log exceeded configured bound");
        QFile blocked(logs.path() + "/blocked"); check(blocked.open(QIODevice::WriteOnly), "Cannot create unavailable-log fixture"); blocked.close();
        { DiagnosticLog unavailable(logs.path() + "/blocked"); qWarning() << "Disk unavailable test"; }
    }
    return ok ? 0 : 1;
}
