//
// Created by cvrain on 2025/11/4.
//

#ifndef TRYNANDINA_COMPONENT_MANAGER_HPP
#define TRYNANDINA_COMPONENT_MANAGER_HPP


#include <QQmlEngine>

#include "base_component.hpp"
#include "component_collection.hpp"
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

        Q_INVOKABLE QStringList getComponentStyleNames();

        Q_INVOKABLE [[nodiscard]] Nandina::Components::NanButtonStyle* getButtonStyle(const QString &name) const;

        void addButtonStyle(const NanButtonStyle &style) const;

    private:
        explicit ComponentManager(QObject *parent = nullptr);

        void loadComponentStyles(const QString &fileName, const QString &filePath);

        static ComponentManager *instance;

        std::shared_ptr<ComponentCollection> componentCollection;
    };
}


#endif //TRYNANDINA_COMPONENT_MANAGER_HPP
