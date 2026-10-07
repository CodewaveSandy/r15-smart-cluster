#include "DisplaySettings.h"
#include <QLoggingCategory>
Q_LOGGING_CATEGORY(settingsLog, "cluster.settings")
namespace { constexpr qint64 kMaximumClockOffsetMs = 100LL * 366 * 24 * 60 * 60 * 1000; }
DisplaySettings::DisplaySettings(QObject *parent) : QObject(parent), m_storageWorker(new QObject) {
    m_track = readBoolean(QStringLiteral("display/track"));
    m_light = readBoolean(QStringLiteral("display/light"));
    m_imperial = readBoolean(QStringLiteral("display/imperial"));
    bool valid = false;
    const auto raw = m_store.value("display/clockOffset", 0);
    const QString offsetText = raw.toString();
    const qint64 offset = offsetText.size() <= 20 ? offsetText.toLongLong(&valid) : 0;
    if (valid && offset >= -kMaximumClockOffsetMs && offset <= kMaximumClockOffsetMs) m_offset = offset;
    else {
        m_persistenceError = QStringLiteral("Invalid saved preferences; defaults are active");
        qCWarning(settingsLog) << "Invalid stored clock offset; using system clock";
    }
    if (m_store.status() != QSettings::NoError) {
        m_persistenceError = QStringLiteral("Preferences could not be read; defaults are active");
        qCWarning(settingsLog) << m_persistenceError;
    }
    m_storageWorker->moveToThread(&m_storageThread);
    connect(&m_storageThread, &QThread::finished, m_storageWorker, &QObject::deleteLater);
    m_storageThread.start();
    connect(&m_timer, &QTimer::timeout, this, &DisplaySettings::clockChanged);
    m_timer.start(1000);
}
DisplaySettings::~DisplaySettings() {
    if (!m_storageThread.isRunning()) { delete m_storageWorker; return; }
    // FIFO drain plus coalesced pending preferences during shutdown only.
    const auto pending = m_pending;
    const auto file = m_store.fileName(); const auto format = m_store.format();
    QMetaObject::invokeMethod(m_storageWorker, [pending, file, format] {
        if (pending.isEmpty()) return;
        QSettings store(file, format);
        for (auto it = pending.cbegin(); it != pending.cend(); ++it) store.setValue(it.key(), it.value());
        store.sync();
        if (store.status() != QSettings::NoError) qCWarning(settingsLog) << "Final preference flush failed";
    }, Qt::BlockingQueuedConnection);
    m_storageThread.quit(); m_storageThread.wait();
}
bool DisplaySettings::readBoolean(const QString &key) {
    const QString value = m_store.value(key, false).toString().toLower();
    if (value == "true" || value == "1") return true;
    if (value == "false" || value == "0") return false;
    m_persistenceError = QStringLiteral("Invalid saved preferences; defaults are active");
    qCWarning(settingsLog) << "Invalid stored boolean" << key << "; using false";
    return false;
}
QDateTime DisplaySettings::now() const { return QDateTime::currentDateTime().addMSecs(m_offset); }
void DisplaySettings::persist(const QString &key, const QVariant &value) {
    // Four known keys; latest value wins while disk is busy. No unbounded write queue.
    m_pending.insert(key, value);
    flushPending();
}
void DisplaySettings::flushPending() {
    if (m_writeInFlight || m_pending.isEmpty()) return;
    if (!m_storageThread.isRunning()) {
        m_persistenceError = QStringLiteral("Preference storage unavailable; changes are temporary");
        qCWarning(settingsLog) << m_persistenceError;
        emit persistenceErrorChanged(); return;
    }
    const QString file = m_store.fileName();
    const QSettings::Format format = m_store.format();
    const auto batch = m_pending; m_pending.clear(); m_writeInFlight = true;
    QMetaObject::invokeMethod(m_storageWorker, [this, file, format, batch] {
        QSettings store(file, format);
        for (auto it = batch.cbegin(); it != batch.cend(); ++it) store.setValue(it.key(), it.value());
        store.sync();
        const bool failed = store.status() != QSettings::NoError;
        if (failed) qCWarning(settingsLog) << "Preference batch write failed; status" << store.status();
        QMetaObject::invokeMethod(this, [this, failed] {
            m_writeInFlight = false;
            const QString error = failed ? QStringLiteral("Preferences could not be saved; changes are temporary") : QString{};
            if (m_persistenceError != error) { m_persistenceError = error; emit persistenceErrorChanged(); }
            emit persistenceFinished();
            flushPending();
        }, Qt::QueuedConnection);
    }, Qt::QueuedConnection);
}
void DisplaySettings::setTrack(bool value) { if (value == m_track) return; m_track = value; persist("display/track", value); emit changed(); }
void DisplaySettings::setLight(bool value) { if (value == m_light) return; m_light = value; persist("display/light", value); emit changed(); }
void DisplaySettings::setImperial(bool value) { if (value == m_imperial) return; m_imperial = value; persist("display/imperial", value); emit changed(); }
bool DisplaySettings::setClock(int year, int month, int day, int hour, int minute) {
    const QDate date(year, month, day); const QTime time(hour, minute);
    if (year < 2020 || year > 2099 || !date.isValid() || !time.isValid()) {
        qCWarning(settingsLog) << "Rejected invalid clock edit";
        return false;
    }
    m_offset = QDateTime::currentDateTime().msecsTo(QDateTime(date, time));
    persist("display/clockOffset", m_offset); emit clockChanged(); return true;
}
void DisplaySettings::useSystemClock() { m_offset = 0; persist("display/clockOffset", 0); emit clockChanged(); }
