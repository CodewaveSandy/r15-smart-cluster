#pragma once
#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include "BikeSnapshot.h"

// Read-only presentation contract. Only adapters publish observations.
class BikeState : public QObject {
    Q_OBJECT
    Q_PROPERTY(int speedKph READ speedKph NOTIFY instrumentsChanged)
    Q_PROPERTY(int engineRpm READ engineRpm NOTIFY instrumentsChanged)
    Q_PROPERTY(int gear READ gear NOTIFY instrumentsChanged)
    Q_PROPERTY(int fuelPercent READ fuelPercent NOTIFY instrumentsChanged)
    Q_PROPERTY(bool leftIndicator READ leftIndicator NOTIFY tellTalesChanged)
    Q_PROPERTY(bool rightIndicator READ rightIndicator NOTIFY tellTalesChanged)
    Q_PROPERTY(bool hazards READ hazards NOTIFY tellTalesChanged)
    Q_PROPERTY(bool highBeam READ highBeam NOTIFY tellTalesChanged)
    Q_PROPERTY(bool engineWarning READ engineWarning NOTIFY tellTalesChanged)
    Q_PROPERTY(bool quickShifter READ quickShifter NOTIFY tellTalesChanged)
    Q_PROPERTY(bool vva READ vva NOTIFY instrumentsChanged)
    Q_PROPERTY(bool phoneConnected READ phoneConnected NOTIFY phoneChanged)
    Q_PROPERTY(int phoneBatteryPercent READ phoneBatteryPercent NOTIFY phoneChanged)
    Q_PROPERTY(bool absWarning READ absWarning NOTIFY tellTalesChanged)
    Q_PROPERTY(bool tcsWarning READ tcsWarning NOTIFY tellTalesChanged)
    Q_PROPERTY(bool coolantWarning READ coolantWarning NOTIFY tellTalesChanged)
    Q_PROPERTY(bool messagePending READ messagePending NOTIFY phoneChanged)
    Q_PROPERTY(bool incomingCall READ incomingCall NOTIFY phoneChanged)

    Q_PROPERTY(double batteryVoltageV READ batteryVoltageV NOTIFY instrumentsChanged)
    Q_PROPERTY(double coolantTemperatureC READ coolantTemperatureC NOTIFY instrumentsChanged)
    Q_PROPERTY(bool batteryVoltageAvailable READ batteryVoltageAvailable NOTIFY changed)
    Q_PROPERTY(bool coolantTemperatureAvailable READ coolantTemperatureAvailable NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY availabilityChanged)
    Q_PROPERTY(QString dataStatus READ dataStatus NOTIFY availabilityChanged)
public:
    enum class Availability { Disconnected, Available, Invalid, Stale };
    Q_ENUM(Availability)
    explicit BikeState(QObject *parent = nullptr, int staleTimeoutMs = 3000);
    int speedKph() const { return m_snapshot.speedKph; }
    int engineRpm() const { return m_snapshot.engineRpm; }
    int gear() const { return m_snapshot.gear; }
    int fuelPercent() const { return m_snapshot.fuelPercent; }
    bool hazards() const { return m_snapshot.hazards; }
    bool highBeam() const { return m_snapshot.highBeam; }
    bool engineWarning() const { return m_snapshot.engineWarning; }
    bool quickShifter() const { return m_snapshot.quickShifter; }
    bool phoneConnected() const { return m_snapshot.phoneConnected; }
    bool absWarning() const { return m_snapshot.absWarning; }
    bool tcsWarning() const { return m_snapshot.tcsWarning; }
    bool coolantWarning() const { return m_snapshot.coolantWarning; }
    bool leftIndicator() const { return m_snapshot.hazards || m_snapshot.leftIndicator; }
    bool rightIndicator() const { return m_snapshot.hazards || m_snapshot.rightIndicator; }
    bool vva() const { return m_snapshot.engineRpm > 7500; }
    int phoneBatteryPercent() const { return m_snapshot.phoneConnected ? m_snapshot.phoneBatteryPercent : -1; }
    bool messagePending() const { return m_snapshot.phoneConnected && m_snapshot.messagePending; }
    bool incomingCall() const { return m_snapshot.phoneConnected && m_snapshot.incomingCall; }
    double batteryVoltageV() const { return m_snapshot.batteryVoltageV; }
    double coolantTemperatureC() const { return m_snapshot.coolantTemperatureC; }
    bool batteryVoltageAvailable() const { return available() && m_snapshot.batteryVoltageAvailable; }
    bool coolantTemperatureAvailable() const { return available() && m_snapshot.coolantTemperatureAvailable; }
    bool available() const { return m_availability == Availability::Available; }
    QString dataStatus() const;
    Availability availability() const { return m_availability; }
    bool applySnapshot(const BikeSnapshot &snapshot);
    void markDisconnected(const QString &reason);
signals:
    void instrumentsChanged();
    void tellTalesChanged();
    void phoneChanged();
    void availabilityChanged();
    void changed(); // Aggregate for ride integration and compatibility, only on actual changes.
private:
    void setAvailability(Availability state, const QString &reason);
    BikeSnapshot m_snapshot;
    Availability m_availability = Availability::Disconnected;
    QTimer m_watchdog;
    QElapsedTimer m_lastUpdate;
    int m_staleTimeoutMs;
};
