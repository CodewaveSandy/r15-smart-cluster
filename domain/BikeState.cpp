#include "BikeState.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(stateLog, "cluster.state")
BikeState::BikeState(QObject *parent, int staleTimeoutMs)
    : QObject(parent), m_staleTimeoutMs(qMax(1, staleTimeoutMs)) {
    m_watchdog.setInterval(qMin(250, m_staleTimeoutMs));
    connect(&m_watchdog, &QTimer::timeout, this, [this] {
        if (available() && m_lastUpdate.elapsed() >= m_staleTimeoutMs)
            setAvailability(Availability::Stale, QStringLiteral("Data heartbeat expired"));
    });
    m_watchdog.start();
}
QString BikeState::dataStatus() const {
    switch (m_availability) {
    case Availability::Available: return QStringLiteral("Available");
    case Availability::Invalid: return QStringLiteral("Invalid data");
    case Availability::Stale: return QStringLiteral("Stale data");
    case Availability::Disconnected: return QStringLiteral("Disconnected");
    }
    return QStringLiteral("Unavailable");
}
void BikeState::setAvailability(Availability state, const QString &reason) {
    const QString boundedReason = reason.left(256);
    if (state == m_availability) return; // Repeated malformed frames must not flood logs/UI notifications.
    m_availability = state;
    if (!available()) qCWarning(stateLog) << dataStatus() << boundedReason;
    else qCInfo(stateLog) << "Data source available";
    emit availabilityChanged(); emit changed();
}
void BikeState::markDisconnected(const QString &reason) {
    setAvailability(Availability::Disconnected, reason);
}
bool BikeState::applySnapshot(const BikeSnapshot &s) {
    struct Range { const char *name; double value, minimum, maximum; };
    const Range ranges[] = {
        {"speedKph", double(s.speedKph), 0, 400}, {"engineRpm", double(s.engineRpm), 0, 30000},
        {"gear", double(s.gear), 0, 6}, {"fuelPercent", double(s.fuelPercent), 0, 100},
        {"phoneBatteryPercent", double(s.phoneBatteryPercent), -1, 100},
        {"batteryVoltageV", s.batteryVoltageV, 0, 60}, {"coolantTemperatureC", s.coolantTemperatureC, -40, 200}
    };
    for (const auto &range : ranges) {
        if (!qIsFinite(range.value) || range.value < range.minimum || range.value > range.maximum) {
            setAvailability(Availability::Invalid, QStringLiteral("Rejected %1=%2; software range %3..%4")
                .arg(QString::fromLatin1(range.name)).arg(range.value).arg(range.minimum).arg(range.maximum));
            return false;
        }
    }
    if ((s.phoneConnected && s.phoneBatteryPercent < 0) || (!s.hazards && s.leftIndicator && s.rightIndicator)) {
        setAvailability(Availability::Invalid, s.phoneConnected && s.phoneBatteryPercent < 0 ?
            QStringLiteral("Connected phone has unavailable battery") : QStringLiteral("Both direction flags set without hazards"));
        return false;
    }
    const auto &old = m_snapshot;
    const bool instruments = old.speedKph != s.speedKph || old.engineRpm != s.engineRpm || old.gear != s.gear || old.fuelPercent != s.fuelPercent ||
        old.batteryVoltageV != s.batteryVoltageV || old.coolantTemperatureC != s.coolantTemperatureC ||
        old.batteryVoltageAvailable != s.batteryVoltageAvailable || old.coolantTemperatureAvailable != s.coolantTemperatureAvailable;
    const bool lamps = old.hazards != s.hazards || old.leftIndicator != s.leftIndicator || old.rightIndicator != s.rightIndicator ||
        old.highBeam != s.highBeam || old.engineWarning != s.engineWarning || old.quickShifter != s.quickShifter ||
        old.absWarning != s.absWarning || old.tcsWarning != s.tcsWarning || old.coolantWarning != s.coolantWarning;
    const bool phone = old.phoneConnected != s.phoneConnected || old.phoneBatteryPercent != s.phoneBatteryPercent ||
        old.messagePending != s.messagePending || old.incomingCall != s.incomingCall;
    m_snapshot = s; m_lastUpdate.start();
    const bool recovered = !available();
    setAvailability(Availability::Available, {});
    if (instruments) emit instrumentsChanged();
    if (lamps) emit tellTalesChanged();
    if (phone) emit phoneChanged();
    if ((instruments || lamps || phone) && !recovered) emit changed();
    return true;
}
