//
// Created by cvrain on 2025/11/4.
//

#ifndef TRYNANDINA_NAN_BUTTON_HPP
#define TRYNANDINA_NAN_BUTTON_HPP

#include <QJsonArray>
#include <QJsonObject>
#include <qqmlintegration.h>

#include "base_component.hpp"
#include "component_factory.hpp"
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

        // 设置原始颜色引用（如 @lavender）
        NanButtonStyle& setBackgroundColorRef(const QString &s);

        NanButtonStyle& setBorderColorRef(const QString &s);

        NanButtonStyle& setForegroundColorRef(const QString &s);

        QVariant toVariant() override;

        void updateColor() override;

    signals:
        void styleChanged();

    private:
        QString styleName;
        QString backgroundColor;
        QString borderColor;
        QString foregroundColor;

        // 保存原始颜色引用，用于主题切换时重新解析
        QString backgroundColorRef;
        QString borderColorRef;
        QString foregroundColorRef;
    };
} // namespace Nandina::Components

namespace Nandina::Core::Utils::JsonParser {
    template<>
    auto parser<std::vector<Components::NanButtonStyle>>(const QJsonObject &json)
        -> std::vector<Components::NanButtonStyle>;
}

namespace Nandina::Components {
    static ComponentRegistrar<NanButtonStyle> registerNanButtonStyle(QStringLiteral("NanButton"));
}

#endif // TRYNANDINA_NAN_BUTTON_HPP
