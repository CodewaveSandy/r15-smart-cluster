#pragma once
#include <QObject>
#include <QElapsedTimer>
#include <QTimer>
#include <QPointer>
#include "BikeState.h"

// Session-only demo odometer/trips; never vehicle persistence.
class RideComputer : public QObject {
    Q_OBJECT
    Q_PROPERTY(double odometerKm READ odometerKm NOTIFY changed)
    Q_PROPERTY(double tripAKm READ tripAKm NOTIFY changed)
    Q_PROPERTY(double tripBKm READ tripBKm NOTIFY changed)
    Q_PROPERTY(double reserveTripKm READ reserveTripKm NOTIFY changed)
    Q_PROPERTY(bool lapRunning READ lapRunning NOTIFY changed)
    Q_PROPERTY(int lapNumber READ lapNumber NOTIFY changed)
    Q_PROPERTY(qint64 currentLapMs READ currentLapMs NOTIFY changed)
    Q_PROPERTY(qint64 latestLapMs READ latestLapMs NOTIFY changed)
    Q_PROPERTY(qint64 fastestLapMs READ fastestLapMs NOTIFY changed)
public:
    explicit RideComputer(BikeState &bike, double initialOdometerKm = 0, QObject *parent = nullptr);
    double odometerKm() const { return m_initialOdometerKm + m_distance; }
    double tripAKm() const { return m_tripA; }
    double tripBKm() const { return m_tripB; }
    double reserveTripKm() const { return m_reserve; }
    bool lapRunning() const { return m_lap.isValid(); }
    int lapNumber() const { return m_laps + (lapRunning() || m_stopped > 0 ? 1 : 0); }
    qint64 currentLapMs() const { return lapRunning() ? m_lap.elapsed() : m_stopped; }
    qint64 latestLapMs() const { return m_latest; }
    qint64 fastestLapMs() const { return m_fastest; }
    void advance(double seconds);
    Q_INVOKABLE void resetTrip(int trip);
    Q_INVOKABLE void lapAction();
    Q_INVOKABLE void stopLap();
    Q_INVOKABLE void resetLaps();
private:
    QPointer<BikeState> m_bike;
    QTimer m_timer;
    QElapsedTimer m_tick, m_lap;
    double m_initialOdometerKm = 0;
    double m_distance = 0, m_tripA = 0, m_tripB = 0, m_reserve = 0;
    qint64 m_latest = 0, m_fastest = 0, m_stopped = 0;
    int m_laps = 0;
signals:
    void changed();
};
