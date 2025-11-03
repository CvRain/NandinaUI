//
// Created by cvrain on 2025/11/2.
//

#include "component_collection.hpp"

namespace Nandina::Core::Types {
    std::map<ComponentCollection::ComponentType, QString> ComponentCollection::componentStylePath = {
        std::make_pair(ComponentType::NanButton, ":/qt/qml/Nandina/Theme/Components/NanButton.json")
    };

    std::map<ComponentCollection::ComponentType, QString> ComponentCollection::componentName = {
        std::make_pair(ComponentType::NanButton, "NanButton")
    };
}
