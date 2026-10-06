#pragma once
#include <QObject>
#include <QElapsedTimer>
#include <QTimer>
#include "BikeState.h"

// Session-only bench values. Never imports or overwrites a real odometer.
class RideComputer : public QObject {
    Q_OBJECT
    Q_PROPERTY(double odometer READ odometer NOTIFY changed)
    Q_PROPERTY(double tripA READ tripA NOTIFY changed)
    Q_PROPERTY(double tripB READ tripB NOTIFY changed)
    Q_PROPERTY(double reserveTrip READ reserveTrip NOTIFY changed)
    Q_PROPERTY(bool lapRunning READ lapRunning NOTIFY changed)
    Q_PROPERTY(int lapNumber READ lapNumber NOTIFY changed)
    Q_PROPERTY(qint64 currentLap READ currentLap NOTIFY changed)
    Q_PROPERTY(qint64 latestLap READ latestLap NOTIFY changed)
    Q_PROPERTY(qint64 fastestLap READ fastestLap NOTIFY changed)
public:
    explicit RideComputer(BikeState *bike, QObject *parent = nullptr) : QObject(parent), m_bike(bike) {
        m_tick.start();
        connect(&m_timer, &QTimer::timeout, this, [this] {
            const qint64 ms = m_tick.restart();
            // Do not count host sleep/suspension as a simulated ride.
            if (ms <= 2000) advance(ms / 1000.0);
            emit changed();
        });
        m_timer.start(100);
    }
    double odometer() const { return 12480.0 + m_distance; }
    double tripA() const { return m_tripA; }
    double tripB() const { return m_tripB; }
    double reserveTrip() const { return m_reserve; }
    bool lapRunning() const { return m_lap.isValid(); }
    int lapNumber() const { return m_laps + (lapRunning() || m_stopped > 0 ? 1 : 0); }
    qint64 currentLap() const { return lapRunning() ? m_lap.elapsed() : m_stopped; }
    qint64 latestLap() const { return m_latest; }
    qint64 fastestLap() const { return m_fastest; }
    void advance(double seconds) {
        if (!qIsFinite(seconds) || seconds < 0) return;
        const double km = m_bike->speed() * seconds / 3600;
        m_distance += km; m_tripA += km; m_tripB += km;
        if (m_bike->fuel() <= 15) m_reserve += km; else m_reserve = 0;
    }
    Q_INVOKABLE void resetTrip(int trip) { if (trip == 0) m_tripA = 0; if (trip == 1) m_tripB = 0; emit changed(); }
    Q_INVOKABLE void lapAction() {
        if (lapRunning()) {
            m_latest = m_lap.elapsed();
            if (m_latest > 0 && (m_fastest == 0 || m_latest < m_fastest)) m_fastest = m_latest;
            ++m_laps;
        }
        m_stopped = 0; m_lap.start(); emit changed();
    }
    Q_INVOKABLE void stopLap() { if (lapRunning()) m_stopped = m_lap.elapsed(); m_lap.invalidate(); emit changed(); }
    Q_INVOKABLE void resetLaps() { m_lap.invalidate(); m_latest = m_fastest = m_stopped = 0; m_laps = 0; emit changed(); }
private:
    BikeState *m_bike;
    QTimer m_timer;
    QElapsedTimer m_tick, m_lap;
    double m_distance = 0, m_tripA = 0, m_tripB = 0, m_reserve = 0;
    qint64 m_latest = 0, m_fastest = 0, m_stopped = 0;
    int m_laps = 0;
signals:
    void changed();
};
