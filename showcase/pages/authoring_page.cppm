module;

#include <memory>
#include <string_view>

export module nandina.showcase.authoring_page;

import nandina.app.authoring;
import nandina.foundation.color;
import nandina.widgets.icon;

export class AuthoringPage final : public nandina::app::NanPage {
public:
    [[nodiscard]] auto route_key() const noexcept -> std::string_view override { return "authoring"; }
    [[nodiscard]] auto title()     const noexcept -> std::string_view override { return "Authoring"; }
    [[nodiscard]] auto icon_type() const noexcept -> nandina::widgets::IconType override {
        return nandina::widgets::IconType::Dots;
    }

    [[nodiscard]] auto build() -> nandina::app::NanComponent::Ptr override {
        using namespace nandina::app;
        using nandina::NanColor;
        using nandina::NanRgb;

        return mount(
            center(
                column(children(
                    label("Authoring")
                        .font_size(24.0f)
                        .color(NanColor::from(NanRgb{210, 214, 240})),
                    label("mount / row / column / padding / center / label / button / card / panel")
                        .font_size(10.0f)
                        .color(NanColor::from(NanRgb{120, 124, 150})),
                    label("即将推出 · 敬请期待")
                        .font_size(9.0f)
                        .color(NanColor::from(NanRgb{90, 94, 120}))
                )).gap(12.0f)
            )
        );
    }
};
