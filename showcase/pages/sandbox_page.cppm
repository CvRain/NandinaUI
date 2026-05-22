module;

#include <string_view>
#include <print>

export module nandina.showcase.sandbox_page;

import nandina.app.authoring;   // re-exports: nandina.app.var, nandina.reactive
import nandina.foundation.color;
import nandina.layout.container;
import nandina.layout.flex_widgets;
import nandina.widgets;
import nandina.theme;

export namespace nandina::showcase {

    /**
     * SandboxPage — 组件验证沙盒（typed builder 演示）
     *
     * 演示三种 authoring 模式：
     *   A. 纯 rvalue 链式配置（所有属性和回调在单个表达式内完成，无中间变量）
     *   B. lvalue 分段配置（先构建节点，再对 lvalue 补充 on_click/on_hover/on_leave）
     *   C. 跨 widget 回调引用（Ref<Button> 类成员 + .bind()，在别处回调中访问节点）
     *
     * 同时演示 Var<T>：声明即用，无需 Pimpl / 匿名 namespace / 手动连接管理。
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
            using nandina::theme::ButtonVariant;
            using nandina::theme::ButtonSize;

            // ── 响应式绑定（一行，替代旧的匿名 namespace + Pimpl + on_change）──
            m_count.bind_text(label_button_ref,
                [](int v) { return std::to_string(v); });

            // ── Pattern C：Ref<Button> 成员 + .bind() ─────────────────────────
            auto label_button = button()
                .bind(label_button_ref)
                .size({240, 60})
                .variant(ButtonVariant::outline)
                .text(std::to_string(m_count()));

            // ── Pattern B：lvalue 分段配置 ────────────────────────────────────
            auto increase_button = button()
                .bind(increase_button_ref)
                .size({120, 60})
                .text("+");

            increase_button.on_click([this]() {
                m_count = m_count() + 1;
            });
            increase_button.on_hover([this]() {
                if (increase_button_ref)
                    increase_button_ref->set_text("▲");
            });
            increase_button.on_leave([this]() {
                if (increase_button_ref)
                    increase_button_ref->set_text("+");
            });

            auto decrease_button = button()
                .bind(decrease_button_ref)
                .size({120, 60})
                .text("-");

            decrease_button.on_click([this]() {
                m_count = m_count() - 1;
            });
            decrease_button.on_hover([this]() {
                if (decrease_button_ref)
                    decrease_button_ref->set_text("▼");
            });
            decrease_button.on_leave([this]() {
                if (decrease_button_ref)
                    decrease_button_ref->set_text("-");
            });

            // ── Pattern A：纯 rvalue 链式配置 ────────────────────────────────
            auto reset_button = button("Reset")
                .size(ButtonSize::sm)
                .font(nandina::text::NanFont{}
                    .weight(text::NanFontWeight::black)
                    .single_line(true)
                    .overflow(text::TextOverflow::scale)
                    )
                .variant(ButtonVariant::ghost)
                .on_click([this]() {
                    m_count = 1;
                });

            return mount(center(column(children(
                label_button,
                row(children(
                    increase_button,
                    decrease_button
                    )).gap(15),
                reset_button
                )).gap(10)));
        }

    private:
        nandina::app::Ref<nandina::widgets::Button> label_button_ref;
        nandina::app::Ref<nandina::widgets::Button> increase_button_ref;
        nandina::app::Ref<nandina::widgets::Button> decrease_button_ref;

        // Var<T>：声明即用，内部管理 State + ScopedConnection，无需 Pimpl 样板
        nandina::app::Var<int> m_count{1};
    };

} // namespace nandina::showcase
