// showcase/pages/home_page.cpp — 简短的项目介绍首页。

#include "home_page.hpp"

#include "component_page.hpp"
#include "nandina/foundation/nan_logger.hpp"

#include <nandina/foundation/motion/behavior.hpp>
#include <nandina/foundation/motion/spec.hpp>
#include <nandina/foundation/motion/spring.hpp>
#include <nandina/theme/nan_style.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/visual_property.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace nandina::showcase
{
    namespace
    {
        inline constexpr float kComponentFontSize = 16.0F;
        inline constexpr float kComponentHoverScale = 1.125F;

        auto gen_brief_intro_section(app::PageContext& context) -> auto {
            const auto& ui = context.ui();

            const auto title = ui.make<widget::Label>("简介")
                                   .color_token(theme::ColorToken::foreground)
                                   .font_size(34.0F)
                                   .width(widget::authoring::fill)
                                   .build();

            const std::string intro_text {
                "NandinaUI（南天竹）是一个用 C++26 编写、基于 Meson 构建的原生桌面 UI 框架。"
                "旨在将简单而不失优雅的现代应用开发体验带进C++"
                "让你用一套简洁、类型安全的 DSL 构建桌面界面，而无需在繁琐的样板代码之间来回穿梭。"
            };
            const auto intro = ui.make<widget::Label>(intro_text)
                                   .color_token(theme::ColorToken::info)
                                   .font_size(16.0F)
                                   .width(widget::authoring::fill)
                                   .configure([](widget::Label& label) {
                                       label.set_overflow(widget::primitives::TextOverflow::wrap);
                                       label.set_max_lines(8);
                                       label.set_color(foundation::NanColor::from_hex(0x6c6f85));
                                   })
                                   .build();

            const auto content = ui.column()
                                     .width(widget::authoring::fill)
                                     .height(widget::authoring::fill)
                                     .gap(5.0F)
                                     .cross_alignment(widget::LayoutAlignment::start)
                                     .children(title)
                                     .children(intro);

            return content;
        }

        auto gen_core_features_section(app::PageContext& context) -> auto {
            const auto& ui = context.ui();

            std::vector<std::string> features = {
                "声明式 UI DSL —— 用 ui.column()、ui.center()、ui.make<widget::Button>() "
                "组合出可读的界面树，布局、对齐、间距一链式完成。",

                "响应式状态 —— signal / computed / effect / property / batch，"
                "状态变化自动驱动界面更新，告别手动刷新。",

                "可组合组件库 —— 覆盖内容展示、输入选择、布局滚动、浮层反馈与通用指针手势，"
                "并通过统一主题和声明式构建器组合。",

                "动画系统 —— Tween、Spring、关键帧、缓动曲线与动画组，让过渡与动效顺滑自然。",

                "现代文本引擎 —— FreeType + HarfBuzz + FriBidi + utf8proc 组成的字形管线，"
                "支持多字体、系统字体发现、复杂文字整形与双向文本。",

                "主题与设计系统 —— 三层设计令牌（primitive → semantic → component）、"
                "明暗外观（Appearance）、内置主题与样式文档。",

                "资源系统 —— 资源清单（manifest）+ 内置/目录/内存/SQLite 四类后端，"
                "配合 nanres 编译器与可移植打包流程。",

                "应用运行时 —— 窗口、Router/Page 导航、视口缩放、异步作用域与统一的输入/剪贴板分发。",

                "可选 2D 物理 —— 基于 Box2D 3.x 的轻量物理桥（默认关闭）。",

                "无障碍语义 —— 控件语义树导出，为可访问性工具铺路。"
            };

            const auto title = ui.make<widget::Label>("核心特性")
                                   .color_token(theme::ColorToken::foreground)
                                   .font_size(34.0F)
                                   .width(widget::authoring::fill)
                                   .build();

            auto layout = ui.column()
                              .width(widget::authoring::fill)
                              .height(widget::authoring::fill)
                              .gap(8.0F)
                              .cross_alignment(widget::LayoutAlignment::start)
                              .children(title);

            for (const auto& it: features) {
                const auto intro =
                    ui.make<widget::Label>(it)
                        .font_size(14.0F)
                        .width(widget::authoring::fill)
                        .configure([](widget::Label& label) {
                            label.set_overflow(widget::primitives::TextOverflow::wrap);
                            label.set_max_lines(8);
                            label.set_color(foundation::NanColor::from_hex(0x7c7f93));
                        })
                        .build();

                layout.children(intro);
            }

            return layout;
        }

        auto gen_motion_section(app::PageContext& context) -> auto {
            const auto& ui = context.ui();
            auto section = ui.column()
                               .width(widget::authoring::fill)
                               .gap(12.0F)
                               .children(ui.make<widget::Label>("动效体验").font_size(28.0F));
            for (const bool use_spring: {false, true}) {
                auto& hovered = ui.signal_value(false);
                auto& opacity = ui.computed([&hovered] { return hovered.get() ? 1.0F : 0.35F; });
                auto& scale = ui.computed([&hovered] {
                    const float value = hovered.get() ? 1.08F : 1.0F;
                    return foundation::NanPoint(value, value);
                });
                auto preview =
                    ui.make<widget::Label>(use_spring ? "Spring · 弹性淡入" : "Tween · 匀速淡入")
                        .font_size(22.0F)
                        .color_token(theme::ColorToken::foreground)
                        .opacity(0.35F)
                        .transform_origin(scene::TransformOrigin::left)
                        .behavior(
                            widget::visual::scale,
                            motion::Behavior<foundation::NanPoint>(0.2F)
                        )
                        .bind(widget::visual::scale, scale);
                if (use_spring) {
                    preview.spring(widget::visual::opacity, motion::SpringSpec(300.0F, 12.0F));
                }
                else {
                    preview.behavior(
                        widget::visual::opacity,
                        motion::Behavior<float>(0.35F, motion::Easing::linear)
                    );
                }
                preview.bind(widget::visual::opacity, opacity);
                // Keep the hover region stationary while its child scales.
                section.children(ui.make<widget::GestureArea>()
                                     .width(widget::authoring::fill)
                                     .height(64.0F)
                                     .on_pointer_enter([&hovered](const scene::MouseEnterEvent&) {
                                         hovered.set(true);
                                     })
                                     .on_pointer_leave([&hovered](const scene::MouseLeaveEvent&) {
                                         hovered.set(false);
                                     })
                                     .child(preview));
            }
            return section;
        }

        auto gen_components_section(app::PageContext& context) -> auto {
            const auto& ui = context.ui();
            const float hover_animation_duration =
                ui.theme_manager().design_system().tokens.motion.short_duration;

            const auto title = ui.make<widget::Label>("组件速览")
                                   .color_token(theme::ColorToken::foreground)
                                   .font_size(34.0F)
                                   .width(widget::authoring::fill)
                                   .build();

            const auto navigation = context.navigation();
            auto component_groups = ui.column()
                                        .width(widget::authoring::fill)
                                        .gap(12.0F)
                                        .cross_alignment(widget::LayoutAlignment::start);

            for (const auto& category: kComponentCategories) {
                auto category_grid = ui.grid()
                                         .width(widget::authoring::fill)
                                         .cross_alignment(widget::LayoutAlignment::center)
                                         .configure([](widget::Grid& grid) {
                                             grid.set_gap(8.0F, 8.0F);
                                             grid.set_columns(3);
                                         });

                for (const auto& component: kComponentCatalog) {
                    if (component.category != category.id) {
                        continue;
                    }

                    auto& is_hover = ui.signal_value(false);
                    auto& hovered_scale = ui.computed([&is_hover]() {
                        const float scale = is_hover.get() ? kComponentHoverScale : 1.0F;
                        return foundation::NanPoint(scale, scale);
                    });

                    // 颜色交给语义角色，悬浮反馈使用不触发布局与字形重栅格化的 L2 scale。
                    //
                    // 这里原本把两个写死的十六进制颜色绑到 `visual::label.color` 上，那有两个
                    // 问题：它违反"页面里不写原始色"的约定；更要紧的是**切换明暗外观后必然停在
                    // 旧值** —— 驱动它的 computed 只依赖 `is_hover`，而 `ThemeManager` 目前只有
                    // revision + 观察者、没有 Signal，所以"主题变了"这件事进不了响应式依赖；
                    // 何况 PropertyEndpoint 一旦有值就会永久遮罩配方字段，Label 的
                    // on_theme_changed 也救不回来（见 stories/005 §6）。
                    //
                    // 保留颜色悬浮的出路是补上"主题版本信号"，那时这里可以重新绑颜色。
                    auto label =
                        ui.make<widget::Label>(std::string(component.name))
                            .width(widget::authoring::fill)
                            .font_size(kComponentFontSize)
                            .color_token(theme::ColorToken::foreground)
                            .transform_origin(scene::TransformOrigin::center)
                            .bind(widget::visual::scale, hovered_scale)
                            .behavior(
                                widget::visual::scale,
                                motion::tween(hover_animation_duration).easing(motion::ease_linear)
                            )
                            .configure([](widget::Label& component_label) {
                                component_label.set_overflow(
                                    widget::primitives::TextOverflow::clip
                                );
                                component_label.set_max_lines(1);
                                component_label.set_align(theme::TextAlign::center);
                            })
                            .build();

                    auto item =
                        ui.make<widget::GestureArea>()
                            .width(widget::authoring::fill)
                            .height(36.0F)
                            .on_pointer_enter([&is_hover](const scene::MouseEnterEvent&) {
                                is_hover.set(true);
                            })
                            .on_pointer_leave([&is_hover](const scene::MouseLeaveEvent&) {
                                is_hover.set(false);
                            })
                            .on_click([navigation,
                                       component_id = component.id](const scene::MouseEvent&) {
                                if (!navigation.navigate<ComponentPage>(
                                        ComponentPageParams {component_id}
                                    ))
                                {
                                    log::error("showcase home: cannot enter component page");
                                }
                            })
                            .child(label);

                    category_grid.children(item);
                }

                component_groups.children(
                    ui.make<widget::Label>(std::string(category.name))
                        .font_size(18.0F)
                        .color_token(theme::ColorToken::foreground),
                    category_grid
                );
            }

            auto layout = ui.column()
                              .width(widget::authoring::fill)
                              .height(widget::authoring::fill)
                              .gap(8.0F)
                              .cross_alignment(widget::LayoutAlignment::start)
                              .children(title)
                              .children(component_groups);

            return layout;
        }
    } // namespace

    auto ShowcaseHomePage::build(app::PageContext& context) -> widget::View {
        const auto sections = std::vector {
            gen_brief_intro_section(context),
            gen_motion_section(context),
            gen_core_features_section(context),
            gen_components_section(context)
        };

        const auto& ui = context.ui();

        auto section_layout = ui.column()
                                  .width(widget::authoring::fill)
                                  .height(widget::authoring::fill)
                                  .gap(25.0F)
                                  .cross_alignment(widget::LayoutAlignment::start);

        for (const auto& section: sections) {
            section_layout.children(section);
        }

        const auto main_content =
            ui.scroll_view()
                .width(widget::authoring::fill)
                .height(widget::authoring::fill)
                .child(ui.padding(foundation::NanInsets::all(32.0F)).child(section_layout));

        return main_content.build();
    }
} // namespace nandina::showcase
