//
// Created by cvrain on 2025/11/3.
//

#include "button_style.hpp"

namespace Nandina::Theme::Components {
    NanButtonStyle::NanButtonStyle(QObject *parent)
        : ComponentStyle(parent) {
    }

    NanButtonStyle::NanButtonStyle(const NanButtonStyle &other) {
        this->background = other.background;
        this->foreground = other.foreground;
        this->border = other.border;
    }

    NanButtonStyle& NanButtonStyle::operator=(const NanButtonStyle &other) {
        if (this != &other) {
            this->background = other.background;
            this->foreground = other.foreground;
            this->border = other.border;
        }
        return *this;
    }

    bool NanButtonStyle::loadFromJson(const QJsonObject &json) {
        if (json.isEmpty()) {
            return false;
        }

        // 读取样式属性
        if (json.contains("background")) {
            background = json["background"].toString();
        }
        if (json.contains("foreground")) {
            foreground = json["foreground"].toString();
        }
        if (json.contains("border")) {
            border = json["border"].toString();
        }

        return isValid();
    }

    bool NanButtonStyle::isValid() const {
        // 检查必需的属性是否都存在且不为空
        return !background.isEmpty() && !foreground.isEmpty() && !border.isEmpty();
    }

    QString NanButtonStyle::getBackground() const {
        return this->background;
    }

    QString NanButtonStyle::getForeground() const {
        return this->foreground;
    }

    QString NanButtonStyle::getBorder() const {
        return this->border;
    }
}
