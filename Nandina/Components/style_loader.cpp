//
// Created by cvrain on 2025/11/8.
//

#include "style_loader.hpp"

namespace Nandina::Components {
    void StyleLoader<NanButtonStyle>::load(const ComponentManager *manager, const QJsonDocument &document) {
        using namespace Core::Utils;
        for (const auto styles = JsonParser::parser<std::vector<NanButtonStyle>>(document.object());
             const auto &style: styles) {
            manager->addButtonStyle(style);
        }
    }
} // namespace Nandina::Components
