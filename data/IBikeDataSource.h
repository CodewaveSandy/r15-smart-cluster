#pragma once
#include <QObject>
#include "domain/BikeSnapshot.h"

// A real, used boundary: simulator now; validated transport adapters later.
class IBikeDataSource : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual void start() = 0;
    virtual void stop() = 0;
signals:
    void snapshotReady(const BikeSnapshot &snapshot);
    void disconnected(const QString &reason);
};
