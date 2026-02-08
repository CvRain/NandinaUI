#include "src/nan_button_registrar.hpp"
#include "src/nan_card_registrar.hpp"

namespace Nandina::Components {
    void registerAllComponents() {
        registerNanButtonFactory();
        registerNanCardFactory();
        // 当你有新组件时，在这里添加它们的注册函数调用
        // e.g. registerMyNewLabelFactory();
    }
} // namespace Nandina::Components
