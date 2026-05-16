module;

#include <memory>
#include <string_view>
#include <print>

export module nandina.showcase.sandbox_page;

import nandina.app.authoring;
import nandina.foundation.color;
import nandina.layout.container;
import nandina.layout.flex_widgets;
import nandina.widgets;

export namespace nandina::showcase {

    /**
     * SandboxPage — 组件验证沙盒（逐个重建 showcase）
     *
     * 当前阶段：空白起点，按顺序逐个加入组件并在每个步骤验证布局正确性。
     */
    class SandboxPage final : public nandina::app::NanPage {
    public:
        [[nodiscard]] auto route_key() const noexcept -> std::string_view override {
            return "sandbox";
        }

        [[nodiscard]] auto title() const noexcept -> std::string_view override {
            return "Sandbox";
        }

        [[nodiscard]] auto icon_type() const noexcept -> nandina::widgets::IconType override {
            return nandina::widgets::IconType::Dot;
        }

        [[nodiscard]] auto build() -> nandina::app::NanComponent::Ptr override {
            using namespace nandina::app;

            auto label = nandina::widgets::Label::create();
            label->set_text("Sandbox — Ready")
                .set_font_size(14.0f)
                .set_color(nandina::NanColor::from(nandina::NanRgb{180, 185, 210}));


            auto button = nandina::widgets::Button::create();
            button->set_size(1000,300);
            button->set_text("你好!");

            auto button_colors = nandina::widgets::ButtonColors{};
            button_colors.text = nandina::NanColor::from(NanRgb{"#4c4f69"});
            button_colors.bg = nandina::NanColor::from(NanRgb{"#e64553"});

            button->set_colors(button_colors);

            button->on_click([&]() {
                std::print("button click");
                throw std::runtime_error("button click");
            });

            // ── 步骤 0：空白起点 ────────────────────────────────
            // 仅显示一个居中 Label 验证 Shell → PageHost → Page 链路正常
            return mount(
                center(adopt([&] {
                    return std::move(button);
                }()))
                );
        }
    };

} // namespace nandina::showcase
