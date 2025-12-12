//
// Created by cvrain on 2025/11/4.
//

#include "component_manager.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QMutex>

#include "Core/Utils/file_operator.hpp"
#include "Core/Utils/json_parser.hpp"
#include "component_factory.hpp"

namespace Nandina::Components {
    ComponentManager *ComponentManager::instance = nullptr;

    ComponentManager *ComponentManager::getInstance(QObject *parent) {
        // 使用静态互斥锁保护单例创建过程，确保线程安全
        static QMutex mutex;
        QMutexLocker locker(&mutex);

        if (instance == nullptr) {
            // 检查 QCoreApplication 是否已初始化
            QObject *parentObj = parent ? parent : QCoreApplication::instance();
            if (!parentObj && !parent) {
                qWarning() << "ComponentManager::getInstance: QCoreApplication not initialized!";
                return nullptr;
            }
            instance = new ComponentManager(parentObj);
        }
        return instance;
    }

    ComponentManager *ComponentManager::create(const QQmlEngine *qmlEngine, const QJSEngine *jsEngine) {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)
        // QML 单例创建时，QQmlEngine 会负责管理其生命周期
        return getInstance(nullptr);
    }

    // NanButtonStyle* ComponentManager::getButtonStyle(const QString &name) const {
    //     // 检查样式是否存在，如果不存在发出警告且返回第一个样式
    //     if (not componentCollection->buttonStyles.contains(name)) {
    //         qWarning() << "Component style directory does not exist:" << name;
    //         return &componentCollection->buttonStyles.begin()->second;
    //     }
    //     return &componentCollection->buttonStyles.at(name);
    // }

    QVariant ComponentManager::getStyle(const QString &component, const QString &name) const {
        if (component == "NanButton") {
            // 检查样式是否存在
            if (!componentCollection->buttonStyles.contains(name)) {
                qWarning() << "Button style not found:" << name << "- using default";
                // 返回第一个可用样式或创建默认样式
                if (!componentCollection->buttonStyles.empty()) {
                    return componentCollection->buttonStyles.begin()->second.toVariant();
                }
                // 如果连默认样式都没有，返回空
                qCritical() << "No button styles available at all!";
                return {};
            }
            const auto result = this->componentCollection->buttonStyles.at(name).toVariant();
            return result;
        }
        return {};
    }


    void ComponentManager::addButtonStyle(const NanButtonStyle &style) const {
        this->componentCollection->buttonStyles.insert({style.getStyleName(), style});
    }

    ComponentManager::ComponentManager(QObject *parent) :
        QObject(parent), componentCollection(std::make_shared<ComponentCollection>()) {
        const QString component_style_directory = ":/qt/qml/Nandina/Components/styles";

        if (QDir dir(component_style_directory); not dir.exists()) {
            throw std::runtime_error("Component style directory does not exist: " +
                                     component_style_directory.toStdString());
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
} // namespace Nandina::Components
