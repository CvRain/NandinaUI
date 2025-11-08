//
// Created by cvrain on 2025/11/8.
//

#ifndef TRYNANDINA_STYLE_LOADER_HPP
#define TRYNANDINA_STYLE_LOADER_HPP

#include "Core/Utils/json_parser.hpp"
#include "src/nan_button.hpp"
#include "component_manager.hpp"

namespace Nandina::Components {
    class ComponentManager;

    template<typename T>
    class StyleLoader {
    public:
        static void load(ComponentManager *manager, const QJsonDocument &loader) {
        }
    };

    template<>
    class StyleLoader<NanButtonStyle> {
    public:
        static void load(ComponentManager *manager, const QJsonDocument &loader);
    };
}


#endif //TRYNANDINA_STYLE_LOADER_HPP