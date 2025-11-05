//
// Created by cvrain on 2025/11/4.
//

#include "component_manager.hpp"

namespace Nandina::Components {
    ComponentManager *ComponentManager::instance = nullptr;

    ComponentManager* ComponentManager::getInstance() {
        if (instance == nullptr) {
            instance = new ComponentManager();
        }
        return instance;
    }

    ComponentManager* ComponentManager::create(const QQmlEngine *qmlEngine, const QJSEngine *jsEngine) {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)
        return ComponentManager::getInstance();
    }

    ComponentManager::ComponentManager(QObject *parent)
        : QObject(parent) {
        const QString component_style_directory = ":/qt/qml/Nandina/Components/Styles";
    }
}
