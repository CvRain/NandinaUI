#include "sidebar.hpp"

#include <nandina/app/nan_router.hpp>
#include <nandina/foundation/nan_logger.hpp>
#include <nandina/widget/controls.hpp>

#include <string>

namespace nandina::showcase
{
    namespace
    {
        /// 侧边栏按钮文字：优先 `title`，其次显示用地址文字 `key`，都没有时给占位符。
        [[nodiscard]] auto nav_label(const app::RouteEntry& entry) -> std::string {
            if (!entry.options.title.empty()) {
                return entry.options.title;
            }
            if (!entry.options.address.empty()) {
                return entry.options.address;
            }
            return "untitled";
        }
    } // namespace

    Sidebar::Sidebar(const app::ShellContext& context): context(context) {}

    auto Sidebar::build_shell() -> widget::View {
        const auto ui = context.ui();
        const auto navigation = context.navigation();

        // 导航项直接从路由表生成：`nav_entries()` 已经把「显示在导航里」和
        // 「点一下就进得去」两条规则收好了，这里不必再写 `if (key == …) navigate<>()`
        // 链，也不会漏掉 `activate` 判空而留下点了没反应的死条目。
        auto navigation_buttons = ui.column().gap(4.0F).width(widget::authoring::fill);
        for (const auto* entry: context.routes().nav_entries()) {
            const auto page_key = entry->page_key;
            navigation_buttons.get().add(
                ui.make<widget::Button>(nav_label(*entry))
                    .width(widget::authoring::fill)
                    .height(24)
                    .on_click([navigation, page_key] {
                        // 运行时页面键的失败是预期路径（路由被移除、窗口正在关闭），
                        // 所以在这里就地处理，而不是让它变成一个静默的空点击。
                        if (const auto entered = navigation.navigate_to(page_key); !entered) {
                            log::error(
                                "showcase sidebar: cannot enter route: {}",
                                describe(entered.error())
                            );
                        }
                    })
                    .build()
            );
        }

        auto sidebar_content = ui.make<widget::Card>()
                                   .width(scene::percent(100))
                                   .height(scene::percent(100))
                                   .child(navigation_buttons);

        auto sidebar = ui.padding(foundation::NanInsets::all(12.0F))
                           .min_width(220.0F)
                           .max_width(320.0F)
                           .width(scene::percent(22))
                           .height(widget::authoring::fill)
                           .child(sidebar_content);

        return ui.row()
            .gap(8.0F)
            .cross_alignment(widget::LayoutAlignment::stretch)
            .width(widget::authoring::fill)
            .height(widget::authoring::fill)
            .children(std::move(sidebar), ui.expanded().child(context.outlet()))
            .build();
    }
} // namespace nandina::showcase