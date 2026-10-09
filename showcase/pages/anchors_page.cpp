/// showcase/pages/anchors_page.cpp — anchors 的侧边栏停靠边切换案例。
///
/// 演示三件事：
///   1. 页面根就是锚定画布，header / 侧边栏 / 编辑区是同层兄弟，空间关系由锚线表达；
///   2. 树 ≠ 布局：切换停靠边只替换锚点描述，组件树、子项顺序与节点身份都不变；
///   3. 多节点切换走 AnchorCanvas::set_child_anchors() 批量提交描述，成功后发布状态；
///      几何、命中与语义在下一次布局更新，不在点击回调中同步更新。
///
/// 边界：画布需要测量时两轴已有确定尺寸。`fill` 在排列容器的 loose() 重排里解析不出值，
/// 所以"占满 Column 剩余空间"的画布不可用；这里把画布放在布局根，用同层兄弟锚点表达
/// "header 之下的剩余区域"。离树 loose 测量下需两轴显式尺寸，百分比同样缺少有限基。

#include "anchors_page.hpp"

#include <nandina/reactive/computed.hpp>
#include <nandina/scene/anchor_canvas.hpp>
#include <nandina/scene/node_ref.hpp>
#include <nandina/theme/visual_state.hpp>
#include <nandina/widget/card.hpp>
#include <nandina/widget/label.hpp>

#include <array>
#include <memory>
#include <string>

namespace nandina::showcase
{
    namespace
    {
        inline constexpr float kHeaderHeight = 56.0F;
        inline constexpr float kSidebarWidth = 220.0F;
        inline constexpr float kGap = 12.0F;

        /// 侧边栏内部仍然用 Column 排列 —— 画布只决定这个 Column 落在哪（逐层治理）。
        auto
        build_sidebar_body(const widget::BuildContext& ui, reactive::Computed<std::string>& status)
            -> std::shared_ptr<scene::NanControl> {
            const auto title = ui.make<widget::Label>("侧边栏")
                                   .color_token(theme::ColorToken::foreground)
                                   .font_size(17.0F)
                                   .build();
            const auto state = ui.make<widget::Label>(status)
                                   .color_token(theme::ColorToken::info)
                                   .font_size(13.0F)
                                   .build();
            const auto note = ui.make<widget::Label>("header / list / footer 仍由 Column 排列")
                                  .color_token(theme::ColorToken::muted_foreground)
                                  .font_size(13.0F)
                                  .build();
            return ui.column()
                .gap(8.0F)
                .cross_alignment(widget::LayoutAlignment::start)
                .children(title, state, note)
                .build();
        }

        auto
        build_editor_body(const widget::BuildContext& ui, reactive::Computed<std::string>& relation)
            -> std::shared_ptr<scene::NanControl> {
            const auto title = ui.make<widget::Label>("编辑区")
                                   .color_token(theme::ColorToken::foreground)
                                   .font_size(17.0F)
                                   .build();
            const auto body =
                ui.make<widget::Label>(relation)
                    .configure([](widget::Label& label) { label.set_name("anchors-relation"); })
                    .color_token(theme::ColorToken::muted_foreground)
                    .font_size(14.0F)
                    .build();
            const auto note = ui.make<widget::Label>("切换只替换锚点描述，不重建组件树")
                                  .color_token(theme::ColorToken::muted_foreground)
                                  .font_size(14.0F)
                                  .build();
            return ui.column()
                .gap(8.0F)
                .cross_alignment(widget::LayoutAlignment::start)
                .children(title, body, note)
                .build();
        }
    } // namespace

    auto AnchorsPage::build(app::PageContext& context) -> widget::View {
        const auto& ui = context.ui();

        auto& docked_right = ui.signal_value(false);
        auto& status = ui.computed([&docked_right]() -> std::string {
            return docked_right.get() ? "当前：停靠右侧" : "当前：停靠左侧";
        });
        auto& relation = ui.computed([&docked_right]() -> std::string {
            return docked_right.get() ? "右边缘锚在侧边栏左边缘上" : "左边缘锚在侧边栏右边缘上";
        });

        const auto header = ui.ref<widget::Row>();
        const auto side = ui.ref<widget::Card>();
        const auto edit = ui.ref<widget::Card>();

        // 先建立画布节点，回调才能按弱引用持有它；`.children()` 只挂载，不会在前向引用
        // 尚未绑定时提前求解。
        auto canvas_builder = ui.anchor_canvas();
        const auto canvas = canvas_builder.build();

        const auto toggle =
            ui.make<widget::Button>("切换停靠边")
                .configure([](widget::Button& button) { button.set_name("anchors-toggle"); })
                .treatment(theme::ButtonTreatment::outlined)
                .on_click([header,
                           side,
                           edit,
                           target = std::weak_ptr<scene::AnchorCanvas>(canvas),
                           &docked_right] {
                    const auto current = target.lock();
                    if (!current) {
                        return;
                    }
                    const auto sidebar_node = side.lock();
                    const auto editor_node = edit.lock();
                    const bool to_right = !docked_right.get();
                    // AnchorSpec 是完整替换而非字段合并，所以两种停靠各自
                    // 给出整份描述，再由批量入口一次校验、一次提交。
                    const auto sidebar_lines =
                                        to_right
                                        ? scene::AnchorSpec {
                                              .right = side.parent.anchor.right,
                                              .top = header.anchor.bottom,
                                              .bottom = side.parent.anchor.bottom
                                          }
                                        : scene::AnchorSpec {
                                              .left = side.parent.anchor.left,
                                              .top = header.anchor.bottom,
                                              .bottom = side.parent.anchor.bottom
                                          };
                    const auto editor_lines =
                                        to_right
                                        ? scene::AnchorSpec {
                                              .left = edit.parent.anchor.left,
                                              .right = side.anchor.left,
                                              .top = header.anchor.bottom,
                                              .bottom = edit.parent.anchor.bottom
                                          }
                                        : scene::AnchorSpec {
                                              .left = side.anchor.right,
                                              .right = edit.parent.anchor.right,
                                              .top = header.anchor.bottom,
                                              .bottom = edit.parent.anchor.bottom
                                          };
                    const std::array updates {
                        scene::AnchorCanvas::Update {sidebar_node, sidebar_lines},
                        scene::AnchorCanvas::Update {editor_node, editor_lines}
                    };
                    current->set_child_anchors(updates);
                    // 描述提交成功后发布状态；几何等下一次 layout 更新。
                    docked_right.set(to_right);
                })
                .build();

        const auto header_row = ui.row()
                                    .configure([](widget::Row& row) {
                                        row.set_name("anchors-header");
                                        row.set_overflow(scene::ControlOverflow::clip);
                                    })
                                    .bind(header)
                                    .height(kHeaderHeight)
                                    .max_height(scene::percent(30.0F))
                                    .gap(kGap)
                                    .cross_alignment(widget::LayoutAlignment::center)
                                    .children(
                                        ui.make<widget::Label>("Anchors 布局：侧边栏停靠切换")
                                            .color_token(theme::ColorToken::foreground)
                                            .font_size(20.0F),
                                        ui.expanded(),
                                        toggle
                                    )
                                    .anchors(
                                        {.left = header.parent.anchor.left,
                                         .right = header.parent.anchor.right,
                                         .top = header.parent.anchor.top}
                                    )
                                    .build();

        const auto sidebar = ui.make<widget::Card>()
                                 .configure([](widget::Card& card) {
                                     card.set_name("anchors-sidebar");
                                     card.set_overflow(scene::ControlOverflow::clip);
                                 })
                                 .bind(side)
                                 .width(kSidebarWidth)
                                 .max_width(scene::percent(30.0F))
                                 .child(build_sidebar_body(ui, status))
                                 .anchors(
                                     {.left = side.parent.anchor.left,
                                      .top = header.anchor.bottom,
                                      .bottom = side.parent.anchor.bottom}
                                 )
                                 .build();

        const auto editor = ui.make<widget::Card>()
                                .configure([](widget::Card& card) {
                                    card.set_name("anchors-editor");
                                    card.set_overflow(scene::ControlOverflow::clip);
                                })
                                .bind(edit)
                                .child(build_editor_body(ui, relation))
                                .anchors(
                                    {.left = side.anchor.right,
                                     .right = edit.parent.anchor.right,
                                     .top = header.anchor.bottom,
                                     .bottom = edit.parent.anchor.bottom}
                                )
                                .build();

        canvas_builder.children(header_row, sidebar, editor);
        return canvas;
    }
} // namespace nandina::showcase
