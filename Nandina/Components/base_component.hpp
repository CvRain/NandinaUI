//
// Created by cvrain on 2025/11/4.
//

#ifndef TRYNANDINA_BASE_COMPONENT_HPP
#define TRYNANDINA_BASE_COMPONENT_HPP

#include <QQmlEngine>

#include "Core/Utils/json_parser.hpp"

namespace Nandina::Components {
    class BaseComponent : public QObject {
        Q_OBJECT

    public:
        explicit BaseComponent(QObject *parent = nullptr);

        virtual void updateColor() = 0;

        virtual QVariant toVariant() = 0;
    };
} // namespace Nandina::Components

#endif // TRYNANDINA_BASE_COMPONENT_HPP
