#pragma once
#include <QObject>
#include <QString>

class AppDiagnostics : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString uiError READ uiError NOTIFY uiErrorChanged)
public:
    using QObject::QObject;
    QString uiError() const { return m_uiError; }
    Q_INVOKABLE void reportUiError(const QString &details);
signals:
    void uiErrorChanged();
private:
    QString m_uiError;
};
