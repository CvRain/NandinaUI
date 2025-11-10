// Created by automated refactor on 2025/11/08
#pragma once

#include <QString>
#include <functional>

#include "base_component.hpp"

namespace Nandina::Components {
    class ComponentManager;

    class ComponentFactoryRegistry {
    public:
        // Factory now represents a loader: given the manager and the JSON document, register styles/etc.
        using Factory = std::function<void(ComponentManager *manager, const QJsonDocument &doc)>;

        static ComponentFactoryRegistry &instance();

        void registerFactory(const QString &typeName, Factory factory);
        bool invoke(const QString &typeName, ComponentManager *manager, const QJsonDocument &doc) const;

    private:
        QHash<QString, Factory> factories;
    };

    // Optional helper registrar left for future use (no-op here)
    template<typename T>
    struct ComponentRegistrar {
        explicit ComponentRegistrar(const QString &typeName) { Q_UNUSED(typeName) }
    };
} // namespace Nandina::Components
