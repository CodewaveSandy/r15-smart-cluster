#pragma once
#include <QObject>
#include <QDateTime>
#include <QSettings>
#include <QTimer>
#include <QThread>
#include <QMap>

// Display preferences only. Persistence runs off the UI thread.
class DisplaySettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool track READ track WRITE setTrack NOTIFY changed)
    Q_PROPERTY(bool light READ light WRITE setLight NOTIFY changed)
    Q_PROPERTY(bool imperial READ imperial WRITE setImperial NOTIFY changed)
    Q_PROPERTY(QDateTime now READ now NOTIFY clockChanged)
    Q_PROPERTY(QString persistenceError READ persistenceError NOTIFY persistenceErrorChanged)
public:
    explicit DisplaySettings(QObject *parent = nullptr);
    ~DisplaySettings() override;
    bool track() const { return m_track; }
    bool light() const { return m_light; }
    bool imperial() const { return m_imperial; }
    QDateTime now() const;
    QString persistenceError() const { return m_persistenceError; }
    void setTrack(bool value);
    void setLight(bool value);
    void setImperial(bool value);
    Q_INVOKABLE bool setClock(int year, int month, int day, int hour, int minute);
    Q_INVOKABLE void useSystemClock();
signals:
    void changed();
    void clockChanged();
    void persistenceErrorChanged();
    void persistenceFinished();
private:
    bool readBoolean(const QString &key);
    void persist(const QString &key, const QVariant &value);
    void flushPending();
    QSettings m_store;
    QTimer m_timer;
    QThread m_storageThread;
    QObject *m_storageWorker; // Owned/deleted on the storage thread.
    bool m_track = false, m_light = false, m_imperial = false;
    qint64 m_offset = 0;
    QString m_persistenceError;
    QMap<QString, QVariant> m_pending;
    bool m_writeInFlight = false;
};
