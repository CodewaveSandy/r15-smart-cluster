#pragma once
#include <QFontDatabase>
#include <QGuiApplication>
#include <QDebug>

inline bool loadClusterFonts()
{
    QString family;
    for (const auto *weight : {"Regular", "SemiBold", "Bold"}) {
        const QString path = QString(":/fonts/BricolageGrotesque-%1.ttf").arg(weight);
        const int id = QFontDatabase::addApplicationFont(path);
        const auto families = QFontDatabase::applicationFontFamilies(id);
        if (id < 0 || families.isEmpty()) {
            qCritical() << "Cannot load bundled font:" << path;
            return false;
        }
        if (family.isEmpty()) family = families.first();
    }
    QGuiApplication::setFont(QFont(family));
    return true;
}
