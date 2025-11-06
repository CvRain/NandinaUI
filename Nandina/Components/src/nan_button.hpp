//
// Created by cvrain on 2025/11/4.
//

#ifndef TRYNANDINA_NAN_BUTTON_HPP
#define TRYNANDINA_NAN_BUTTON_HPP

#include <qqmlintegration.h>
#include <QJsonArray>
#include <QJsonObject>

#include "base_component.hpp"

namespace Nandina::Components {
    class NanButtonStyle : public BaseComponent {
        Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(QString style READ getStyleName NOTIFY styleChanged)
        Q_PROPERTY(QString background READ getForegroundColor NOTIFY styleChanged)
        Q_PROPERTY(QString border READ getBackgroundColor NOTIFY styleChanged)
        Q_PROPERTY(QString foreground READ getBorderColor NOTIFY styleChanged)

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

    signals:
        void styleChanged(const QString &style);

    private:
        QString styleName;
        QString backgroundColor;
        QString borderColor;
        QString foregroundColor;
    };
}

namespace Nandina::Core::Utils::JsonParser {
    template<>
    inline std::vector<Components::NanButtonStyle> parser<std::vector<Components::NanButtonStyle>>(const QJsonObject &json) {
        std::vector<Components::NanButtonStyle> out;

        auto readStyleObj = [&](const QJsonObject &obj) {
            Components::NanButtonStyle style;

            auto getString = [&](std::initializer_list<QString> keys) -> QString {
                for (const auto &k : keys) {
                    if (obj.contains(k) && obj.value(k).isString()) return obj.value(k).toString();
                }
                return {};
            };

            QString sname = getString({"style", "styleName", "name"});
            QString bg = getString({"background", "backgroundColor", "bg"});
            QString br = getString({"border", "borderColor", "br"});
            QString fg = getString({"foreground", "foregroundColor", "color"});

            if (!sname.isEmpty()) style.setStyleName(sname);
            if (!bg.isEmpty()) style.setBackgroundColor(bg);
            if (!br.isEmpty()) style.setBorderColor(br);
            if (!fg.isEmpty()) style.setForegroundColor(fg);

            out.push_back(style);
        };

        // If there is a top-level "styles" array
        if (json.contains("styles") && json.value("styles").isArray()) {
            QJsonArray arr = json.value("styles").toArray();
            for (const QJsonValue &v : arr) {
                if (v.isObject()) readStyleObj(v.toObject());
            }
            return out;
        }

        // Support nested { "NanButton": { ... } } structure
        if (json.contains("NanButton") && json.value("NanButton").isObject()) {
            QJsonObject nb = json.value("NanButton").toObject();
            if (nb.contains("styles") && nb.value("styles").isArray()) {
                for (const QJsonValue &v : nb.value("styles").toArray()) {
                    if (v.isObject()) readStyleObj(v.toObject());
                }
            } else {
                // Treat the NanButton object itself as a style object
                readStyleObj(nb);
            }
            return out;
        }

        // Otherwise treat the entire object as a single style
        readStyleObj(json);
        return out;
    }
}

#endif //TRYNANDINA_NAN_BUTTON_HPP
