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
     * SandboxPage — 组件验证沙盒（typed builder 演示）
     *
     * 演示三种用法：
     *   A. 一口气链式配置（rvalue chain）
     *   B. 分段 lvalue 配置（先 auto btn = ...; 再 btn.on_click(...);）
     *   C. 跨 widget 回调引用（Ref<Button> 类成员 + bind()）
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

            // ── 演示 A+C: 链式配置 + bind 成员 Ref ──────────
            // primary_btn 本身用一口气 rvalue 链构建；
            // .bind(m_primary_ref) 注册绑定，mount 后可从其他回调访问底层 Button
            auto primary_btn = button("测试按钮")
                .bind(m_primary_ref)
                .font(
                    nandina::text::NanFont{}
                    .color(NanColor::from(NanRgb{"#e64553"}))
                    .weight(text::NanFontWeight::black)
                    .letter_spacing(1.0f)
                    .size(18)
                    )
                .width(300)
                .height(150);

            // ── 演示 B: lvalue 分段配置 ───────────────────
            // 现在 primary_btn 是 lvalue ButtonNode，可以继续调用任意方法
            primary_btn.on_click([] {
                std::print("primary clicked!\n");
            });
            primary_btn.on_hover([] {
                std::print("primary hovered!\n");
            });
            primary_btn.on_leave([] {
                std::print("primary leave!\n");
            });

            // ── 演示 2: NanFont 统一配置 ──────────────────
            auto heading_font = nandina::text::NanFont{}
                .size(24)
                .weight(nandina::text::NanFontWeight::extraBold)
                .color(NanColor::from(NanRgb{"#e64553"}))
                .letter_spacing(1.0f)
                .single_line(true);

            // ── 演示 C: 跨 widget 引用 ────────────────────
            // destructive 按钮点击时，通过 m_primary_ref 直接修改 primary_btn 的底层 Label
            // m_primary_ref 在 mount() 之后由 MountedNodeComponent::bind_refs() 设置好
            auto styled_btn = button("Destructive")
                .variant(nandina::theme::ButtonVariant::destructive)
                .size(nandina::theme::ButtonSize::lg)
                .width(150).height(75)
                .on_click([this] {
                    std::print("destructive clicked!\n");
                    if (m_primary_ref) {
                        m_primary_ref->set_text("被改了!");
                    }
                });

            // ── 演示 4: outline 变体 ──────────────────────
            auto outline_btn = button("Outline")
                .variant(nandina::theme::ButtonVariant::outline)
                .size(nandina::theme::ButtonSize::md)
                .on_click([] {
                    std::print("outline clicked!\n");
                });

            auto interactive_btn = button("Hover me")
                .on_click([]() {
                    std::print("test button clicked!\n");
                })
                .on_hover([]() {
                    std::print("test button hovered!\n");
                })
                .on_leave([]() {
                    std::print("test button leave!\n");
                })
                .on_release([]() {
                    std::print("test button released!\n");
                });

            // ── 布局（children 无需 std::move；.width() 自动包裹为 SizedBox）──
            auto btn_row = row(children(
                    primary_btn,
                    sized_box(spacer()).width(20),
                    styled_btn,
                    sized_box(spacer()).width(20),
                    outline_btn,
                    sized_box(spacer()).width(20),
                    interactive_btn))
                .gap(12);

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

    private:
        // 演示 C: 跨 widget 回调引用——必须作为类成员而非 build() 局部变量
        // mount() 时 MountedNodeComponent::bind_refs() 会将底层 Button* 填入
        nandina::app::Ref<nandina::widgets::Button> m_primary_ref;
    };

} // namespace nandina::showcase
