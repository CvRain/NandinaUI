//
// Created by cvrain on 2025/11/4.
//

#include "component_manager.hpp"

#include <QDir>
#include <QDirIterator>

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

    ComponentManager::ComponentManager(QObject *parent)
        : QObject(parent), componentCollection(std::make_shared<ComponentCollection>()) {
        const QString component_style_directory = ":/qt/qml/Nandina/Components/styles";
        // 遍历目录，加载样式文件 *.json

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

    void ComponentManager::loadComponentStyles(const QString &fileName, const QString &filePath) const {
        using namespace Core::Utils;

        const auto jsonDocument = FileOperator::readJsonFile(filePath);
        if (not jsonDocument.has_value()) {
            throw std::runtime_error("Failed to read component style file: " + filePath.toStdString());
        }

        //todo 临时编写，亟待改进，修改jsonDocument加载方式
        if (fileName == "NanButton") {
            const auto component = JsonParser::parser<std::vector<NanButtonStyle>>(
                jsonDocument.value().object());
            std::ranges::for_each(component, [this](const NanButtonStyle &style) {
                qDebug() << "Loaded NanButton style:" << style.getStyleName();
                this->componentCollection->buttonStyles.insert({style.getStyleName(), style});
            });
        }
    }
}
