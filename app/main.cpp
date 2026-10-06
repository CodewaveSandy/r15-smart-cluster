#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTimer>
#include "BikeState.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("R15 Smart Cluster");
    BikeState bike;
    QQmlApplicationEngine engine;
    engine.setInitialProperties({{"bike", QVariant::fromValue(&bike)}});
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("R15Cluster", "Main");
    // Useful for a startup check without leaving a process running.
    if (app.arguments().contains("--smoke-test"))
        QTimer::singleShot(1000, &app, &QCoreApplication::quit);
    return app.exec();
}
