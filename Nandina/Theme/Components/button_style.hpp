//
// Created by cvrain on 2025/11/3.
//

#ifndef TRYNANDINA_BUTTON_STYLE_HPP
#define TRYNANDINA_BUTTON_STYLE_HPP

#include "component_style.hpp"

namespace Nandina::Theme::Components {
    class NanButtonStyle : public ComponentStyle {
        Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(QString background READ getBackground CONSTANT)
        Q_PROPERTY(QString foreground READ getForeground CONSTANT)
        Q_PROPERTY(QString border READ getBorder CONSTANT)

    public:
        explicit NanButtonStyle(QObject *parent = nullptr);
        NanButtonStyle(const NanButtonStyle &other);
        NanButtonStyle& operator=(const NanButtonStyle &other);

        bool loadFromJson(const QJsonObject &json) override;

        bool isValid() const override;

        [[nodiscard]] QString getBackground() const;
        [[nodiscard]] QString getForeground() const;
        [[nodiscard]] QString getBorder() const;

    private:
        QString background{};
        QString foreground{};
        QString border{};
    };
}

#endif //TRYNANDINA_BUTTON_STYLE_HPP
