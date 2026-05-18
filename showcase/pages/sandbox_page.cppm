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
import nandina.theme;

export namespace nandina::showcase {

    /**
     * SandboxPage — 组件验证沙盒（P0: 字体 + 样式链式 API 演示）
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

            // ── 演示 1: 直接用链式 API 配置按钮字体 ──────────
            auto primary_btn = button("Primary Action")
                .font(
                    nandina::text::NanFont{}
                    .color(NanColor::from(NanRgb{"#e64553"}))
                    .weight(text::NanFontWeight::black)
                    .letter_spacing(1.0f)
                    .size(18)
                    )
                .width(300)
                .height(150)
                .on_click([] {
                    std::print("primary clicked!\n");
                });

            // ── 演示 2: NanFont 统一配置 ──────────────────
            auto heading_font = nandina::text::NanFont{}
                .size(24)
                .weight(nandina::text::NanFontWeight::extraBold)
                .color(NanColor::from(NanRgb{"#e64553"}))
                .letter_spacing(1.0f)
                .single_line(true);

            // ── 演示 3: button_style 应用全局样式 ──────────
            auto style                 = nandina::theme::NanStylePrimitives::default_style();
            style.button.corner_radius = 12.0f;
            style.button.padding       = nandina::geometry::NanInsets{16.0f, 10.0f, 16.0f, 10.0f};
            style.button.bg            = nandina::NanColor::from(nandina::NanRgb{230, 69, 83});

            auto styled_btn = button("Styled Button")
                .button_style(style.button)
                .width(250)
                .height(100)
                .on_click([] {
                    std::print("styled button clicked!\n");
                });

            // ── 布局（children 无需 std::move；.width() 自动包裹为 SizedBox）──
            auto btn_row = row(children(
                primary_btn,
                sized_box(spacer()).width(20),
                styled_btn
                )).gap(12);

            return mount(
                column(children(
                    label("NandinaUI Sandbox")
                    .font(std::move(heading_font))
                    .align(widgets::TextAlign::Start)
                    .width(320),
                    btn_row
                    ))
                .gap(16)
                .padding(24)
                );
        }
    };

} // namespace nandina::showcase
