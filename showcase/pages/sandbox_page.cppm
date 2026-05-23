module;

#include <format>
#include <string>

export module nandina.showcase.sandbox_page;

import nandina.app.authoring;   // re-exports: nandina.app.var, nandina.app.computed_var, nandina.reactive
import nandina.foundation.color;
import nandina.layout.container;
import nandina.layout.flex_widgets;
import nandina.widgets;
import nandina.theme;

export namespace nandina::showcase {

    /**
     * SandboxPage — 组件验证沙盒（新响应式 API 演示）
     *
     * 演示新响应式 API：
     *   A. text(fn)        — 自动 Effect 追踪，替代 bind_text 两步绑定
     *   B. update(fn)      — 原地修改 [](int& v){ v++; } 与值语义 [](int){ return 1; }
     *   C. ComputedVar<T>  — 页面成员级只读派生 signal（完整参与 Effect 依赖图）
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

            // ── Pattern A：text(fn) 自动 Effect 追踪 ─────────────────────────
            auto label_button = button()
                .bind(label_button_ref)
                .size({240, 60})
                .variant(ButtonVariant::outline)
                .text([this]{ return std::to_string(m_count()); });

            // ── Pattern C：ComputedVar<T> 派生 signal 参与追踪图 ─────────────
            auto double_label = button()
                .size({240, 60})
                .variant(ButtonVariant::outline)
                .text([this]{ return std::format("x2 = {}", m_double()); });

            // ── Pattern B：update() 原地修改 ─────────────────────────────────
            auto increase_button = button()
                .bind(increase_button_ref)
                .size({120, 60})
                .text("+");

            increase_button.on_click([this]() {
                m_count.update([](int& v){ v++; });
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
                m_count.update([](int& v){ v--; });
            });
            decrease_button.on_hover([this]() {
                if (decrease_button_ref)
                    decrease_button_ref->set_text("▼");
            });
            decrease_button.on_leave([this]() {
                if (decrease_button_ref)
                    decrease_button_ref->set_text("-");
            });

            // ── Pattern B：update() 值语义重置 ──────────────────────────────
            auto reset_button = button()
                .size(ButtonSize::sm)
                .variant(ButtonVariant::ghost)
                .text("reset")
                .on_click([this]() {
                    m_count.update([](int){ return 1; });
                });

            return mount(center(column(children(
                label_button,
                double_label,
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

        // Var<T>：可变 signal，update() 支持原地修改与值语义两种风格
        nandina::app::Var<int> m_count{1};

        // ComputedVar<T>：只读派生 signal，依赖图自动建立
        nandina::app::ComputedVar<int> m_double{[this]{ return m_count() * 2; }};
    };

} // namespace nandina::showcase
