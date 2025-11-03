//
// Created by cvrain on 2025/11/3.
//

#ifndef TRYNANDINA_COMPONENT_STYLE_HPP
#define TRYNANDINA_COMPONENT_STYLE_HPP
#include <QJsonObject>
#include <QObject>
#include <qqmlintegration.h>

namespace Nandina::Theme::Components {
    class ComponentStyle : public QObject {
        Q_OBJECT
    public:
        explicit ComponentStyle(QObject *parent = nullptr);

        virtual bool loadFromJson(const QJsonObject &json) = 0;

        [[nodiscard]] virtual bool isValid() const = 0;
    };
}

#endif //TRYNANDINA_COMPONENT_STYLE_HPP
