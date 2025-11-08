// Created by automated refactor on 2025/11/08

#include "component_factory.hpp"
#include <QDebug>

namespace Nandina::Components {
    ComponentFactoryRegistry& ComponentFactoryRegistry::instance() {
        static ComponentFactoryRegistry inst;
        return inst;
    }

    void ComponentFactoryRegistry::registerFactory(const QString &typeName, Factory factory) {
        factories.insert(typeName, std::move(factory));
        qDebug() << "ComponentFactoryRegistry: registered factory for type" << typeName;
    }

    bool ComponentFactoryRegistry::invoke(const QString &typeName, ComponentManager *manager,
                                          const QJsonDocument &doc) const {
        const auto it = factories.find(typeName);
        if (it != factories.end()) {
            qDebug() << "ComponentFactoryRegistry: invoking factory for type" << typeName;
            it.value()(manager, doc);
            return true;
        }
        qWarning() << "ComponentFactoryRegistry: no factory for type" << typeName;
        return false;
    }
} // namespace Nandina::Components
