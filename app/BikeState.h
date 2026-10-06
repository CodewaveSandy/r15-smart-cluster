#pragma once

#include <QObject>
#include <QtGlobal>

// Desktop-only state foundation. Ranges are simulator bounds, not verified R15 specs.
class BikeState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int speed READ speed NOTIFY changed)
    Q_PROPERTY(int rpm READ rpm NOTIFY changed)
    Q_PROPERTY(int gear READ gear NOTIFY changed)
    Q_PROPERTY(int fuel READ fuel NOTIFY changed)
    Q_PROPERTY(bool leftIndicator READ leftIndicator NOTIFY changed)
    Q_PROPERTY(bool rightIndicator READ rightIndicator NOTIFY changed)
    Q_PROPERTY(bool highBeam READ highBeam NOTIFY changed)
    Q_PROPERTY(bool engineWarning READ engineWarning NOTIFY changed)
    Q_PROPERTY(bool quickShifter READ quickShifter NOTIFY changed)
    Q_PROPERTY(bool vva READ vva NOTIFY changed)
    Q_PROPERTY(bool phoneConnected READ phoneConnected NOTIFY changed)
    Q_PROPERTY(int phoneBattery READ phoneBattery NOTIFY changed)
    Q_PROPERTY(bool absWarning READ absWarning NOTIFY changed)
    Q_PROPERTY(bool tcsWarning READ tcsWarning NOTIFY changed)
    Q_PROPERTY(bool coolantWarning READ coolantWarning NOTIFY changed)
    Q_PROPERTY(bool messagePending READ messagePending NOTIFY changed)
    Q_PROPERTY(bool incomingCall READ incomingCall NOTIFY changed)

public:
    explicit BikeState(QObject *parent = nullptr) : QObject(parent) {}
    int speed() const { return m_speed; }
    int rpm() const { return m_rpm; }
    int gear() const { return m_gear; }
    int fuel() const { return m_fuel; }
    bool leftIndicator() const { return m_left; }
    bool rightIndicator() const { return m_right; }
    bool highBeam() const { return m_highBeam; }
    bool engineWarning() const { return m_engineWarning; }

    bool quickShifter() const { return m_qs; }
    bool vva() const { return m_rpm > 7500; }
    bool phoneConnected() const { return m_phone; }
    int phoneBattery() const { return m_phone ? 84 : -1; }
    bool absWarning() const { return m_abs; }
    bool tcsWarning() const { return m_tcs; }
    bool coolantWarning() const { return m_coolant; }
    bool messagePending() const { return m_phone && m_message; }
    bool incomingCall() const { return m_phone && m_call; }
    Q_INVOKABLE void toggleQuickShifter() { m_qs = !m_qs; emit changed(); }
    Q_INVOKABLE void togglePhone() { m_phone = !m_phone; emit changed(); }
    Q_INVOKABLE void toggleAbs() { m_abs = !m_abs; emit changed(); }
    Q_INVOKABLE void toggleTcs() { m_tcs = !m_tcs; emit changed(); }
    Q_INVOKABLE void toggleCoolant() { m_coolant = !m_coolant; emit changed(); }
    Q_INVOKABLE void toggleMessage() { m_message = !m_message; emit changed(); }
    Q_INVOKABLE void toggleCall() { m_call = !m_call; emit changed(); }

    // These explicit simulator actions never transmit commands to a motorcycle.
    Q_INVOKABLE void adjustSpeed(int delta) { m_speed = qBound(0, m_speed + delta, 180); emit changed(); }
    Q_INVOKABLE void adjustRpm(int delta) { m_rpm = qBound(0, m_rpm + delta, 13000); emit changed(); }
    Q_INVOKABLE void setGear(int gear) { m_gear = qBound(0, gear, 6); emit changed(); }
    Q_INVOKABLE void adjustFuel(int delta) { m_fuel = qBound(0, m_fuel + delta, 100); emit changed(); }
    Q_INVOKABLE void toggleLeft() { m_left = !m_left; emit changed(); }
    Q_INVOKABLE void toggleRight() { m_right = !m_right; emit changed(); }
    Q_INVOKABLE void toggleHighBeam() { m_highBeam = !m_highBeam; emit changed(); }
    Q_INVOKABLE void toggleEngineWarning() { m_engineWarning = !m_engineWarning; emit changed(); }
    Q_INVOKABLE void reset() {
        m_speed = 0; m_rpm = 0; m_gear = 0; m_fuel = 72;
        m_left = m_right = m_highBeam = m_engineWarning = false;
        m_qs = m_phone = m_abs = m_tcs = m_coolant = m_message = m_call = false;
        emit changed();
    }

signals:
    void changed();

private:
    int m_speed = 0;
    int m_rpm = 0;
    int m_gear = 0;
    int m_fuel = 72;
    bool m_left = false;
    bool m_right = false;
    bool m_highBeam = false;
    bool m_engineWarning = false;
    bool m_qs = false, m_phone = false, m_abs = false, m_tcs = false;
    bool m_coolant = false, m_message = false, m_call = false;
};
