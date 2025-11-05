//
// Created by cvrain on 2025/11/4.
//

#ifndef TRYNANDINA_NAN_BUTTON_HPP
#define TRYNANDINA_NAN_BUTTON_HPP

#include <qqmlintegration.h>

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
    inline Components::NanButtonStyle parser<Components::NanButtonStyle>(const QJsonObject &json) {
        Nandina::Components::NanButtonStyle style;

        return style;
    }
}

#endif //TRYNANDINA_NAN_BUTTON_HPP
