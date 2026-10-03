/// @file showcase/components/sidebar.cpp
/// @author cvrain
/// @date 2026/10/1
/// 当前页面的导航栏侧边栏。它是外壳的一部分，和内容区共存于同一个 window。
/// 在实际开发中遇到了两个问题
/// 悬浮的"底色变化"仍然是瞬时的。state layer 是 on_draw 里每帧算出来的叠加色，不是动画属性，
/// 框架没有暴露它。要让它也带过渡，得改 Button（给 state layer 加一个 AnimatedProperty<float> 透明度 + 配方里的时长字段）
/// 那是框架级改动、影响所有按钮。
///
/// 圆角基值在构建时读一次。实例覆盖的本质是"接管这个字段"，所以主题在运行中改圆角令牌不会反映到已有条目上；
/// 圆角令牌与明暗外观无关，实际不会变。

#include "sidebar.hpp"

#include "../pages/component_page.hpp"

#include <nandina/app/nan_router.hpp>
#include <nandina/foundation/motion/spec.hpp>
#include <nandina/foundation/nan_logger.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/visual_property.hpp>

#include <string>

namespace nandina::showcase
{
    namespace
    {
        /// 侧边栏按钮文字：优先 `title`，其次显示用地址文字 `address`，都没有时给占位符。
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

    auto Sidebar::generate_nav_item_builder(
        const std::string& item_title,
        const app::PageKey& page_key,
        reactive::Signal<bool>& hovered,
        reactive::Signal<bool>& pressed
    ) const -> auto {
        const auto& ui = context.ui();
        const auto& navigation = context.navigation();
        const float micro_motion = ui.theme_manager().design_system().tokens.motion.short_duration;

        auto builder =
            ui.make<widget::Button>(item_title)
                .width(widget::authoring::fill)
                .height(k_item_height)
                // 初始 treatment 只是"第一帧别闪"：真正的选中态由下面绑定 Router 决定。
                .treatment(theme::ButtonTreatment::outlined)
                .font_size(k_item_font_size)

                // 登记过渡策略**不等于**写入值：这里只安装插值方式，值的来源仍然是
                // 主题配方，所以换主题、换 tone 都不受影响。
                //
                // 这里选 `behavior`（定时长缓动）而不是 `spring`：增量只有 2px，弹簧
                // 那点过冲（阻尼 28 / 刚度 420 ≈ 5%，算下来不到 0.2px）根本看不见，
                // 而弹簧参数没有任何主题来源 —— 换成主题令牌驱动的时长，手感才能被
                // 主题统一调。弹簧留给需要连续性的大位移（拖拽、面板推出）更合适。
                .behavior(widget::visual::container.radius, motion::tween(micro_motion))
                .on_hover_changed([&hovered](const bool value) { hovered.set(value); })
                .on_press([&pressed] { pressed.set(true); })
                .on_release([&pressed] { pressed.set(false); })
                .on_cancel([&pressed] { pressed.set(false); })
                .on_click([navigation, page_key] {
                    // 运行时页面键的失败是预期路径（路由被移除、窗口正在关闭），
                    // 所以在这里就地处理，而不是让它变成一个静默的空点击。
                    if (const auto entered = navigation.navigate_to(page_key); !entered) {
                        log::error(
                            "showcase sidebar: cannot enter route: {}",
                            describe(entered.error())
                        );
                    }
                });

        return builder.build();
    }

    auto Sidebar::generate_navigation_buttons() const -> auto {
        const auto ui = context.ui();
        const auto navigation = context.navigation();

        // 当前页面的响应式来源由 Router 持有，外壳在 set_shell() 时就能绑定它 ——
        // 那时还没有任何页面，首屏 start() 会把它发布出来。
        auto& current = context.current_page();

        // 导航项直接从路由表生成：`nav_entries()` 已经把「显示在导航里」和
        // 「点一下就进得去」两条规则收好了，这里不必再按页面类型逐个写
        // `navigate<PageT>()` 分支，也不会漏掉 `activate` 判空而留下点了没反应的死条目。
        //
        // 动效时长取自主题令牌而不是写死：主题把 `short_duration` 调小（或改成 0）
        // 就能整体降速或关闭这类微交互，不必逐个组件改。
        auto navigation_buttons = ui.column().gap(k_item_gap).width(widget::authoring::fill);

        navigation_buttons.children(ui.make<widget::Label>("页面").font_size(12.0F).color_token(
            theme::ColorToken::muted_foreground
        ));

        for (const auto* entry: context.routes().nav_entries()) {
            const auto page_key = entry->page_key;

            // `Button::hovered()` / `pressed()` 是普通 getter、不是响应式来源。要用它们驱动
            // 动画，得先由交互回调把它们转成信号。信号由**外壳作用域**持有，回调也被
            // `guarded()` 包过，所以作用域清理时不会留下悬垂引用 —— 清理顺序是先失效
            // 回调、最后才释放 signal。
            auto& hovered = ui.signal_value(false);
            auto& pressed = ui.signal_value(false);

            const auto title_text = nav_label(*entry);

            auto button = generate_nav_item_builder(title_text, page_key, hovered, pressed);

            // 基值取主题解析出来的圆角，不硬编码 —— 实例覆盖只在它之上加增量，所以面板
            // 换一套圆角尺度时这里跟着走。代价是这个值在构建时取一次：运行中改圆角令牌
            // 需要重建外壳才会反映到这里（圆角令牌与明暗外观无关，实际不会变）。
            const float base_radius = button->resolved_style().container.radius;

            // 悬浮 + 按下的圆角动画。两个状态**叠加**而不是互相覆盖：按住时鼠标必然也在
            // 悬浮，若让 pressed 优先，按下瞬间圆角会先回缩一次，看起来像卡了一下。
            // 捕获列表是刻意的：`base_radius` 是循环体内的局部量、`k_*_radius_gain` 是成员
            // （读成员就等于捕获 `this`，而 `Sidebar` 是 main_window 里的临时对象，
            // set_shell 工厂一返回就销毁）。这个 computed 活在**外壳作用域**里、会在
            // hover/press 变化时被重新求值，所以三者都必须按值捕获 —— 否则动画一开始
            // 生效就是读悬垂内存。
            auto& radius = ui.computed([&hovered,
                                        &pressed,
                                        base_radius,
                                        hover_gain = k_hover_radius_gain,
                                        pressed_gain = k_pressed_radius_gain] {
                float gain = 0.0F;
                if (hovered.get()) {
                    gain += hover_gain;
                }
                if (pressed.get()) {
                    gain += pressed_gain;
                }
                return base_radius + gain;
            });
            ui.bind(button, widget::visual::container.radius, radius);

            // 选中态：换 treatment（outlined → filled）。颜色全部来自配方，所以跟随主题与
            // tone，切换明暗外观也不会在这里留下写死的颜色。
            auto& is_current =
                ui.computed([&current, page_key] { return current.get() == page_key; });
            ui.bind(
                button,
                [](widget::Button& node, const bool active) {
                    node.set_treatment(
                        active ? theme::ButtonTreatment::filled : theme::ButtonTreatment::outlined
                    );
                },
                is_current
            );

            // 按下时的水波纹由配方的 ripple 字段驱动（默认 `motion_medium_duration`），
            // Button 自己按 `on_process` 推进并遵循 reduced-motion，这里不必重做一遍。
            navigation_buttons.children(button);
        }

        navigation_buttons.children(
            ui.make<widget::Divider>().width(widget::authoring::fill),
            ui.make<widget::Label>("组件").font_size(12.0F).color_token(
                theme::ColorToken::muted_foreground
            )
        );

        for (const auto& category: kComponentCategories) {
            navigation_buttons.children(ui.make<widget::Label>(std::string(category.name))
                                            .font_size(11.0F)
                                            .color_token(theme::ColorToken::muted_foreground));

            for (const auto& component: kComponentCatalog) {
                if (component.category != category.id) {
                    continue;
                }

                navigation_buttons.children(
                    ui.make<widget::Button>(std::string(component.name))
                        .width(widget::authoring::fill)
                        .height(k_item_height)
                        .treatment(theme::ButtonTreatment::outlined)
                        .font_size(k_item_font_size)
                        .on_click([navigation, component_id = component.id] {
                            if (!navigation.navigate<ComponentPage>(
                                    ComponentPageParams {component_id}
                                )) {
                                log::error("showcase sidebar: cannot enter component page");
                            }
                        })
                );
            }
        }
        return navigation_buttons;
    }

    auto Sidebar::build_shell() const -> widget::View {
        const auto ui = context.ui();
        const auto navigation = context.navigation();

        auto navigation_buttons = generate_navigation_buttons();

        auto button_scroll_view = ui.scroll_view()
                                      .child(navigation_buttons)
                                      .width(widget::authoring::fill)
                                      .height(widget::authoring::fill);

        auto sidebar_content = ui.make<widget::Card>()
                                   .width(scene::percent(100))
                                   .height(scene::percent(100))
                                   .child(button_scroll_view);

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
