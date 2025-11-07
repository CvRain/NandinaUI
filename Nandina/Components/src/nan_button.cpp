//
// Created by cvrain on 2025/11/4.
//

#include "nan_button.hpp"

namespace Nandina::Components {
    NanButtonStyle::NanButtonStyle(QObject *parent)
        : BaseComponent(parent) {
        connect(ThemeManager::getInstance(), &Nandina::ThemeManager::paletteChanged,
                this, &NanButtonStyle::updateColor);
    }

    NanButtonStyle::NanButtonStyle(NanButtonStyle const &style) {
        this->backgroundColor = style.backgroundColor;
        this->borderColor = style.borderColor;
        this->foregroundColor = style.foregroundColor;
        this->styleName = style.styleName;
    }

    NanButtonStyle& NanButtonStyle::operator=(NanButtonStyle const &style) {
        if (this != &style) {
            this->backgroundColor = style.backgroundColor;
            this->borderColor = style.borderColor;
            this->foregroundColor = style.foregroundColor;
            this->styleName = style.styleName;
        }
        return *this;
    }

    QString NanButtonStyle::getStyleName() const {
        return this->styleName;
    }

    QString NanButtonStyle::getBackgroundColor() const {
        return this->backgroundColor;
    }

    QString NanButtonStyle::getBorderColor() const {
        return this->borderColor;
    }

    QString NanButtonStyle::getForegroundColor() const {
        return this->foregroundColor;
    }

    NanButtonStyle& NanButtonStyle::setStyleName(const QString &s) {
        styleName = s;
        return *this;
    }

    NanButtonStyle& NanButtonStyle::setBackgroundColor(const QString &s) {
        backgroundColor = s;
        return *this;
    }

    NanButtonStyle& NanButtonStyle::setBorderColor(const QString &s) {
        borderColor = s;
        return *this;
    }

    NanButtonStyle& NanButtonStyle::setForegroundColor(const QString &s) {
        foregroundColor = s;
        return *this;
    }

    void NanButtonStyle::updateColor() {
    }
}
