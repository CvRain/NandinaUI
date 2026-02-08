#include "nan_card.hpp"
#include "Theme/Utils/color_utils.hpp"

namespace Nandina::Components {
    NanCardStyle::NanCardStyle(QObject *parent) : BaseComponent(parent) {
        connect(ThemeManager::getInstance(), &Nandina::ThemeManager::paletteChanged, this, &NanCardStyle::updateColor);
    }

    NanCardStyle::NanCardStyle(NanCardStyle const &style) : BaseComponent(nullptr) {
        this->backgroundColor = style.backgroundColor;
        this->borderColor = style.borderColor;
        this->foregroundColor = style.foregroundColor;
        this->styleName = style.styleName;
        this->backgroundColorRef = style.backgroundColorRef;
        this->borderColorRef = style.borderColorRef;
        this->foregroundColorRef = style.foregroundColorRef;
        connect(ThemeManager::getInstance(), &Nandina::ThemeManager::paletteChanged, this, &NanCardStyle::updateColor);
    }

    NanCardStyle &NanCardStyle::operator=(NanCardStyle const &style) {
        if (this != &style) {
            this->backgroundColor = style.backgroundColor;
            this->borderColor = style.borderColor;
            this->foregroundColor = style.foregroundColor;
            this->styleName = style.styleName;
            this->backgroundColorRef = style.backgroundColorRef;
            this->borderColorRef = style.borderColorRef;
            this->foregroundColorRef = style.foregroundColorRef;
        }
        return *this;
    }

    QString NanCardStyle::getStyleName() const { return this->styleName; }

    QString NanCardStyle::getBackgroundColor() const { return this->backgroundColor; }

    QString NanCardStyle::getBorderColor() const { return this->borderColor; }

    QString NanCardStyle::getForegroundColor() const { return this->foregroundColor; }

    NanCardStyle &NanCardStyle::setStyleName(const QString &s) {
        styleName = s;
        return *this;
    }

    NanCardStyle &NanCardStyle::setBackgroundColor(const QString &s) {
        backgroundColor = s;
        return *this;
    }

    NanCardStyle &NanCardStyle::setBorderColor(const QString &s) {
        borderColor = s;
        return *this;
    }

    NanCardStyle &NanCardStyle::setForegroundColor(const QString &s) {
        foregroundColor = s;
        return *this;
    }

    NanCardStyle &NanCardStyle::setBackgroundColorRef(const QString &s) {
        backgroundColorRef = s;
        return *this;
    }

    NanCardStyle &NanCardStyle::setBorderColorRef(const QString &s) {
        borderColorRef = s;
        return *this;
    }

    NanCardStyle &NanCardStyle::setForegroundColorRef(const QString &s) {
        foregroundColorRef = s;
        return *this;
    }

    QVariant NanCardStyle::toVariant() {
        QVariantMap map;
        map.insert("style", this->getStyleName());
        map.insert("background", this->getBackgroundColor());
        map.insert("border", this->getBorderColor());
        map.insert("foreground", this->getForegroundColor());
        return map;
    }

    void NanCardStyle::updateColor() {
        using Nandina::Theme::Utils::obtainRealColor;

        if (!backgroundColorRef.isEmpty()) {
            backgroundColor = obtainRealColor(backgroundColorRef);
        }
        if (!borderColorRef.isEmpty()) {
            borderColor = obtainRealColor(borderColorRef);
        }
        if (!foregroundColorRef.isEmpty()) {
            foregroundColor = obtainRealColor(foregroundColorRef);
        }

        emit styleChanged();

        qDebug() << "Updated card style" << styleName << "bg:" << backgroundColor << "border:" << borderColor
                 << "fg:" << foregroundColor;
    }
} // namespace Nandina::Components

template<>
auto Nandina::Core::Utils::JsonParser::parser<std::vector<Nandina::Components::NanCardStyle>>(const QJsonObject &json)
        -> std::vector<Components::NanCardStyle> {
    using Nandina::Theme::Utils::obtainRealColor;

    std::vector<Components::NanCardStyle> styles{};

    if (const auto target = json.value("target").toString(); target != "NanCard") {
        throw std::runtime_error("JSON object is not for NanCardStyle");
    }

    for (const auto &item: json.value("styles").toArray()) {
        const auto styleObject = item.toObject();
        const auto type = styleObject.value("type").toString();
        const auto background = styleObject.value("background").toString();
        const auto border = styleObject.value("border").toString();
        const auto foreground = styleObject.value("foreground").toString();

        const auto realBackground = obtainRealColor(background);
        const auto realBorder = obtainRealColor(border);
        const auto realForeground = obtainRealColor(foreground);

        Components::NanCardStyle style;
        style.setStyleName(type)
                .setBackgroundColor(realBackground)
                .setBorderColor(realBorder)
                .setForegroundColor(realForeground)
                .setBackgroundColorRef(background)
                .setBorderColorRef(border)
                .setForegroundColorRef(foreground);
        styles.push_back(style);
    }

    return styles;
}
