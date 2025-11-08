#include "src/nan_button_registrar.hpp"

namespace Nandina::Components {
    void registerAllComponents() {
        registerNanButtonFactory();
        // 当你有新组件时，在这里添加它们的注册函数调用
        // e.g. registerMyNewLabelFactory();
    }
}

