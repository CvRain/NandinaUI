#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <qqmlintegration.h>

#include "base_component.hpp"
#include "component_factory.hpp"
#include "themeManager.hpp"

namespace Nandina::Components {
    class NanCardStyle : public BaseComponent {
        Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(QString style READ getStyleName NOTIFY styleChanged)
        Q_PROPERTY(QString background READ getBackgroundColor NOTIFY styleChanged)
        Q_PROPERTY(QString border READ getBorderColor NOTIFY styleChanged)
        Q_PROPERTY(QString foreground READ getForegroundColor NOTIFY styleChanged)

    public:
        explicit NanCardStyle(QObject *parent = nullptr);

        NanCardStyle(NanCardStyle const &style);

        NanCardStyle &operator=(NanCardStyle const &style);

        [[nodiscard]] QString getStyleName() const;

        [[nodiscard]] QString getBackgroundColor() const;

        [[nodiscard]] QString getBorderColor() const;

        [[nodiscard]] QString getForegroundColor() const;

        NanCardStyle &setStyleName(const QString &s);

        NanCardStyle &setBackgroundColor(const QString &s);

        NanCardStyle &setBorderColor(const QString &s);

        NanCardStyle &setForegroundColor(const QString &s);

        NanCardStyle &setBackgroundColorRef(const QString &s);

        NanCardStyle &setBorderColorRef(const QString &s);

        NanCardStyle &setForegroundColorRef(const QString &s);

        QVariant toVariant() override;

        void updateColor() override;

    signals:
        void styleChanged();

    private:
        QString styleName;
        QString backgroundColor;
        QString borderColor;
        QString foregroundColor;

        QString backgroundColorRef;
        QString borderColorRef;
        QString foregroundColorRef;
    };
} // namespace Nandina::Components

namespace Nandina::Core::Utils::JsonParser {
    template<>
    auto parser<std::vector<Components::NanCardStyle>>(const QJsonObject &json)
            -> std::vector<Components::NanCardStyle>;
}

namespace Nandina::Components {
    static ComponentRegistrar<NanCardStyle> registerNanCardStyle(QStringLiteral("NanCard"));
}
