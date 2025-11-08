//
// Created by cvrain on 2025/11/4.
//

#include "component_manager.hpp"

#include <QDir>
#include <QDirIterator>

#include "component_factory.hpp"
#include "Core/Utils/file_operator.hpp"
#include "Core/Utils/json_parser.hpp"

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

    /**
     * 一个测试函数，返回当前实现的组件样式名称列表
     * @return 包含当前实现的组件样式名称的字符串列表
     */
    QStringList ComponentManager::getComponentStyleNames() {
        return {
            "NanButton",
        };
    }

    NanButtonStyle* ComponentManager::getButtonStyle(const QString &name) const {
        //检查样式是否存在，如果不存在发出警告且返回第一个样式
        if (not componentCollection->buttonStyles.contains(name)) {
            qWarning() << "Component style directory does not exist:" << name;
            return &componentCollection->buttonStyles.begin()->second;
        }
        return &componentCollection->buttonStyles.at(name);
    }


    void ComponentManager::addButtonStyle(const NanButtonStyle &style) const {
        this->componentCollection->buttonStyles.insert({style.getStyleName(), style});
    }

    ComponentManager::ComponentManager(QObject *parent)
        : QObject(parent), componentCollection(std::make_shared<ComponentCollection>()) {
        const QString component_style_directory = ":/qt/qml/Nandina/Components/styles";

        if (QDir dir(component_style_directory); not dir.exists()) {
            throw std::runtime_error(
                "Component style directory does not exist: " + component_style_directory.toStdString());
        }


        QDirIterator it(component_style_directory, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const auto filePath = it.next();
            const auto fileInfo = it.fileInfo();

            loadComponentStyles(fileInfo.baseName(), filePath);
        }
    }

    void ComponentManager::loadComponentStyles(const QString &fileName, const QString &filePath) {
        using namespace Core::Utils;

        const auto jsonDocument = FileOperator::readJsonFile(filePath);
        if (not jsonDocument.has_value()) {
            throw std::runtime_error("Failed to read component style file: " + filePath.toStdString());
        }

        // 使用工厂注册表来处理不同组件类型的样式加载
        const QString typeName = fileName; // 文件名即类型名的约定
        using namespace Nandina::Components;

        if (not ComponentFactoryRegistry::instance().invoke(typeName, this, jsonDocument.value())) {
            qWarning() << "loadComponentStyles: no registered handler for type" << typeName << "- skipping";
        }
    }
}
