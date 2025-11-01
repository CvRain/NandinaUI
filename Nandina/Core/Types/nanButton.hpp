//
// Created by cvrain on 2025/10/30.
//

#ifndef TRYNANDINA_NANENUMS_HPP
#define TRYNANDINA_NANENUMS_HPP

#include <QObject>
#include <qqmlintegration.h>
#include <QString>

#include "Theme/themeManager.hpp"

namespace Nandina {
    class ButtonPalette : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString backgroundColor MEMBER backgroundColor CONSTANT)
        Q_PROPERTY(QString foregroundColor MEMBER foregroundColor CONSTANT)
        Q_PROPERTY(QString borderColor MEMBER borderColor CONSTANT)

    public:
        explicit ButtonPalette(QObject *parent = nullptr);

        ButtonPalette(const ButtonPalette &);

        ButtonPalette& operator=(const ButtonPalette &);

        ButtonPalette* setBackgroundColor(const QString &color);

        ButtonPalette* setForegroundColor(const QString& color);

        ButtonPalette* setBorderColor(const QString& color);

        QString backgroundColor{ThemeManager::getInstance()->getColor()->base};
        QString foregroundColor{ThemeManager::getInstance()->getColor()->subtext0};
        QString borderColor{ThemeManager::getInstance()->getColor()->crust};
    };

    class NanButtonProperty : public QObject {
        Q_OBJECT
        QML_ELEMENT

    public:
        explicit NanButtonProperty(QObject *parent = nullptr);

        Q_INVOKABLE Nandina::ButtonPalette* getButtonPalette(const QString& type);
        Q_INVOKABLE QString test();
    };
}

#endif //TRYNANDINA_NANENUMS_HPP