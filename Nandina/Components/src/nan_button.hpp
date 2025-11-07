//
// Created by cvrain on 2025/11/4.
//

#ifndef TRYNANDINA_NAN_BUTTON_HPP
#define TRYNANDINA_NAN_BUTTON_HPP

#include <qqmlintegration.h>
#include <QJsonArray>
#include <QJsonObject>

#include "base_component.hpp"
#include "themeManager.hpp"

namespace Nandina::Components {
    class NanButtonStyle : public BaseComponent {
        Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(QString style READ getStyleName NOTIFY styleChanged)
        Q_PROPERTY(QString background READ getBackgroundColor NOTIFY styleChanged)
        Q_PROPERTY(QString border READ getBorderColor NOTIFY styleChanged)
        Q_PROPERTY(QString foreground READ getForegroundColor NOTIFY styleChanged)

    public:
        explicit NanButtonStyle(QObject *parent = nullptr);

        NanButtonStyle(NanButtonStyle const &style);

        NanButtonStyle& operator=(NanButtonStyle const &style);

        [[nodiscard]] QString getStyleName() const;

        [[nodiscard]] QString getBackgroundColor() const;

        [[nodiscard]] QString getBorderColor() const;

        [[nodiscard]] QString getForegroundColor() const;

        NanButtonStyle& setStyleName(const QString &s);

        NanButtonStyle& setBackgroundColor(const QString &s);

        NanButtonStyle& setBorderColor(const QString &s);

        NanButtonStyle& setForegroundColor(const QString &s);

        //todo
        void updateColor() override;

    signals:
        void styleChanged();

    private:
        QString styleName;
        QString backgroundColor;
        QString borderColor;
        QString foregroundColor;
    };
}

namespace Nandina::Core::Utils::JsonParser {
    template<>
    inline auto parser<std::vector<Components::NanButtonStyle>>(const QJsonObject &json)
        -> std::vector<Components::NanButtonStyle> {
        std::vector<Components::NanButtonStyle> styles{};

        if (const auto target = json.value("target").toString(); target != "NanButton") {
            throw std::runtime_error("JSON object is not for NanButtonStyle");
        }

        //todo util function will move to Core::Utils namespace
        const auto obtainRealColor = [](const QString& color) {
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
                    << "background:" << realBackground
                    << "border:" << realBorder
                    << "foreground:" << realForeground;

            Components::NanButtonStyle style;
            style.setStyleName(type)
                    .setBackgroundColor(realBackground)
                    .setBorderColor(realBorder)
                    .setForegroundColor(realForeground);
            styles.push_back(style);
        }

        return styles;
    }
}

#endif //TRYNANDINA_NAN_BUTTON_HPP
