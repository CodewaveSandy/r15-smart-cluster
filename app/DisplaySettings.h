#pragma once
#include <QObject>
#include <QDateTime>
#include <QSettings>
#include <QTimer>

// Display preferences only: never changes the Mac clock or sends ECU commands.
class DisplaySettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool track READ track WRITE setTrack NOTIFY changed)
    Q_PROPERTY(bool light READ light WRITE setLight NOTIFY changed)
    Q_PROPERTY(bool imperial READ imperial WRITE setImperial NOTIFY changed)
    Q_PROPERTY(QDateTime now READ now NOTIFY clockChanged)
public:
    explicit DisplaySettings(QObject *parent = nullptr) : QObject(parent) {
        m_track = m_store.value("display/track", false).toBool();
        m_light = m_store.value("display/light", false).toBool();
        m_imperial = m_store.value("display/imperial", false).toBool();
        m_offset = m_store.value("display/clockOffset", 0).toLongLong();
        connect(&m_timer, &QTimer::timeout, this, &DisplaySettings::clockChanged);
        m_timer.start(1000);
    }
    bool track() const { return m_track; }
    bool light() const { return m_light; }
    bool imperial() const { return m_imperial; }
    QDateTime now() const { return QDateTime::currentDateTime().addMSecs(m_offset); }
    void setTrack(bool value) { m_track = value; m_store.setValue("display/track", value); emit changed(); }
    void setLight(bool value) { m_light = value; m_store.setValue("display/light", value); emit changed(); }
    void setImperial(bool value) { m_imperial = value; m_store.setValue("display/imperial", value); emit changed(); }
    Q_INVOKABLE bool setClock(int year, int month, int day, int hour, int minute) {
        const QDate date(year, month, day);
        const QTime time(hour, minute);
        if (year < 2020 || year > 2099 || !date.isValid() || !time.isValid()) return false;
        const QDateTime target(date, time);
        m_offset = QDateTime::currentDateTime().msecsTo(target);
        m_store.setValue("display/clockOffset", m_offset);
        emit clockChanged();
        return true;
    }
    Q_INVOKABLE void useSystemClock() {
        m_offset = 0; m_store.setValue("display/clockOffset", 0); emit clockChanged();
    }
private:
    QSettings m_store;
    QTimer m_timer;
    bool m_track = false, m_light = false, m_imperial = false;
    qint64 m_offset = 0;
signals:
    void changed();
    void clockChanged();
};
