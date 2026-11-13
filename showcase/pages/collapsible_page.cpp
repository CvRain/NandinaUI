//
// showcase/pages/collapsible_page.cpp — 折叠内容的保留状态、嵌套与 Signal 同步。
//

#include "collapsible_page.hpp"

#include "../components/page_chrome.hpp"

#include <nandina/widget/authoring.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/collapsible.hpp>
#include <nandina/widget/label.hpp>
#include <nandina/widget/layout.hpp>

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace nandina::showcase
{
    namespace
    {
        /// 折叠内容：收起时节点仍挂在 Collapsible 的 body 上，展开不会重建。
        [[nodiscard]] auto collapsible_body(
            const widget::BuildContext& ui,
            const PageChrome& chrome,
            const std::string_view text
        ) -> std::shared_ptr<scene::NanControl> {
            return ui.column()
                .width(widget::authoring::fill)
                .gap(chrome.item)
                .cross_alignment(widget::LayoutAlignment::start)
                .children(ui.make<widget::Label>(std::string(text))
                              .color_token(theme::ColorToken::muted_foreground)
                              .font_size(chrome.supporting)
                              .leading(theme::Leading::normal)
                              .width(widget::authoring::fill)
                              .configure([](widget::Label& label) {
                                  label.set_overflow(text::TextOverflow::wrap);
                                  label.set_max_lines(4);
                              }))
                .children(ui.make<widget::Button>("内容内的按钮")
                              .treatment(theme::ButtonTreatment::outlined))
                .build();
        }

        [[nodiscard]] auto make_collapsible(
            const widget::BuildContext& ui,
            const PageChrome& chrome,
            const std::string_view title,
            std::shared_ptr<scene::NanControl> body,
            const bool open,
            const bool disabled,
            const std::string_view name
        ) -> widget::authoring::NodeBuilder<widget::Collapsible> {
            return ui.make<widget::Collapsible>(std::string(title))
                .configure([body = std::move(body), open, disabled, name, gap = chrome.item](
                               widget::Collapsible& node
                           ) {
                    node.set_name(std::string(name));
                    node.set_gap(gap);
                    node.set_child(body);
                    node.set_open(open);
                    node.set_disabled(disabled);
                })
                .width(widget::authoring::fill);
        }

        [[nodiscard]] auto intro_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);
            return ui.column()
                .width(widget::authoring::fill)
                .gap(chrome.block)
                .cross_alignment(widget::LayoutAlignment::stretch)
                .children(page_heading(ui, chrome, "折叠内容（Collapsible）"))
                .children(supporting_text(
                    ui,
                    chrome,
                    "P4.2c 的折叠组件由 Button 触发，内容槽位在收起时仍然保留：再次展开不会重建"
                    "子节点，焦点与滚动位置也不会丢。程序化 set_open 不触发用户回调，只有用户"
                    "激活才发出 Signal。",
                    8
                ));
        }

        [[nodiscard]] auto states_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            auto closed = ui.column()
                              .width(widget::authoring::fill)
                              .gap(chrome.item)
                              .cross_alignment(widget::LayoutAlignment::start)
                              .children(caption_text(ui, chrome, "默认收起"))
                              .children(make_collapsible(
                                  ui,
                                  chrome,
                                  "默认收起（点击展开）",
                                  collapsible_body(
                                      ui,
                                      chrome,
                                      "这段内容在收起时仍保留在组件树中，展开不会重建。"
                                  ),
                                  false,
                                  false,
                                  "collapsible-closed"
                              ));

            auto opened = ui.column()
                              .width(widget::authoring::fill)
                              .gap(chrome.item)
                              .cross_alignment(widget::LayoutAlignment::start)
                              .children(caption_text(ui, chrome, "初始展开（set_open）"))
                              .children(make_collapsible(
                                  ui,
                                  chrome,
                                  "初始展开",
                                  collapsible_body(
                                      ui,
                                      chrome,
                                      "程序化 set_open(true) 静默生效：不会触发 on_change，也不会"
                                      "伪造一次用户切换。"
                                  ),
                                  true,
                                  false,
                                  "collapsible-open"
                              ));

            return section_block(
                ui,
                chrome,
                "收起与展开",
                "同一组件的两个实例：左边保持收起，右边在构建时通过 set_open(true) 展开；"
                "两者都保留内容子树。",
                ui.row()
                    .width(widget::authoring::fill)
                    .gap(chrome.block)
                    .cross_alignment(widget::LayoutAlignment::stretch)
                    .children(ui.expanded().child(closed))
                    .children(ui.expanded().child(opened))
            );
        }

        [[nodiscard]] auto nested_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            auto inner = make_collapsible(
                ui,
                chrome,
                "内层：二级折叠",
                collapsible_body(
                    ui,
                    chrome,
                    "内层内容由外层的内容槽位持有，收起外层时整棵子树一起隐藏。"
                ),
                false,
                false,
                "collapsible-inner"
            );

            auto outer_body = ui.column()
                                  .width(widget::authoring::fill)
                                  .gap(chrome.group)
                                  .cross_alignment(widget::LayoutAlignment::start)
                                  .children(supporting_text(
                                      ui,
                                      chrome,
                                      "外层内容是 Column：一段说明加一个内层 Collapsible。"
                                      "嵌套不引入第二套展开状态，每层各自维护 open。",
                                      4
                                  ))
                                  .children(inner)
                                  .build();

            auto outer = make_collapsible(
                ui,
                chrome,
                "外层：包含嵌套折叠",
                std::move(outer_body),
                true,
                false,
                "collapsible-outer"
            );

            return section_block(
                ui,
                chrome,
                "嵌套",
                "Collapsible 的内容槽位接受任意控件；这里嵌入第二个 Collapsible，"
                "验证展开状态互相独立、外层收起时内层保留状态。",
                outer
            );
        }

        [[nodiscard]] auto signal_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            auto& open = ui.signal_value(false);
            auto& status = ui.computed([&open]() -> std::string {
                return open.get() ? "状态：展开（Signal 已收到）" : "状态：收起";
            });

            auto trigger_body = collapsible_body(
                ui,
                chrome,
                "用户激活后，on_change 把新状态写入 Signal，右侧文本随之更新；"
                "程序化 set_open 不经过这条路径。"
            );

            const auto trigger =
                ui.make<widget::Collapsible>("用户切换会更新右侧状态")
                    .configure([body = std::move(trigger_body),
                                &open,
                                gap = chrome.item](widget::Collapsible& node) {
                        node.set_name("collapsible-signal");
                        node.set_gap(gap);
                        node.set_child(body);
                        node.set_on_change([&open](bool value) { open.set(value); });
                    })
                    .width(widget::authoring::fill);

            const auto disabled = ui.make<widget::Collapsible>("禁用状态不可展开")
                                      .configure([gap = chrome.item](widget::Collapsible& node) {
                                          node.set_name("collapsible-disabled");
                                          node.set_gap(gap);
                                          node.set_disabled(true);
                                      })
                                      .width(widget::authoring::fill);

            return section_block(
                ui,
                chrome,
                "Signal 与禁用",
                "用户切换触发 on_change → Signal → 右侧状态文本；程序化 set_open 不会。"
                "禁用的 Collapsible 保留内容但不响应激活。",
                ui.column()
                    .width(widget::authoring::fill)
                    .gap(chrome.group)
                    .cross_alignment(widget::LayoutAlignment::stretch)
                    .children(ui.row()
                                  .width(widget::authoring::fill)
                                  .gap(chrome.block)
                                  .cross_alignment(widget::LayoutAlignment::center)
                                  .children(ui.expanded().child(trigger))
                                  .children(ui.make<widget::Label>(status)
                                                .color_token(theme::ColorToken::muted_foreground)
                                                .font_size(chrome.supporting)))
                    .children(disabled)
            );
        }
    } // namespace

    auto CollapsiblePage::build(app::PageContext& context) -> widget::View {
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
        defer_section(420.0F, [](app::PageContext& ctx) { return states_section(ctx).build(); });
        defer_section(480.0F, [](app::PageContext& ctx) { return nested_section(ctx).build(); });
        defer_section(420.0F, [](app::PageContext& ctx) { return signal_section(ctx).build(); });

        return ui.scroll_view()
            .width(widget::authoring::fill)
            .height(widget::authoring::fill)
            .child(ui.padding(foundation::NanInsets::all(chrome.page)).child(content))
            .build();
    }
} // namespace nandina::showcase
