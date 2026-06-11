#include <memory>

import nandina.app.application;
import nandina.showcase;

auto main() -> int {
    nandina::app::NanApplication app;

    // 运行主窗口
    app.run(std::make_unique<MainWindow>());

    return 0;
}
