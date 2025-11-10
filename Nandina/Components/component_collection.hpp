//
// Created by cvrain on 2025/11/6.
//

#ifndef TRYNANDINA_COMPONENT_COLLECTION_HPP
#define TRYNANDINA_COMPONENT_COLLECTION_HPP

#include <QObject>

#include "nan_button.hpp"

namespace Nandina::Components {
    class ComponentCollection {
    public:
        explicit ComponentCollection();

        std::map<QString, NanButtonStyle> buttonStyles{};
    };
} // namespace Nandina::Components


#endif // TRYNANDINA_COMPONENT_COLLECTION_HPP
