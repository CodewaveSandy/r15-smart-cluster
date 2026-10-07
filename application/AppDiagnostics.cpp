#include "AppDiagnostics.h"
#include <QLoggingCategory>
Q_LOGGING_CATEGORY(uiLog, "cluster.ui")
void AppDiagnostics::reportUiError(const QString &details) {
    qCWarning(uiLog).noquote() << details.left(2048);
    if (!m_uiError.isEmpty()) return; // Latch without triggering repeated binding/layout updates.
    m_uiError = QStringLiteral("UI error — inspect diagnostics");
    emit uiErrorChanged();
}
