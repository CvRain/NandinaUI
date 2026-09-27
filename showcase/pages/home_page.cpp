// showcase/pages/home_page.cpp — 简短的项目介绍首页。

#include "home_page.hpp"

#include <nandina/theme/nan_style.hpp>
#include <nandina/widget/controls.hpp>

#include <memory>
#include <string>
#include <utility>

namespace nandina::showcase
{
    namespace
    {
        inline constexpr int kUnlimitedLines = 8;

        [[nodiscard]] auto paragraph(
            widget::BuildContext ui,
            std::string content,
            const float font_size,
            const theme::ColorToken color
        ) -> std::shared_ptr<scene::NanControl> {
            return ui.make<widget::Label>(std::move(content))
                .font_size(font_size)
                .color_token(color)
                .configure([](widget::Label& label) {
                    label.set_overflow(widget::primitives::TextOverflow::wrap);
                    label.set_max_lines(kUnlimitedLines);
                })
                .build();
        }
    } // namespace

    auto ShowcaseHomePage::build(app::PageContext& context) -> widget::View {
        auto ui = context.ui();

        auto badges = ui.row()
                          .gap(8.0F)
                          .cross_alignment(widget::LayoutAlignment::center)
                          .children(
                              ui.make<widget::Badge>("C++26"),
                              ui.make<widget::Badge>("Linux"),
                              ui.make<widget::Badge>("alpha")
                          )
                          .build();

        auto intro = ui.column()
                         .gap(12.0F)
                         .children(
                             ui.make<widget::Label>("NandinaUI")
                                 .font_size(34.0F)
                                 .color_token(theme::ColorToken::foreground)
                                 .build(),
                             paragraph(
                                 ui,
                                 "一个基于 C++26 和 raylib 的原生桌面 UI 框架。",
                                 18.0F,
                                 theme::ColorToken::foreground
                             ),
                             std::move(badges),
                             paragraph(
                                 ui,
                                 "项目关注声明式界面、响应式状态、主题和组件组合，目标是让 C++ 桌面应用开发更简单、更舒服。",
                                 15.0F,
                                 theme::ColorToken::muted_foreground
                             )
                         )
                         .build();

        auto alpha_notice = ui.make<widget::Alert>(
                                  theme::AlertTone::info,
                                  "当前阶段",
                                  "NandinaUI 仍处于 alpha 开发阶段，API 和组件会继续演进。"
                              )
                                  .build();

        auto content = ui.column()
                           .gap(24.0F)
                           .children(std::move(intro), std::move(alpha_notice))
                           .build();

        return ui.center()
            .child(ui.padding(foundation::NanInsets::all(32.0F)).child(content))
            .build();
    }
} // namespace nandina::showcase
