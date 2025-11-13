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

        // 虚析构函数：确保派生类对象通过基类指针删除时能正确调用派生类的析构函数
        ~BaseComponent() override = default;

        virtual void updateColor() = 0;

        virtual QVariant toVariant() = 0;
    };
} // namespace Nandina::Components

#endif // TRYNANDINA_BASE_COMPONENT_HPP
