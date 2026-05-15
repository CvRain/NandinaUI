module;

#include <memory>

export module nandina.showcase.registry;

import nandina.app.authoring;
import nandina.showcase.overview_page;
import nandina.showcase.layout_page;
import nandina.showcase.widgets_page;
import nandina.showcase.authoring_page;

export namespace nandina::showcase {

    auto register_default_pages(app::NanRouter& router) -> void {
        router.register_page(std::make_unique<OverviewPage>());
        router.register_page(std::make_unique<LayoutPage>());
        router.register_page(std::make_unique<WidgetsPage>());
        router.register_page(std::make_unique<AuthoringPage>());
    }

} // namespace nandina::showcase