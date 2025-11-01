#include "nanButton.hpp"

#include <Theme/themeManager.hpp>
#include <QMetaProperty>

namespace Nandina {
    ButtonPalette::ButtonPalette(QObject *parent)
        : QObject(parent) {
    }

    ButtonPalette::ButtonPalette(const ButtonPalette &sourcePalette) {
        this->backgroundColor = sourcePalette.backgroundColor;
        this->foregroundColor = sourcePalette.foregroundColor;
        this->borderColor = sourcePalette.borderColor;
    }

    ButtonPalette& ButtonPalette::operator=(const ButtonPalette &) {
        return *this;
    }

    ButtonPalette* ButtonPalette::setBackgroundColor(const QString &color) {
        this->backgroundColor = color;
        return this;
    }

    ButtonPalette* ButtonPalette::setForegroundColor(const QString& color) {
        this->foregroundColor = color;
        return this;
    }

    ButtonPalette* ButtonPalette::setBorderColor(const QString& color) {
        this->borderColor = color;
        return this;
    }

    NanButtonProperty::NanButtonProperty(QObject *parent) : QObject(parent) {
    }

    namespace {
        QString resolveColor(const QString& colorName) {
            if (colorName.startsWith("#") || colorName == "transparent") {
                return colorName;
            }

            auto* baseColors = ThemeManager::getInstance()->getColor();
            if (!baseColors) return {};

            const auto* metaObject = baseColors->metaObject();
            for (int i = metaObject->propertyOffset(); i < metaObject->propertyCount(); ++i) {
                QMetaProperty property = metaObject->property(i);
                if (strcmp(property.name(), colorName.toStdString().c_str()) == 0) {
                    return property.read(baseColors).toString();
                }
            }
            return colorName; // Fallback
        }
    } 

    ButtonPalette* NanButtonProperty::getButtonPalette(const QString& type) {
        auto* palette = new ButtonPalette();
        auto* themeManager = ThemeManager::getInstance();

        QString bgPath = QString("NanButton.colors.%1.backgroundColor").arg(type);
        QString fgPath = QString("NanButton.colors.%1.foregroundColor").arg(type);
        QString borderPath = QString("NanButton.colors.%1.borderColor").arg(type);

        QString bgColorName = themeManager->getComponentStyle(bgPath).toString();
        QString fgColorName = themeManager->getComponentStyle(fgPath).toString();
        QString borderColorName = themeManager->getComponentStyle(borderPath).toString();

        palette->setBackgroundColor(resolveColor(bgColorName))
               ->setForegroundColor(resolveColor(fgColorName))
               ->setBorderColor(resolveColor(borderColorName));

        return palette;
    }

    QString NanButtonProperty::test() {
        return "Hello world!";
    }
}

