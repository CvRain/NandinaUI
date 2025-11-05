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

    private:
    };
}

#endif //TRYNANDINA_COMPONENT_COLLECTION_HPP
