//
// Created by Gemini on 2025/11/1.
//

#ifndef TRYNANDINA_NANBUTTONTYPES_HPP
#define TRYNANDINA_NANBUTTONTYPES_HPP

#include <QObject>
#include <qqmlintegration.h>

namespace Nandina::Core::Types {
    class NanButtonTypes : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

        Q_PROPERTY(QString type_default MEMBER type_default CONSTANT)
        Q_PROPERTY(QString filledPrimary MEMBER filledPrimary CONSTANT)
        Q_PROPERTY(QString filledSecondary MEMBER filledSecondary CONSTANT)
        Q_PROPERTY(QString filledTertiary MEMBER filledTertiary CONSTANT)
        Q_PROPERTY(QString filledSuccess MEMBER filledSuccess CONSTANT)
        Q_PROPERTY(QString filledWarning MEMBER filledWarning CONSTANT)
        Q_PROPERTY(QString filledError MEMBER filledError CONSTANT)
        Q_PROPERTY(QString filledSurface MEMBER filledSurface CONSTANT)
        Q_PROPERTY(QString tonalPrimary MEMBER tonalPrimary CONSTANT)
        Q_PROPERTY(QString tonalSecondary MEMBER tonalSecondary CONSTANT)
        Q_PROPERTY(QString tonalTertiary MEMBER tonalTertiary CONSTANT)
        Q_PROPERTY(QString tonalSuccess MEMBER tonalSuccess CONSTANT)
        Q_PROPERTY(QString tonalWarning MEMBER tonalWarning CONSTANT)
        Q_PROPERTY(QString tonalError MEMBER tonalError CONSTANT)
        Q_PROPERTY(QString tonalSurface MEMBER tonalSurface CONSTANT)
        Q_PROPERTY(QString outlinedPrimary MEMBER outlinedPrimary CONSTANT)
        Q_PROPERTY(QString outlinedSecondary MEMBER outlinedSecondary CONSTANT)
        Q_PROPERTY(QString outlinedTertiary MEMBER outlinedTertiary CONSTANT)
        Q_PROPERTY(QString outlinedSuccess MEMBER outlinedSuccess CONSTANT)
        Q_PROPERTY(QString outlinedWarning MEMBER outlinedWarning CONSTANT)
        Q_PROPERTY(QString outlinedError MEMBER outlinedError CONSTANT)
        Q_PROPERTY(QString outlinedSurface MEMBER outlinedSurface CONSTANT)

    public:
        explicit NanButtonTypes(QObject *parent = nullptr) : QObject(parent) {}

        const QString type_default = "default";
        const QString filledPrimary = "filledPrimary";
        const QString filledSecondary = "filledSecondary";
        const QString filledTertiary = "filledTertiary";
        const QString filledSuccess = "filledSuccess";
        const QString filledWarning = "filledWarning";
        const QString filledError = "filledError";
        const QString filledSurface = "filledSurface";
        const QString tonalPrimary = "tonalPrimary";
        const QString tonalSecondary = "tonalSecondary";
        const QString tonalTertiary = "tonalTertiary";
        const QString tonalSuccess = "tonalSuccess";
        const QString tonalWarning = "tonalWarning";
        const QString tonalError = "tonalError";
        const QString tonalSurface = "tonalSurface";
        const QString outlinedPrimary = "outlinedPrimary";
        const QString outlinedSecondary = "outlinedSecondary";
        const QString outlinedTertiary = "outlinedTertiary";
        const QString outlinedSuccess = "outlinedSuccess";
        const QString outlinedWarning = "outlinedWarning";
        const QString outlinedError = "outlinedError";
        const QString outlinedSurface = "outlinedSurface";
    };
}

#endif //TRYNANDINA_NANBUTTONTYPES_HPP
