// showcase/pages/input_focus_page.cpp — D1 第一阶段（焦点提示策略）的人工验收页。
//
// 页面刻意不模拟 scene 状态：鼠标点击、Tab/方向键、Escape 和文本框输入都走真实事件路径。
// 观察点：
//   1. 普通按钮：键盘进入/导航时出现提示环，鼠标点击后隐藏；
//   2. 可编辑与只读文本字段：指针聚焦也保留编辑提示；
//   3. “始终显示”偏好：只覆盖活动窗口内的模态隐藏；
//   4. RadioGroup 成员持有真实焦点；Tabs 与下拉菜单由容器持有焦点、内部活动项漫游；
//   5. 对话框通过 Escape 关闭后，触发按钮恢复焦点并显示键盘提示。

#include "input_focus_page.hpp"

#include <nandina/widget/button.hpp>
#include <nandina/widget/checkbox.hpp>
#include <nandina/widget/dialog.hpp>
#include <nandina/widget/dropdown_menu.hpp>
#include <nandina/widget/label.hpp>
#include <nandina/widget/menu_item.hpp>
#include <nandina/widget/radio_button.hpp>
#include <nandina/widget/radio_group.hpp>
#include <nandina/widget/tabs.hpp>
#include <nandina/widget/text_field.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace nandina::showcase
{
    namespace
    {
        auto section_title(const widget::BuildContext& ui, const std::string& text) {
            return ui.make<widget::Label>(text)
                .color_token(theme::ColorToken::foreground)
                .font_size(20.0F)
                .width(widget::authoring::fill)
                .build();
        }

        auto hint_label(const widget::BuildContext& ui, const std::string& text) {
            return ui.make<widget::Label>(text)
                .color_token(theme::ColorToken::muted_foreground)
                .font_size(14.0F)
                .width(widget::authoring::fill)
                .configure([](widget::Label& label) {
                    label.set_overflow(text::TextOverflow::wrap);
                    label.set_max_lines(4);
                })
                .build();
        }

        auto menu_items() -> std::vector<widget::MenuItem> {
            return {
                widget::MenuItem {.id = "rename", .label = "重命名", .shortcut = "F2"},
                widget::MenuItem {.id = "duplicate", .label = "复制"},
                widget::MenuItem {.id = "separator", .kind = widget::MenuItemKind::separator},
                widget::MenuItem {.id = "delete", .label = "删除", .shortcut = "Del"},
            };
        }
    } // namespace

    auto InputFocusPage::build(app::PageContext& context) -> widget::View {
        const auto& ui = context.ui();

        // 偏好开关需要一个已入树节点来取得 scene tree。
        const auto preference_anchor = ui.make<widget::Label>("").build();

        auto preference = ui.make<widget::Checkbox>("始终显示焦点提示")
                              .configure([](widget::Checkbox& checkbox) {
                                  checkbox.set_name("focus-preference");
                              })
                              .on_change([anchor = preference_anchor](const bool enabled) {
                                  if (auto* tree = anchor->get_tree()) {
                                      tree->set_focus_hint_always_visible(enabled);
                                  }
                              })
                              .build();

        const auto hint_text = hint_label(
            ui,
            "按 Tab / 方向键导航，或点击任意控件，观察提示环的变化。可编辑字段在指针聚焦时"
            "仍保留编辑提示；普通按钮在指针点击后隐藏提示环。"
        );

        const auto interactive_title = section_title(ui, "模态与提示");
        const auto buttons = ui.row()
                                 .gap(12.0F)
                                 .cross_alignment(widget::LayoutAlignment::center)
                                 .children(ui.make<widget::Button>("按钮 A").build())
                                 .children(ui.make<widget::Button>("按钮 B").build())
                                 .children(preference)
                                 .build();

        auto editable = ui.make<widget::TextField>("", "可编辑字段").build();
        auto read_only = ui.make<widget::TextField>("", "只读选择字段").build();
        read_only->set_read_only(true);
        const auto fields = ui.row()
                                .gap(12.0F)
                                .cross_alignment(widget::LayoutAlignment::center)
                                .children(editable)
                                .children(read_only)
                                .build();

        // RadioGroup：成员持有真实焦点，方向键同时移动焦点与选择。
        auto radio_group = widget::RadioGroup::create();
        const auto radios =
            ui.row()
                .gap(12.0F)
                .cross_alignment(widget::LayoutAlignment::center)
                .children(ui.make<widget::RadioButton>("选项一", radio_group).build())
                .children(ui.make<widget::RadioButton>("选项二", radio_group).build())
                .children(ui.make<widget::RadioButton>("选项三", radio_group).build())
                .build();

        // Tabs：容器持有真实焦点，方向键只移动内部活动项。
        auto& selected = ui.signal_value(0);
        const auto tabs =
            ui.make<widget::Tabs>(selected, std::vector<std::string> {"笔记", "任务", "设置"})
                .configure([](widget::Tabs& widget) { widget.set_name("focus-tabs"); })
                .build();

        // DropdownMenu：触发控件聚焦，打开后容器持有焦点、内部条目漫游。
        auto menu_trigger = ui.make<widget::Button>("打开菜单").build();
        const auto menu =
            ui.make<widget::DropdownMenu>(menu_trigger, menu_items())
                .configure([](widget::DropdownMenu& widget) { widget.set_name("focus-menu"); })
                .build();

        const auto roving =
            ui.row()
                .width(widget::authoring::fill)
                .gap(16.0F)
                .cross_alignment(widget::LayoutAlignment::start)
                .children(
                    ui.expanded().child(ui.column()
                                            .gap(6.0F)
                                            .children(hint_label(ui, "RadioGroup（成员焦点）"))
                                            .children(radios)
                                            .build())
                )
                .children(ui.expanded().child(ui.column()
                                                  .gap(6.0F)
                                                  .children(hint_label(ui, "Tabs（容器焦点）"))
                                                  .children(tabs)
                                                  .build()))
                .children(ui.expanded().child(ui.column()
                                                  .gap(6.0F)
                                                  .children(hint_label(ui, "下拉菜单（容器焦点）"))
                                                  .children(menu)
                                                  .build()))
                .build();

        // Dialog：指针打开时不应伪装成键盘导航；Escape 关闭后恢复触发按钮。
        auto dialog_content = ui.make<widget::Label>(
                                    "按 Escape 关闭。关闭后焦点应回到“打开对话框”按钮，"
                                    "并显示键盘提示。"
        )
                                  .width(widget::authoring::fill)
                                  .leading(theme::Leading::normal)
                                  .configure([](widget::Label& label) {
                                      label.set_overflow(text::TextOverflow::wrap);
                                      label.set_max_lines(4);
                                  })
                                  .build();
        const auto dialog =
            ui.make<widget::Dialog>("焦点恢复", dialog_content)
                .configure([](widget::Dialog& widget) { widget.set_name("focus-dialog"); })
                .build();
        const auto dialog_button =
            ui.make<widget::Button>("打开对话框").on_click([dialog] { dialog->open(); }).build();
        const auto dialog_row = ui.row()
                                    .gap(12.0F)
                                    .cross_alignment(widget::LayoutAlignment::center)
                                    .children(dialog_button)
                                    .build();

        const auto content = ui.column()
                                 .width(widget::authoring::fill)
                                 .height(widget::authoring::fill)
                                 .gap(14.0F)
                                 .cross_alignment(widget::LayoutAlignment::start)
                                 .children(section_title(ui, "输入与焦点（D1 第一阶段）"))
                                 .children(hint_text)
                                 .children(interactive_title)
                                 .children(buttons)
                                 .children(fields)
                                 .children(section_title(ui, "两种漫游模型"))
                                 .children(roving)
                                 .children(section_title(ui, "弹窗进入与恢复"))
                                 .children(dialog_row)
                                 .children(preference_anchor)
                                 .children(dialog)
                                 .build();

        const auto padded = ui.padding(foundation::NanInsets::all(32.0F)).child(content).build();
        return ui.scroll_view()
            .width(widget::authoring::fill)
            .height(widget::authoring::fill)
            .child(padded)
            .build();
    }
} // namespace nandina::showcase
