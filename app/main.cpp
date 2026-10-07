#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTimer>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <QCommandLineParser>
#include <exception>
#include "domain/BikeState.h"
#include "data/simulator/SimulatorBikeDataSource.h"
#include "settings/DisplaySettings.h"
#include "domain/RideComputer.h"
#include "application/AppDiagnostics.h"
#include "diagnostics/DiagnosticLog.h"
#include "Fonts.h"

Q_LOGGING_CATEGORY(appLog, "cluster.app")
int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("R15 Smart Cluster");
    app.setOrganizationName("R15SmartCluster");
    QCommandLineParser options;
    options.addHelpOption();
    options.addOption({"smoke-test", "Exit after startup verification"});
    options.addOption({"test-startup-failure", "Inject a primary UI startup failure (test only)"});
    options.addOption({"log-directory", "Override the diagnostic log directory", "directory"});
    options.process(app);
    try {
        const QString logDirectory = options.isSet("log-directory") ? options.value("log-directory") :
            QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/logs";
        DiagnosticLog log(logDirectory);
        qCInfo(appLog) << "Starting desktop simulator; Qt" << qVersion() << "log" << log.filePath();
        qRegisterMetaType<BikeSnapshot>();
        try {
            const bool fontsReady = loadClusterFonts();
            BikeState bike;
            SimulatorBikeDataSource simulator;
            QObject::connect(&simulator, &IBikeDataSource::snapshotReady, &bike, &BikeState::applySnapshot);
            QObject::connect(&simulator, &IBikeDataSource::disconnected, &bike, &BikeState::markDisconnected);
            simulator.start();
            DisplaySettings display;
            RideComputer ride(bike, SimulatorBikeDataSource::kDemoOdometerKm);
            AppDiagnostics diagnostics;
            QQmlApplicationEngine engine;
            QObject::connect(&engine, &QQmlEngine::warnings, &diagnostics, [&diagnostics](const QList<QQmlError> &errors) {
                for (const auto &error : errors) diagnostics.reportUiError(error.toString());
            });
            engine.setInitialProperties({{"bike", QVariant::fromValue(&bike)}, {"display", QVariant::fromValue(&display)},
                {"ride", QVariant::fromValue(&ride)}, {"simulator", QVariant::fromValue(&simulator)},
                {"diagnostics", QVariant::fromValue(&diagnostics)}});
            if (fontsReady && !options.isSet("test-startup-failure")) engine.loadFromModule("R15Cluster", "Main");
            const bool startupFailed = engine.rootObjects().isEmpty();
            QQmlApplicationEngine fallback;
            if (startupFailed) {
                qCCritical(appLog) << "Primary UI unavailable; opening independent failure screen";
                fallback.loadFromModule("R15Cluster", "ErrorScreen");
                if (fallback.rootObjects().isEmpty()) { qCCritical(appLog) << "Failure screen also unavailable"; return 1; }
            }
            if (options.isSet("smoke-test")) QTimer::singleShot(1000, &app, &QCoreApplication::quit);
            const int result = app.exec();
            simulator.stop();
            qCInfo(appLog) << "Shutting down";
            return startupFailed ? 1 : result;
        } catch (const std::exception &error) {
            qCCritical(appLog) << "Application boundary exception:" << error.what();
            return 1;
        } catch (...) {
            qCCritical(appLog) << "Unknown application boundary exception";
            return 1;
        }
    } catch (const std::exception &error) {
        qCCritical(appLog) << "Diagnostic startup exception:" << error.what();
        return 1;
    } catch (...) {
        qCCritical(appLog) << "Unknown diagnostic startup exception";
        return 1;
    }
}
