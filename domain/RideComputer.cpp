#include "RideComputer.h"
#include <QLoggingCategory>
#include <limits>
Q_LOGGING_CATEGORY(rideLog, "cluster.ride")
RideComputer::RideComputer(BikeState &bike, double initialOdometerKm, QObject *parent) : QObject(parent), m_bike(&bike) {
    if (qIsFinite(initialOdometerKm) && initialOdometerKm >= 0) m_initialOdometerKm = initialOdometerKm;
    else qCWarning(rideLog) << "Rejected invalid initial session distance";
    m_tick.start();
    connect(&bike, &QObject::destroyed, this, [this] {
        m_timer.stop(); qCWarning(rideLog) << "State source destroyed; ride accumulation stopped";
    });
    connect(&m_timer, &QTimer::timeout, this, [this] {
        const qint64 ms = m_tick.restart();
        // Ignore host sleep, and do not integrate last-known unavailable speed.
        if (ms <= 2000) advance(ms / 1000.0);
        if (lapRunning()) emit changed();
    });
    m_timer.start(100);
}
void RideComputer::advance(double seconds) {
    if (!qIsFinite(seconds) || seconds < 0 || seconds > 86400) {
        qCWarning(rideLog) << "Rejected invalid ride interval"; return;
    }
    if (!m_bike || !m_bike->available()) return;
    const double km = m_bike->speedKph() * seconds / 3600;
    const double reserve = m_bike->fuelPercent() <= 15 ? m_reserve + km : 0;
    if (!qIsFinite(m_distance + km) || !qIsFinite(m_tripA + km) || !qIsFinite(m_tripB + km) || !qIsFinite(reserve)) {
        qCWarning(rideLog) << "Distance overflow rejected"; return;
    }
    const bool modified = km != 0 || reserve != m_reserve;
    m_distance += km; m_tripA += km; m_tripB += km; m_reserve = reserve;
    if (modified) emit changed();
}
void RideComputer::resetTrip(int trip) {
    if (trip != 0 && trip != 1) { qCWarning(rideLog) << "Rejected unknown trip index" << trip; return; }
    double &value = trip == 0 ? m_tripA : m_tripB;
    if (value == 0) return;
    value = 0; emit changed();
}
void RideComputer::lapAction() {
    if (m_laps == std::numeric_limits<int>::max() - 1) { qCWarning(rideLog) << "Lap count limit reached"; return; }
    if (lapRunning()) {
        m_latest = m_lap.elapsed();
        if (m_latest > 0 && (m_fastest == 0 || m_latest < m_fastest)) m_fastest = m_latest;
        ++m_laps;
    }
    m_stopped = 0; m_lap.start(); emit changed();
}
void RideComputer::stopLap() {
    if (!lapRunning()) return;
    m_stopped = m_lap.elapsed(); m_lap.invalidate(); emit changed();
}
void RideComputer::resetLaps() {
    if (!lapRunning() && m_latest == 0 && m_fastest == 0 && m_stopped == 0 && m_laps == 0) return;
    m_lap.invalidate(); m_latest = m_fastest = m_stopped = 0; m_laps = 0; emit changed();
}
