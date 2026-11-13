//
// showcase/pages/surface_page.cpp — Surface 盒样式、Card 内容边距与嵌套边界。
//

#include "surface_page.hpp"

#include "../components/page_chrome.hpp"

#include <nandina/theme/design_system.hpp>
#include <nandina/widget/authoring.hpp>
#include <nandina/widget/card.hpp>
#include <nandina/widget/label.hpp>
#include <nandina/widget/layout.hpp>
#include <nandina/widget/surface.hpp>

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace nandina::showcase
{
    namespace
    {
        [[nodiscard]] auto card_body(
            const widget::BuildContext& ui,
            const PageChrome& chrome,
            const std::string_view text
        ) -> std::shared_ptr<scene::NanControl> {
            return ui.column()
                .width(widget::authoring::fill)
                .gap(chrome.item)
                .cross_alignment(widget::LayoutAlignment::start)
                .children(ui.make<widget::Label>(std::string(text))
                              .color_token(theme::ColorToken::foreground)
                              .font_size(chrome.supporting)
                              .leading(theme::Leading::normal)
                              .width(widget::authoring::fill)
                              .configure([](widget::Label& label) {
                                  label.set_overflow(text::TextOverflow::wrap);
                                  label.set_max_lines(3);
                              }))
                .build();
        }

        [[nodiscard]] auto inset_label(
            const widget::BuildContext& ui,
            const PageChrome& chrome,
            const std::string_view text
        ) -> widget::authoring::NodeBuilder<widget::Label> {
            return caption_text(ui, chrome, text);
        }

        /// 一张带固定内容边距的卡片；`insets` 为空时沿用 Card 配方默认值。
        [[nodiscard]] auto make_card(
            const widget::BuildContext& ui,
            const PageChrome& chrome,
            const std::string_view text,
            const foundation::NanInsets* insets,
            const std::string_view name
        ) -> std::shared_ptr<widget::Card> {
            auto body = card_body(ui, chrome, text);
            return ui.make<widget::Card>()
                .configure([body = std::move(body), insets, name](widget::Card& card) {
                    card.set_name(std::string(name));
                    if (insets != nullptr) {
                        card.set_content_insets(*insets);
                    }
                    card.set_child(body);
                })
                .width(widget::authoring::fill)
                .build();
        }

        [[nodiscard]] auto box_surface(
            const widget::BuildContext& ui,
            const PageChrome& chrome,
            const std::string_view text,
            const theme::ColorToken fill,
            const theme::ColorToken border,
            const float radius
        ) -> std::shared_ptr<widget::Surface> {
            auto body = ui.make<widget::Label>(std::string(text))
                            .color_token(theme::ColorToken::foreground)
                            .font_size(chrome.supporting)
                            .width(widget::authoring::fill)
                            .build();
            return ui.surface()
                .configure([fill, border, radius, chrome](widget::Surface& surface) {
                    surface.set_padding(foundation::NanInsets::all(chrome.group));
                    surface.set_box_style(
                        theme::BoxStyle {
                            .fill = theme::ThemeColor::token(fill),
                            .border = theme::ThemeColor::token(border),
                            .border_width =
                                theme::ThemeScalar::token(theme::ScalarToken::border_thin),
                            .radius = theme::ThemeScalar::literal(radius),
                        }
                    );
                })
                .width(widget::authoring::fill)
                .child(body)
                .build();
        }

        [[nodiscard]] auto intro_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);
            return ui.column()
                .width(widget::authoring::fill)
                .gap(chrome.block)
                .cross_alignment(widget::LayoutAlignment::stretch)
                .children(page_heading(ui, chrome, "表面与卡片（Surface / Card）"))
                .children(supporting_text(
                    ui,
                    chrome,
                    "P4.1b 的视觉表面原语：Surface 提供矩形填充/描边/圆角/内边距，Card 在配方"
                    "之上暴露四方内容边距。两者都只是布局与绘制；圆角不会裁剪子内容，"
                    "这条边界在最后一节如实展示。",
                    8
                ));
        }

        [[nodiscard]] auto insets_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);
            const foundation::NanInsets compact = foundation::NanInsets::all(8.0F);
            const foundation::NanInsets roomy = foundation::NanInsets::all(24.0F);

            auto variant = [&](const std::string_view caption,
                               const foundation::NanInsets* insets,
                               const std::string_view name,
                               const std::string_view body) {
                return ui.column()
                    .width(widget::authoring::fill)
                    .gap(chrome.tight)
                    .cross_alignment(widget::LayoutAlignment::stretch)
                    .children(inset_label(ui, chrome, caption))
                    .children(make_card(ui, chrome, body, insets, name));
            };

            return section_block(
                ui,
                chrome,
                "卡片内容边距",
                "同一内容的三张卡片：配方默认、显式 8 px、显式 24 px。显式值只在实例上覆盖，"
                "clear_content_insets() 会回到配方默认。",
                ui.column()
                    .width(widget::authoring::fill)
                    .gap(chrome.group)
                    .cross_alignment(widget::LayoutAlignment::stretch)
                    .children(variant(
                        "配方默认（CardRecipe.metrics_padding）",
                        nullptr,
                        "card-inset-default",
                        "内容边距来自 Card 配方；不同主题可以给出不同的默认内边距。"
                    ))
                    .children(
                        variant("显式 8 px", &compact, "card-inset-8", "紧凑信息卡常用这一档。")
                    )
                    .children(variant(
                        "显式 24 px",
                        &roomy,
                        "card-inset-24",
                        "内容优先的阅读卡常用这一档。"
                    ))
            );
        }

        [[nodiscard]] auto box_style_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);
            return section_block(
                ui,
                chrome,
                "Surface 盒样式",
                "`ui.surface()` 直接接受 BoxStyle：语义色 token 负责填充与描边，圆角与边宽可以"
                "用 token 或字面量。内边距来自 Padding 布局，与盒样式互不耦合。",
                ui.column()
                    .width(widget::authoring::fill)
                    .gap(chrome.group)
                    .cross_alignment(widget::LayoutAlignment::stretch)
                    .children(box_surface(
                        ui,
                        chrome,
                        "填充 surface：card 底色 + border 细描边 + 12 px 圆角",
                        theme::ColorToken::card,
                        theme::ColorToken::border,
                        12.0F
                    ))
                    .children(box_surface(
                        ui,
                        chrome,
                        "描边 surface：透明底 + accent 描边 + 直角（radius 0）",
                        theme::ColorToken::background,
                        theme::ColorToken::accent,
                        0.0F
                    ))
            );
        }

        [[nodiscard]] auto nested_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            auto accent_bar = box_surface(
                ui,
                chrome,
                "这块子 Surface 顶到卡片四边，用来观察圆角与子内容的交界。",
                theme::ColorToken::accent,
                theme::ColorToken::accent,
                0.0F
            );

            auto body =
                ui.column()
                    .width(widget::authoring::fill)
                    .gap(chrome.item)
                    .cross_alignment(widget::LayoutAlignment::stretch)
                    .children(supporting_text(
                        ui,
                        chrome,
                        "Card 的圆角只作用于自身背景，不裁剪子内容：贴边子节点不会被修成圆角。"
                        "这是已登记的渲染边界，页面如实展示，不用截图隐藏。",
                        4
                    ))
                    .children(accent_bar)
                    .build();

            auto nested = ui.make<widget::Card>()
                              .configure([body = std::move(body)](widget::Card& card) {
                                  card.set_name("card-nested");
                                  card.set_child(body);
                              })
                              .width(widget::authoring::fill)
                              .build();

            return section_block(
                ui,
                chrome,
                "嵌套与圆角边界",
                "外层 Card 的圆角与内层贴边 Surface 一起出现时，四角仍然是矩形裁剪的交集；"
                "卡片不会创建圆角遮罩，这是当前 clip 契约的已知限制。",
                nested
            );
        }
    } // namespace

    auto SurfacePage::build(app::PageContext& context) -> widget::View {
        const auto& ui = context.ui();
        const auto chrome = make_page_chrome(ui);

        auto content = ui.column()
                           .width(widget::authoring::fill)
                           .max_width(chrome.content_max)
                           .gap(chrome.block)
                           .cross_alignment(widget::LayoutAlignment::stretch);

        auto defer_section = [&context, &content, &ui](
                                 const float estimated_height,
                                 std::move_only_function<widget::View(app::PageContext&)> builder
                             ) -> void {
            auto placeholder =
                ui.column().width(widget::authoring::fill).height(estimated_height).build();
            const auto slot = context.defer_section(placeholder, std::move(builder));
            content.children(std::static_pointer_cast<scene::NanControl>(slot));
        };

        content.children(intro_section(context));
        defer_section(520.0F, [](app::PageContext& ctx) { return insets_section(ctx).build(); });
        defer_section(320.0F, [](app::PageContext& ctx) { return box_style_section(ctx).build(); });
        defer_section(340.0F, [](app::PageContext& ctx) { return nested_section(ctx).build(); });

        return ui.scroll_view()
            .width(widget::authoring::fill)
            .height(widget::authoring::fill)
            .child(ui.padding(foundation::NanInsets::all(chrome.page)).child(content))
            .build();
    }
} // namespace nandina::showcase
