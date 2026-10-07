#include "SimulatorBikeDataSource.h"
#include <QLoggingCategory>
Q_LOGGING_CATEGORY(simLog, "cluster.simulator")
namespace {
BikeSnapshot demoSnapshot() {
    BikeSnapshot snapshot;
    snapshot.fuelPercent = 72;
    snapshot.batteryVoltageV = 12.8;
    snapshot.coolantTemperatureC = 84;
    snapshot.batteryVoltageAvailable = snapshot.coolantTemperatureAvailable = true;
    return snapshot;
}
int adjusted(int value, int delta, int maximum) {
    return static_cast<int>(qBound<qint64>(qint64(0), qint64(value) + delta, qint64(maximum)));
}
}
SimulatorBikeDataSource::SimulatorBikeDataSource(QObject *parent) : IBikeDataSource(parent) {
    m_snapshot = demoSnapshot(); // Desktop samples, never a vehicle fallback.
    m_heartbeat.setInterval(1000);
    connect(&m_heartbeat, &QTimer::timeout, this, &SimulatorBikeDataSource::publish);
}
void SimulatorBikeDataSource::start() {
    if (m_running) return;
    m_running = true; m_heartbeat.start(); publish();
    qCInfo(simLog) << "Desktop simulator started";
}
void SimulatorBikeDataSource::stop() {
    if (!m_running) return;
    m_running = false; m_heartbeat.stop();
    emit disconnected(QStringLiteral("Simulator stopped"));
}
void SimulatorBikeDataSource::publish() { if (m_running) emit snapshotReady(m_snapshot); }
void SimulatorBikeDataSource::adjustSpeed(int delta) { m_snapshot.speedKph = adjusted(m_snapshot.speedKph, delta, 180); publish(); }
void SimulatorBikeDataSource::adjustRpm(int delta) { m_snapshot.engineRpm = adjusted(m_snapshot.engineRpm, delta, 13000); publish(); }
void SimulatorBikeDataSource::adjustFuel(int delta) { m_snapshot.fuelPercent = adjusted(m_snapshot.fuelPercent, delta, 100); publish(); }
void SimulatorBikeDataSource::setGear(int gear) { m_snapshot.gear = qBound(0, gear, 6); publish(); }
void SimulatorBikeDataSource::toggleLeft() {
    const bool enable = m_snapshot.hazards || !m_snapshot.leftIndicator;
    m_snapshot.hazards = false; m_snapshot.leftIndicator = enable; m_snapshot.rightIndicator = false; publish();
}
void SimulatorBikeDataSource::toggleRight() {
    const bool enable = m_snapshot.hazards || !m_snapshot.rightIndicator;
    m_snapshot.hazards = false; m_snapshot.rightIndicator = enable; m_snapshot.leftIndicator = false; publish();
}
void SimulatorBikeDataSource::toggleHazards() {
    m_snapshot.hazards = !m_snapshot.hazards; m_snapshot.leftIndicator = m_snapshot.rightIndicator = false; publish();
}
void SimulatorBikeDataSource::togglePhone() {
    m_snapshot.phoneConnected = !m_snapshot.phoneConnected;
    m_snapshot.phoneBatteryPercent = m_snapshot.phoneConnected ? 84 : -1; publish();
}
void SimulatorBikeDataSource::reset() { m_snapshot = demoSnapshot(); publish(); }
void SimulatorBikeDataSource::toggleHighBeam() { m_snapshot.highBeam = !m_snapshot.highBeam; publish(); }
void SimulatorBikeDataSource::toggleEngineWarning() { m_snapshot.engineWarning = !m_snapshot.engineWarning; publish(); }
void SimulatorBikeDataSource::toggleQuickShifter() { m_snapshot.quickShifter = !m_snapshot.quickShifter; publish(); }
void SimulatorBikeDataSource::toggleAbs() { m_snapshot.absWarning = !m_snapshot.absWarning; publish(); }
void SimulatorBikeDataSource::toggleTcs() { m_snapshot.tcsWarning = !m_snapshot.tcsWarning; publish(); }
void SimulatorBikeDataSource::toggleCoolant() { m_snapshot.coolantWarning = !m_snapshot.coolantWarning; m_snapshot.coolantTemperatureC = m_snapshot.coolantWarning ? 115 : 84; publish(); }
void SimulatorBikeDataSource::toggleMessage() { m_snapshot.messagePending = !m_snapshot.messagePending; publish(); }
void SimulatorBikeDataSource::toggleCall() { m_snapshot.incomingCall = !m_snapshot.incomingCall; publish(); }
