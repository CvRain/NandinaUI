module;

#include <memory>
#include <string_view>
#include <print>

export module nandina.showcase.sandbox_page;

import nandina.app.authoring;   // re-exports nandina.reactive
import nandina.foundation.color;
import nandina.layout.container;
import nandina.layout.flex_widgets;
import nandina.widgets;
import nandina.theme;

// ── 模块私有：响应式数据（GCC 无法序列化 State/ScopedConnection，放匿名 namespace）──
namespace {
    struct SandboxReactive {
        nandina::reactive::State<int> number{1};
        nandina::reactive::ScopedConnection number_conn;
    };
} // namespace

export namespace nandina::showcase {

    /**
     * SandboxPage — 组件验证沙盒（typed builder 演示）
     *
     * 演示三种 authoring 模式：
     *   A. 纯 rvalue 链式配置（所有属性和回调在单个表达式内完成，无中间变量）
     *   B. lvalue 分段配置（先构建节点，再对 lvalue 补充 on_click/on_hover/on_leave）
     *   C. 跨 widget 回调引用（Ref<Button> 类成员 + .bind()，在别处回调中访问节点）
     *
     * 同时演示 StateSlot<T> + bind_text() 响应式绑定（无需 Pimpl / anonymous namespace）。
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

            // 懒初始化响应式数据（build() 可能被多次调用，只创建一次）
            if (!m_reactive) {
                m_reactive = std::make_shared<SandboxReactive>();
            }
            auto* r = static_cast<SandboxReactive*>(m_reactive.get());

            // ── 响应式绑定：bind_text 一行代替手写 ScopedConnection + on_change lambda ──
            // bind_text 在 number 变化时自动调用 label_button_ref->set_text(...)，
            // ScopedConnection 存储在 Pimpl struct 中，随 SandboxReactive 自动销毁。
            r->number_conn = bind_text(r->number, label_button_ref,
                [](int v) { return std::to_string(v); });

            // ── Pattern C：Ref<Button> 成员 + .bind() ─────────────────────────
            // label_button 绑定到成员 Ref；bind_text 回调可通过 Ref 直接更新文本，
            // 不需要持有节点变量本身。
            auto label_button = button()
                .bind(label_button_ref)
                .size({240, 60})
                .variant(ButtonVariant::outline)
                .text(std::to_string(r->number()));

            // ── Pattern B：lvalue 分段配置 ────────────────────────────────────
            // 先用链式配置锁定布局与外观，再对同一 lvalue 逐步挂载事件回调。
            // on_hover / on_leave 通过 Ref 访问节点，在悬停时临时改变文字。
            auto increase_button = button()
                .bind(increase_button_ref)
                .size({120, 60})
                .text("+");

            increase_button.on_click([r]() {
                std::print("increase clicked!\n");
                r->number.set(r->number() + 1);
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

            decrease_button.on_click([r]() {
                std::print("decrease clicked!\n");
                r->number.set(r->number() - 1);
            });
            decrease_button.on_hover([this]() {
                if (decrease_button_ref)
                    decrease_button_ref->set_text("▼");
            });
            decrease_button.on_leave([this]() {
                if (decrease_button_ref)
                    decrease_button_ref->set_text("-");
            });

            // ── Pattern A：纯 rvalue 链式配置（一口气构建完毕）──────────────────
            // 所有属性与回调在单个链式表达式内完成，无任何中间变量。
            auto reset_button = button("Reset")
                .size(ButtonSize::sm)
                .font(nandina::text::NanFont{}
                    .weight(text::NanFontWeight::black)
                    .single_line(true)
                    .overflow(text::TextOverflow::scale)
                    )
                .variant(ButtonVariant::ghost)
                .on_click([r]() {
                    std::print("reset clicked!\n");
                    r->number.set(1);
                });

            auto page = center(column(children(
                label_button,
                row(children(
                    increase_button,
                    decrease_button
                    )).gap(15),
                reset_button
                )).gap(10));

            return mount(std::move(page));
        }

    private:
        nandina::app::Ref<nandina::widgets::Button> label_button_ref;
        nandina::app::Ref<nandina::widgets::Button> increase_button_ref;
        nandina::app::Ref<nandina::widgets::Button> decrease_button_ref;

        // Pimpl：State/ScopedConnection 含不可序列化类型，GCC 模块需用 shared_ptr<void> 类型擦除
        std::shared_ptr<void> m_reactive;
    };

} // namespace nandina::showcase
