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
        QML_ELEMENT

    public:
        explicit BaseComponent(QObject *parent = nullptr);
    };
}

#endif //TRYNANDINA_BASE_COMPONENT_HPP
