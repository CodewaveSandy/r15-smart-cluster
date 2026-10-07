#pragma once
#include <QString>
#include <QtLogging>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

// Process Qt handler: bounded queue; rotating disk writes happen on its worker.
// Construct before application services; destroy after all their worker threads.
class DiagnosticLog {
public:
    explicit DiagnosticLog(const QString &directory, qint64 maximumFileBytes = 512 * 1024);
    ~DiagnosticLog();
    DiagnosticLog(const DiagnosticLog &) = delete;
    DiagnosticLog &operator=(const DiagnosticLog &) = delete;
    QString filePath() const { return m_filePath; }
private:
    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message);
    void enqueue(QByteArray line);
    void run();
    QString m_filePath;
    qint64 m_maximumFileBytes;
    QtMessageHandler m_previous = nullptr;
    std::mutex m_mutex;
    std::condition_variable m_ready;
    std::deque<QByteArray> m_queue;
    quint64 m_dropped = 0;
    bool m_stopping = false;
    std::thread m_worker;
};
