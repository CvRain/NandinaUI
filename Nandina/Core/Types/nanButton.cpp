#include "nanButton.hpp"

#include <Theme/themeManager.hpp>

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

    ButtonPalette* NanButtonProperty::getButtonPalette(const Type type) {
        auto* palette = new ButtonPalette();
        switch (type) {
            case Type::Default:
                palette->setBackgroundColor("#7186ff")
                        ->setForegroundColor(ThemeManager::getInstance()->getColor()->base)
                        ->setBorderColor(ThemeManager::getInstance()->getColor()->text);
                break;
            case Type::FilledPrimary:
                palette->setBackgroundColor("#e977ca")
                        ->setForegroundColor(ThemeManager::getInstance()->getColor()->base)
                        ->setBorderColor(ThemeManager::getInstance()->getColor()->lavender);
                break;
            case Type::FilledSecondary:
                palette->setBackgroundColor("#0f9299")
                        ->setForegroundColor(ThemeManager::getInstance()->getColor()->base)
                        ->setBorderColor(ThemeManager::getInstance()->getColor()->flamingo);
                break;
            case Type::FilledTertiary:
                palette->setBackgroundColor("#3ea028")
                        ->setForegroundColor(ThemeManager::getInstance()->getColor()->base)
                        ->setBorderColor(ThemeManager::getInstance()->getColor()->sapphire);
                break;
            case Type::FilledSuccess:
                palette->setBackgroundColor("#df8e1c")
                    ->setForegroundColor(ThemeManager::getInstance()->getColor()->base)
                    ->setBorderColor(ThemeManager::getInstance()->getColor()->green);
                break;
            case Type::FilledWarning:
                palette->setBackgroundColor("#d40338")
                    ->setForegroundColor(ThemeManager::getInstance()->getColor()->base)
                    ->setBorderColor(ThemeManager::getInstance()->getColor()->yellow);
                break;
            case Type::FilledError:
                palette->setBackgroundColor("#8c8fa3")
                    ->setForegroundColor(ThemeManager::getInstance()->getColor()->base)
                    ->setBorderColor(ThemeManager::getInstance()->getColor()->red);
                break;
            case Type::FilledSurface:
                palette->setBackgroundColor("#e9fbff")
                    ->setForegroundColor(ThemeManager::getInstance()->getColor()->base)
                    ->setBorderColor(ThemeManager::getInstance()->getColor()->surface2);
                break;
            case Type::TonalPrimary:
                palette->setBackgroundColor("#f7c1e8")
                        ->setForegroundColor(ThemeManager::getInstance()->getColor()->lavender)
                        ->setBorderColor(ThemeManager::getInstance()->getColor()->lavender);
                break;
            case Type::TonalSecondary:
                    palette->setBackgroundColor("#93e2d5")
                        ->setForegroundColor(ThemeManager::getInstance()->getColor()->teal)
                        ->setBorderColor("#93e2d5");
                break;
            case Type::TonalTertiary:
            palette->setBackgroundColor("#a6e3a1")
                        ->setForegroundColor(ThemeManager::getInstance()->getColor()->green)
                        ->setBorderColor("#a6e3a1");
                break;
            case Type::TonalSurface:
            palette->setBackgroundColor("#f9e2af")
                        ->setForegroundColor(ThemeManager::getInstance()->getColor()->text)
                        ->setBorderColor("#f9e2af");
                break;
            case Type::OutlinedPrimary:
            palette->setBackgroundColor("transparent")
                        ->setForegroundColor(ThemeManager::getInstance()->getColor()->text)
                        ->setBorderColor("#f38ba8");
            
                break;
            case Type::OutlinedSecondary:
            palette->setBackgroundColor("transparent")
                        ->setForegroundColor(ThemeManager::getInstance()->getColor()->text)
                        ->setBorderColor("#f38ba8");
                break;
            case Type::OutlinedTertiary:
            palette->setBackgroundColor("transparent")
                        ->setForegroundColor(ThemeManager::getInstance()->getColor()->text)
                        ->setBorderColor("#dcdfe6");
            
                break;
            case Type::OutlinedSurface:
            palette->setBackgroundColor("transparent")
                    ->setForegroundColor(ThemeManager::getInstance()->getColor()->text)
                    ->setBorderColor("#8c8fa3");
                break;
        }
        return palette;
    }

    QString NanButtonProperty::test() {
        return "Hello world!";
    }
}
