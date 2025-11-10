//
// Created by cvrain on 2025/11/4.
//

#include "nan_button.hpp"

namespace Nandina::Components {
    NanButtonStyle::NanButtonStyle(QObject *parent) : BaseComponent(parent) {
        connect(ThemeManager::getInstance(), &Nandina::ThemeManager::paletteChanged, this,
                &NanButtonStyle::updateColor);
    }

    NanButtonStyle::NanButtonStyle(NanButtonStyle const &style) : BaseComponent(nullptr) {
        this->backgroundColor = style.backgroundColor;
        this->borderColor = style.borderColor;
        this->foregroundColor = style.foregroundColor;
        this->styleName = style.styleName;
        // 复制颜色引用
        this->backgroundColorRef = style.backgroundColorRef;
        this->borderColorRef = style.borderColorRef;
        this->foregroundColorRef = style.foregroundColorRef;
        // 连接主题改变信号
        connect(ThemeManager::getInstance(), &Nandina::ThemeManager::paletteChanged, this,
                &NanButtonStyle::updateColor);
    }

    NanButtonStyle& NanButtonStyle::operator=(NanButtonStyle const &style) {
        if (this != &style) {
            this->backgroundColor = style.backgroundColor;
            this->borderColor = style.borderColor;
            this->foregroundColor = style.foregroundColor;
            this->styleName = style.styleName;
            // 复制颜色引用
            this->backgroundColorRef = style.backgroundColorRef;
            this->borderColorRef = style.borderColorRef;
            this->foregroundColorRef = style.foregroundColorRef;
        }
        return *this;
    }

    QString NanButtonStyle::getStyleName() const { return this->styleName; }

    QString NanButtonStyle::getBackgroundColor() const { return this->backgroundColor; }

    QString NanButtonStyle::getBorderColor() const { return this->borderColor; }

    QString NanButtonStyle::getForegroundColor() const { return this->foregroundColor; }

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

    NanButtonStyle& NanButtonStyle::setBackgroundColorRef(const QString &s) {
        backgroundColorRef = s;
        return *this;
    }

    NanButtonStyle& NanButtonStyle::setBorderColorRef(const QString &s) {
        borderColorRef = s;
        return *this;
    }

    NanButtonStyle& NanButtonStyle::setForegroundColorRef(const QString &s) {
        foregroundColorRef = s;
        return *this;
    }

    // 将样式转换为QVariant以供QML使用
    QVariant NanButtonStyle::toVariant() {
        QVariantMap map;
        map.insert("style", this->getStyleName());
        map.insert("background", this->getBackgroundColor());
        map.insert("border", this->getBorderColor());
        map.insert("foreground", this->getForegroundColor());
        return map;
    }

    void NanButtonStyle::updateColor() {
        // 辅助函数：将颜色引用转换为实际颜色
        const auto obtainRealColor = [](const QString &colorRef) -> QString {
            // 如果颜色字符串中出现了@,则表示是引用主题颜色，需要进行转换
            if (not colorRef.startsWith("@") || colorRef == "transparent") {
                return colorRef;
            }
            const auto colorName = colorRef.mid(1); // 去掉@符号
            return ThemeManager::getInstance()->getColorByString(colorName);
        };

        // 重新解析颜色引用
        if (!backgroundColorRef.isEmpty()) {
            backgroundColor = obtainRealColor(backgroundColorRef);
        }
        if (!borderColorRef.isEmpty()) {
            borderColor = obtainRealColor(borderColorRef);
        }
        if (!foregroundColorRef.isEmpty()) {
            foregroundColor = obtainRealColor(foregroundColorRef);
        }

        // 发出信号通知颜色已更新
        emit styleChanged();

        qDebug() << "Updated colors for style:" << styleName << "bg:" << backgroundColor << "border:" << borderColor
                << "fg:" << foregroundColor;
    }
} // namespace Nandina::Components

template<>
auto Nandina::Core::Utils::JsonParser::parser<std::vector<Nandina::Components::NanButtonStyle>>(const QJsonObject &json)
    -> std::vector<Components::NanButtonStyle> {
    std::vector<Components::NanButtonStyle> styles{};

    if (const auto target = json.value("target").toString(); target != "NanButton") {
        throw std::runtime_error("JSON object is not for NanButtonStyle");
    }

    // todo util function will move to Core::Utils namespace
    const auto obtainRealColor = [](const QString &color) {
        // 如果颜色字符串中出现了@,则表示是引用主题颜色，需要进行转换
        if (not color.startsWith("@") || color == "transparent") {
            return color;
        }
        const auto colorName = color.mid(1); // 去掉@符号
        return ThemeManager::getInstance()->getColorByString(colorName);
    };

    for (const auto &item: json.value("styles").toArray()) {
        const auto styleObject = item.toObject();
        const auto type = styleObject.value("type").toString();
        const auto background = styleObject.value("background").toString();
        const auto border = styleObject.value("border").toString();
        const auto foreground = styleObject.value("foreground").toString();

        const auto realBackground = obtainRealColor(background);
        const auto realBorder = obtainRealColor(border);
        const auto realForeground = obtainRealColor(foreground);

        qDebug() << "[" << type << "]"
                << "background:" << realBackground << "border:" << realBorder << "foreground:" << realForeground;

        Components::NanButtonStyle style;
        style.setStyleName(type)
                .setBackgroundColor(realBackground)
                .setBorderColor(realBorder)
                .setForegroundColor(realForeground)
                // 同时保存原始颜色引用
                .setBackgroundColorRef(background)
                .setBorderColorRef(border)
                .setForegroundColorRef(foreground);
        styles.push_back(style);
    }

    return styles;
}
