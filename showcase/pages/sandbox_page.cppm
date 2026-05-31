module;

#include <format>
#include <functional>
#include <string>
#include <random>

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
            using nandina::layout::LayoutAlignment;

            // ── Pattern A：text(fn) 自动 Effect 追踪 ─────────────────────────
            // 不设固定尺寸，由 column align_items(stretch) 撑满列宽
            auto label = app::label()
                .bind(label_ref)
                .vertical_align(widgets::TextVerticalAlign::Center)
                .align_items(LayoutAlignment::center)
                .align(widgets::TextAlign::Center)
                .font(text::NanFont{}
                    .color(NanColor::from(NanRgb{"#dd7878"}))
                    .overflow(text::TextOverflow::scale)
                    .size(23)
                    .weight(text::NanFontWeight::bold))
                .text([this] {
                    return std::to_string(m_count());
                });

            // ── Pattern C：ComputedVar<T> 派生 signal 参与追踪图 ─────────────
            auto double_label = button()
                .variant(ButtonVariant::outline)
                .text([this] {
                    return std::format("x2 = {}", m_double());
                });

            // ── Pattern B：update() 原地修改 ─────────────────────────────────
            // expanded() 包裹后在 row 中各占 50% 宽度
            auto increase_button = button()
                .bind(increase_button_ref)
                .text("+");

            increase_button.on_click([this]() {
                m_count.update([](int& v) {
                    v++;
                });
            });
            increase_button.on_hover([this]() {
                if (increase_button_ref)
                    increase_button_ref->text("▲");
            });
            increase_button.on_leave([this]() {
                if (increase_button_ref)
                    increase_button_ref->text("+");
            });

            auto decrease_button = button()
                .bind(decrease_button_ref)
                .text("-");

            decrease_button.on_click([&]() {
                m_count.update([](int& v) {
                    v--;
                });
                label_ref->update_font([this](text::NanFont& font) {
                    //generate random number 0 ~ 255
                    std::random_device random_device;
                    std::mt19937 generator(random_device());
                    std::uniform_int_distribution<uint8_t> distribution(1, 255);

                    // generate random green color
                    const auto green = distribution(generator);
                    const auto red   = distribution(generator);
                    const auto blue  = distribution(generator);


                    font.color(NanColor::from(NanRgb{red, green, blue}));
                });
            });

            decrease_button.on_hover([this]() {
                if (decrease_button_ref)
                    decrease_button_ref->text("▼");
            });
            decrease_button.on_leave([this]() {
                if (decrease_button_ref)
                    decrease_button_ref->text("-");
            });

            // ── Pattern B：update() 值语义重置 ──────────────────────────────
            // ghost sm 按钮保持自然宽度，通过两侧 spacer() 居中
            auto reset_button = button()
                .size(ButtonSize::sm)
                .variant(ButtonVariant::ghost)
                .text("reset")
                .on_click([this]() {
                    m_count.update([](int) {
                        return 1;
                    });
                });

            auto text_field = app::text_field()
                .placeholder("Type into TextField...")
                .on_change([this](std::string_view value) {
                    if (text_field_status_ref) {
                        text_field_status_ref->set_text(value.empty()
                            ? "Input = (empty)"
                            : std::format("Input = {}", value));
                    }
                })
                .on_submit([this](std::string_view value) {
                    if (text_field_status_ref) {
                        text_field_status_ref->set_text(value.empty()
                            ? "Submitted empty input"
                            : std::format("Submitted = {}", value));
                    }
                });

            auto text_field_status = app::label("Input = (empty)")
                .bind(text_field_status_ref)
                .font(text::NanFont{}
                    .size(13)
                    .color(NanColor::from(NanRgb{"#7c7f93"}))
                    .single_line(true)
                    .overflow(text::TextOverflow::clip));

            // ── Field 演示 ──────────────────────────────────
            auto field = app::field()
                .label("Email")
                .helper_text("We'll never share your email.")
                .error_text("Invalid email address.")
                .control(app::text_field().placeholder("Enter email"))
                .bind(field_ref);

            auto toggle_invalid_btn = button()
                .size(ButtonSize::sm)
                .variant(ButtonVariant::outline)
                .text([this] {
                    return m_field_invalid ? "✓ Valid" : "✗ Invalid";
                });

            toggle_invalid_btn.on_click([this]() {
                m_field_invalid = !m_field_invalid;
                if (auto* f = field_ref.get()) {
                    f->set_invalid(m_field_invalid);
                    if (auto* ctrl = dynamic_cast<nandina::widgets::TextField*>(f->control())) {
                        if (m_field_invalid) {
                            ctrl->set_value("");
                        } else {
                            ctrl->set_value("user@example.com");
                        }
                    }
                }
            });

            return mount(center(
                column(children(
                    label,
                    double_label,
                    text_field.width(280.0f),
                    text_field_status,
                    field.width(280.0f),
                    toggle_invalid_btn,
                    row(children(
                        expanded(increase_button),
                        expanded(decrease_button)
                        )).gap(12),
                    row(children(
                        spacer(),
                        reset_button,
                        spacer()
                        ))
                    ))
                .gap(12)
                .align_items(LayoutAlignment::stretch)
                .width(360.0f)
                ));
        }

    private:
        nandina::app::Ref<nandina::widgets::Button> increase_button_ref;
        nandina::app::Ref<nandina::widgets::Button> decrease_button_ref;
        nandina::app::Ref<widgets::Label> label_ref;
        nandina::app::Ref<widgets::Label> text_field_status_ref;
        nandina::app::Ref<nandina::widgets::Field> field_ref;

        // Var<T>：可变 signal，update() 支持原地修改与值语义两种风格
        nandina::app::Var<int> m_count{1};

        bool m_field_invalid{false};

        // ComputedVar<T>：只读派生 signal，依赖图自动建立
        nandina::app::ComputedVar<int> m_double{[this] {
            return m_count() * 2;
        }};
    };

} // namespace nandina::showcase
