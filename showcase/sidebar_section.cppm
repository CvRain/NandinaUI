module;

#include <array>
#include <memory>
#include <string_view>
#include <tuple>

export module nandina.showcase.sidebar_section;

import nandina.foundation.color;
import nandina.foundation.nan_size;
import nandina.runtime.nan_widget;
import nandina.widgets.icon;
import nandina.widgets.sidebar;
import nandina.widgets.sidebar_menu_button;

namespace {
static auto color4_to_nancolor(const uint8_t r, const uint8_t g, const uint8_t b,
    const uint8_t a = 255) -> nandina::NanColor {
    return nandina::NanColor::from(nandina::NanRgb{r, g, b, a});
}
}

export namespace nandina::showcase {

class SidebarSection final : public nandina::widgets::Sidebar {
public:
    using Ptr = std::unique_ptr<SidebarSection>;

    static auto create() -> Ptr {
        return Ptr{new SidebarSection()};
    }

    [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
        return {240.0f, 0.0f};
    }

private:
    SidebarSection() {
        using nandina::widgets::IconType;
        using nandina::widgets::SidebarMenuButton;

        {
            auto dash = SidebarMenuButton::create();
            dash->set_label("Dashboard")
                .set_icon_type(IconType::Square)
                .set_active(true)
                .set_accent_color(color4_to_nancolor(99, 102, 241));
            add_menu_item(std::move(dash));
        }
        {
            auto proj = SidebarMenuButton::create();
            proj->set_label("Projects")
                .set_icon_type(IconType::Square);
            add_menu_item(std::move(proj));
        }
        {
            auto anal = SidebarMenuButton::create();
            anal->set_label("Analytics")
                .set_icon_type(IconType::Square);
            add_menu_item(std::move(anal));
        }
        {
            auto sett = SidebarMenuButton::create();
            sett->set_label("Settings")
                .set_icon_type(IconType::Square);
            add_menu_item(std::move(sett));
        }

        const auto project_colors = std::to_array<std::tuple<uint8_t, uint8_t, uint8_t>>({
            {99, 102, 241},
            {95, 200, 130},
            {245, 158, 60},
            {236, 110, 130},
        });
        const auto project_names = std::to_array<std::string_view>({
            "nandina-ui", "layout-core", "flex-box", "render-test"
        });

        for (size_t i = 0; i < project_names.size(); ++i) {
            auto item = SidebarMenuButton::create();
            item->set_label(project_names[i])
                .set_icon_type(IconType::Dot);
            const auto& [pr, pg, pb] = project_colors[i];
            item->set_accent_color(color4_to_nancolor(pr, pg, pb));
            add_project_item(std::move(item));
        }

        set_user_name("CvRain")
            .set_user_role("Developer");
    }
};

} // namespace nandina::showcase