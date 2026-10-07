#include "DiagnosticLog.h"
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <cstdio>

namespace {
std::mutex handlerMutex;
DiagnosticLog *currentLog = nullptr; // Qt's process-global message hook requires one sink.
constexpr std::size_t kQueueCapacity = 512;
constexpr int kMaximumLineBytes = 4096;
}
DiagnosticLog::DiagnosticLog(const QString &directory, qint64 maximumFileBytes)
    : m_filePath(QDir(directory).filePath("cluster.log")), m_maximumFileBytes(qMax<qint64>(1024, maximumFileBytes)) {
    if (!QDir().mkpath(directory)) std::fprintf(stderr, "cluster.log: cannot create log directory; stderr remains available\n");
    m_worker = std::thread(&DiagnosticLog::run, this);
    qSetMessagePattern("[%{time yyyy-MM-ddTHH:mm:ss.zzz}] [%{type}] [%{category}] [%{file}:%{line}] %{message}");
    std::lock_guard<std::mutex> lock(handlerMutex);
    currentLog = this;
    m_previous = qInstallMessageHandler(&DiagnosticLog::messageHandler);
}
DiagnosticLog::~DiagnosticLog() {
    {
        std::lock_guard<std::mutex> lock(handlerMutex);
        qInstallMessageHandler(m_previous); currentLog = nullptr;
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex); m_stopping = true;
    }
    m_ready.notify_one();
    if (m_worker.joinable()) m_worker.join();
}
void DiagnosticLog::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message) {
    const QByteArray line = qFormatLogMessage(type, context, message.left(kMaximumLineBytes)).toUtf8().left(kMaximumLineBytes) + '\n';
    std::fwrite(line.constData(), 1, static_cast<std::size_t>(line.size()), stderr);
    std::fflush(stderr); // Fatal messages remain visible even if the process terminates before disk flush.
    std::lock_guard<std::mutex> lock(handlerMutex);
    if (currentLog) currentLog->enqueue(line);
}
void DiagnosticLog::enqueue(QByteArray line) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queue.size() == kQueueCapacity) { ++m_dropped; return; }
    try { m_queue.push_back(std::move(line)); }
    catch (...) { ++m_dropped; std::fprintf(stderr, "cluster.log: queue allocation failed\n"); return; }
    m_ready.notify_one();
}
void DiagnosticLog::run() {
    QFile file(m_filePath);
    bool diskAvailable = file.open(QIODevice::WriteOnly | QIODevice::Append);
    if (!diskAvailable) std::fprintf(stderr, "cluster.log: disk logging unavailable; stderr remains available\n");
    for (;;) {
        QByteArray line;
        quint64 dropped = 0;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_ready.wait(lock, [this] { return m_stopping || !m_queue.empty(); });
            if (m_queue.empty() && m_stopping) break;
            line = std::move(m_queue.front()); m_queue.pop_front();
            dropped = m_dropped; m_dropped = 0;
        }
        if (dropped) line.prepend("[cluster.log] Dropped " + QByteArray::number(dropped) + " messages due to queue limit\n");
        if (!diskAvailable) continue;
        if (file.size() + line.size() > m_maximumFileBytes) {
            file.close();
            const QString first = m_filePath + ".1", second = m_filePath + ".2";
            bool rotated = (!QFile::exists(second) || QFile::remove(second));
            if (rotated && QFile::exists(first)) rotated = QFile::rename(first, second);
            if (rotated && QFile::exists(m_filePath)) rotated = QFile::rename(m_filePath, first);
            diskAvailable = rotated && file.open(QIODevice::WriteOnly | QIODevice::Append);
            if (!diskAvailable) { std::fprintf(stderr, "cluster.log: rotation failed; stderr remains available\n"); continue; }
        }
        if (file.write(line) != line.size() || !file.flush()) {
            diskAvailable = false; file.close();
            std::fprintf(stderr, "cluster.log: write failed; stderr remains available\n");
        }
    }
}
