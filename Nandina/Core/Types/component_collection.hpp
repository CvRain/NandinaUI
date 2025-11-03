//
// Created by cvrain on 2025/11/2.
//

#ifndef TRYNANDINA_COMPONENT_COLLECTION_HPP
#define TRYNANDINA_COMPONENT_COLLECTION_HPP

#include <QObject>

namespace Nandina::Core::Types {
    class ComponentCollection {
    public:
        explicit ComponentCollection() = default;

        enum class ComponentType {
            NanButton,
        };

        // 保存组件类型对应的样式文件路径
        static std::map<ComponentCollection::ComponentType, QString> componentStylePath;

        // 保存组件类型对应的字符串名字
        static std::map<ComponentCollection::ComponentType, QString> componentName;
    };
}

#endif //TRYNANDINA_COMPONENT_COLLECTION_HPP
