//
// Created by cvrain on 2025/11/4.
//

#ifndef TRYNANDINA_COMPONENT_MANAGER_HPP
#define TRYNANDINA_COMPONENT_MANAGER_HPP


#include <QQmlEngine>

#include "base_component.hpp"
#include "nan_button.hpp"

namespace Nandina::Components {
    template<typename T>
    concept InhertByBaseComponent = std::is_base_of_v<T, BaseComponent>;

    class ComponentManager : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

    public:
        static ComponentManager* getInstance();

        static ComponentManager* create(const QQmlEngine *qmlEngine, const QJSEngine *jsEngine);

    private:
        explicit ComponentManager(QObject *parent = nullptr);

        static ComponentManager *instance;

        std::map<QString, NanButtonStyle> buttonStyles;
    };
}


#endif //TRYNANDINA_COMPONENT_MANAGER_HPP
