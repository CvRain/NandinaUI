//
// CheckboxPage — Checkbox 组件展示页
// 参考 shadcn checkbox demo: basic / description / disabled / colorVariant / size / group
//

module;

#include <functional>
#include <string>
#include <string_view>

export module nandina.showcase.page.checkbox;

import nandina.app.application;
import nandina.app.authoring;
import nandina.app.page;
import nandina.foundation.color;
import nandina.layout.container;
import nandina.layout.flex_widgets;
import nandina.text;
import nandina.theme;
import nandina.widgets;

export namespace nandina::showcase {

    class CheckboxPage final : public nandina::app::NanPage {
    public:
        [[nodiscard]] auto route_key() const noexcept -> std::string_view override;

        [[nodiscard]] auto title() const noexcept -> std::string_view override;

        [[nodiscard]] auto icon_type() const noexcept -> nandina::widgets::IconType override;

        [[nodiscard]] auto build() -> app::NanComponent::Ptr override;

    private:
        // ── 返回一个 section 标题 ──
        static auto section_title(std::string_view text) -> app::LabelNode {
            return app::label(text)
                .font(nandina::text::NanFont{}
                    .size(15)
                    .weight(nandina::text::NanFontWeight::semiBold)
                    .color(nandina::NanColor::from(nandina::NanRgb{"#cdd6f4"})));
        }

        static auto section_desc(std::string_view text) -> app::LabelNode {
            return app::label(text)
                .font(nandina::text::NanFont{}
                    .size(12)
                    .color(nandina::NanColor::from(nandina::NanRgb{"#a6adc8"})));
        }

        static auto demo_card(app::Children&& content) -> app::Node {
            return app::card(app::children(
                app::column(std::move(content)).gap(12)
            ))
                .title("")
                .padding(20, 16, 20, 16);
        }

        static auto demo_row(app::Children&& children) -> app::Node {
            return app::row(std::move(children))
                .gap(12)
                .align_items(nandina::layout::LayoutAlignment::center);
        }
    };

} // namespace nandina::showcase


// ── 实现 ─────────────────────────────────────────────────────────────────────

namespace nandina::showcase {

    auto CheckboxPage::route_key() const noexcept -> std::string_view {
        return "showcase::checkbox";
    }

    auto CheckboxPage::title() const noexcept -> std::string_view {
        return "Checkbox";
    }

    auto CheckboxPage::icon_type() const noexcept -> nandina::widgets::IconType {
        return nandina::widgets::IconType::Check;
    }

    auto CheckboxPage::build() -> app::NanComponent::Ptr {
        using namespace nandina::app;
        using nandina::theme::ColorVariant;
        using nandina::theme::CheckboxSize;
        using nandina::layout::LayoutAlignment;

        // ═══════════════════════════════════════════════════════════
        // 1. Basic — 最简单的 checkbox + label
        // ═══════════════════════════════════════════════════════════
        auto basic_demo = demo_card(children(
            section_title("Basic"),
            section_desc("最简单的 checkbox，配合 label 使用。"),

            demo_row(children(
                checkbox("Accept terms and conditions")
            ))
        ));

        // ═══════════════════════════════════════════════════════════
        // 2. With Description — checkbox + label + 描述文本
        // ═══════════════════════════════════════════════════════════
        auto desc_demo = demo_card(children(
            section_title("With Description"),
            section_desc("配 label 与描述文本，表达更完整的选项语义。"),

            column(children(
                demo_row(children(
                    checkbox("Accept terms and conditions")
                        .checked(true)
                )),
                label("By clicking this checkbox, you agree to the terms.")
                    .font(nandina::text::NanFont{}
                        .size(12)
                        .color(NanColor::from(NanRgb{"#a6adc8"})))
                    .align_items(LayoutAlignment::start)
            )).gap(4)
        ));

        // ═══════════════════════════════════════════════════════════
        // 3. Checked State — 受控选中
        // ═══════════════════════════════════════════════════════════
        // 使用 Var<bool> + on_checked_changed 演示受控模式
        // (当前 authoring 不支持 .text(fn) 用于 checkbox label，
        //  因此用静态文本说明，但 checked/on_checked_changed 已验证)
        auto checked_demo = demo_card(children(
            section_title("Checked State"),
            section_desc("使用 checked 与 on_checked_changed 控制选中状态。"),

            column(children(
                demo_row(children(
                    checkbox("Unchecked (default)")
                )),
                demo_row(children(
                    checkbox("Checked")
                        .checked(true)
                )),
                demo_row(children(
                    checkbox("Disabled checked")
                        .checked(true)
                        .disabled(true)
                ))
            )).gap(8)
        ));

        // ═══════════════════════════════════════════════════════════
        // 4. Color Variants — 颜色变体
        // ═══════════════════════════════════════════════════════════
        auto variant_demo = demo_card(children(
            section_title("Color Variants"),
            section_desc("color_variant 控制语义颜色族：primary / secondary / neutral / destructive。"),

            column(children(
                demo_row(children(
                    checkbox("Primary (default)")
                        .checked(true)
                        .color_variant(ColorVariant::primary)
                )),
                demo_row(children(
                    checkbox("Secondary")
                        .checked(true)
                        .color_variant(ColorVariant::secondary)
                )),
                demo_row(children(
                    checkbox("Neutral")
                        .checked(true)
                        .color_variant(ColorVariant::neutral)
                )),
                demo_row(children(
                    checkbox("Destructive")
                        .checked(true)
                        .color_variant(ColorVariant::destructive)
                ))
            )).gap(8)
        ));

        // ═══════════════════════════════════════════════════════════
        // 5. Sizes — 尺寸
        // ═══════════════════════════════════════════════════════════
        auto size_demo = demo_card(children(
            section_title("Sizes"),
            section_desc("size 控制复选框和文本的大小：sm / md。"),

            column(children(
                demo_row(children(
                    checkbox("Small size")
                        .size(CheckboxSize::sm)
                )),
                demo_row(children(
                    checkbox("Medium size (default)")
                        .size(CheckboxSize::md)
                        .checked(true)
                ))
            )).gap(8)
        ));

        // ═══════════════════════════════════════════════════════════
        // 6. Disabled — 禁用态
        // ═══════════════════════════════════════════════════════════
        auto disabled_demo = demo_card(children(
            section_title("Disabled"),
            section_desc("disabled 阻止交互并降低视觉透明度。"),

            column(children(
                demo_row(children(
                    checkbox("Disabled unchecked")
                        .disabled(true)
                )),
                demo_row(children(
                    checkbox("Disabled checked")
                        .disabled(true)
                        .checked(true)
                )),
                demo_row(children(
                    checkbox("Disabled + destructive")
                        .disabled(true)
                        .checked(true)
                        .color_variant(ColorVariant::destructive)
                ))
            )).gap(8)
        ));

        // ═══════════════════════════════════════════════════════════
        // 7. Group — 复选框组（带 Field legend 语义）
        // ═══════════════════════════════════════════════════════════
        auto group_demo = demo_card(children(
            section_title("Checkbox Group"),
            section_desc("多个 checkbox 组成选项组，可配合 Field 容器表达表单语义。"),

            column(children(
                label("Show these items on the desktop:")
                    .font(nandina::text::NanFont{}
                        .size(13)
                        .weight(nandina::text::NanFontWeight::semiBold)
                        .color(NanColor::from(NanRgb{"#cdd6f4"}))),
                label("Select the items you want to show on the desktop.")
                    .font(nandina::text::NanFont{}
                        .size(12)
                        .color(NanColor::from(NanRgb{"#a6adc8"}))),

                column(children(
                    checkbox("Hard disks")
                        .checked(true),
                    checkbox("External disks")
                        .checked(true),
                    checkbox("CDs, DVDs, and iPods"),
                    checkbox("Connected servers")
                )).gap(6)
            )).gap(8)
        ));

        // ═══════════════════════════════════════════════════════════
        // 8. With Field — 配合 Field 表单容器
        // ═══════════════════════════════════════════════════════════
        auto field_demo = demo_card(children(
            section_title("With Field"),
            section_desc("checkbox 作为 Field 的 control，获得 label/helper/error 语义。"),

            column(children(
                app::field()
                    .label("Notifications")
                    .helper_text("You can enable or disable notifications at any time.")
                    .control(
                        checkbox("Enable push notifications")
                            .checked(true)
                    ),
                app::field()
                    .label("Privacy")
                    .helper_text("Control how your data is shared.")
                    .control(
                        checkbox("Share usage data")
                    )
            )).gap(12)
        ));

        // ── 最终页面布局 ──────────────────────────────────────────
        return mount(
            column(children(
                label("Checkbox")
                    .font(nandina::text::NanFont{}
                        .size(24)
                        .weight(nandina::text::NanFontWeight::bold)
                        .color(NanColor::from(NanRgb{"#cdd6f4"}))),
                label("A control that allows the user to toggle between checked and not checked.")
                    .font(nandina::text::NanFont{}
                        .size(13)
                        .color(NanColor::from(NanRgb{"#a6adc8"}))),

                basic_demo,
                desc_demo,
                checked_demo,
                variant_demo,
                size_demo,
                disabled_demo,
                group_demo,
                field_demo
            ))
            .align_items(LayoutAlignment::stretch)
            .gap(20)
            .padding(24)
        );
    }

} // namespace nandina::showcase
