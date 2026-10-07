#pragma once
#include "data/IBikeDataSource.h"
#include <QTimer>

class SimulatorBikeDataSource final : public IBikeDataSource {
    Q_OBJECT
public:
    static constexpr double kDemoOdometerKm = 12480.0;
    explicit SimulatorBikeDataSource(QObject *parent = nullptr);
    void start() override;
    void stop() override;
    Q_INVOKABLE void adjustSpeed(int delta);
    Q_INVOKABLE void adjustRpm(int delta);
    Q_INVOKABLE void adjustFuel(int delta);
    Q_INVOKABLE void setGear(int gear);
    Q_INVOKABLE void toggleLeft();
    Q_INVOKABLE void toggleRight();
    Q_INVOKABLE void toggleHazards();
    Q_INVOKABLE void togglePhone();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void toggleHighBeam();
    Q_INVOKABLE void toggleEngineWarning();
    Q_INVOKABLE void toggleQuickShifter();
    Q_INVOKABLE void toggleAbs();
    Q_INVOKABLE void toggleTcs();
    Q_INVOKABLE void toggleCoolant();
    Q_INVOKABLE void toggleMessage();
    Q_INVOKABLE void toggleCall();
private:
    void publish();
    BikeSnapshot m_snapshot;
    QTimer m_heartbeat;
    bool m_running = false;
};
