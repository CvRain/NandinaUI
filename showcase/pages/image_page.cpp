//
// Created by cvrain on 2026/10/24.
//
// Image 组件文档页。
//
// 与 label_page.cpp 使用同一套排版骨架：chrome 走主题令牌，间距只用 4px 网格，
// 每个 section 统一是「标题 → 说明 → Card 预览面板」。图片本身通过 `res://` 从
// showcase 的资源包读取，预览尺寸由页面决定，缩放/对齐只影响绘制区域。
//
// 素材选择：Basic usage 用宽幅作品展示 cover；Scale/Alignment/Tint 统一用同一张方形
// 作品，否则「只改变缩放或对齐」的对照会被素材本身的差异盖住。

#include "image_page.hpp"

#include "../components/component_info_table.hpp"
#include "../components/page_chrome.hpp"

#include <nandina/widget/controls.hpp>

#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace nandina::showcase
{
    namespace
    {
        /// 单张示例图的展示高度。宽度跟随内容列，避免大图按自然尺寸撑破页面。
        inline constexpr float kHeroHeight = 240.0F;
        /// 三种缩放模式并排展示时的统一高度。
        inline constexpr float kScaleHeight = 150.0F;
        /// 对齐示例用更矮的横幅，让 contain 两侧的留白更容易观察。
        inline constexpr float kAlignmentHeight = 112.0F;

        /// 宽幅作品：Basic usage 展示三个 cover 变体。
        inline constexpr std::array kCoverSources {
            std::string_view {"res://images/nandina_cover_1.jpg"},
            std::string_view {"res://images/nandina_cover_2.jpg"},
            std::string_view {"res://images/nandina_cover_3.jpg"},
        };
        /// 方形作品：对照缩放与 contain 对齐时差别最明显。
        inline constexpr std::string_view kLogoSource = "res://images/nandina_logo_1.jpg";

        /// 统一的图片节点：尺寸由页面给，缩放/对齐/着色按 section 需要覆盖。
        [[nodiscard]] auto scaled_image(
            const widget::BuildContext& ui,
            const std::string_view source,
            const float height,
            const widget::ImageScale scale,
            const widget::ImageAlignment alignment = widget::ImageAlignment::center,
            const std::optional<foundation::NanColor> tint = std::nullopt
        ) -> widget::authoring::NodeBuilder<widget::Image> {
            return ui.make<widget::Image>(std::string(source))
                .width(widget::authoring::fill)
                .height(height)
                .configure([scale, alignment, tint](widget::Image& image) {
                    image.set_scale_mode(scale);
                    image.set_alignment(alignment);
                    if (tint) {
                        image.set_tint(*tint);
                    }
                });
        }

        /// 图注放不下时换行而不是省略：这些说明是读完才有意义的整句。
        [[nodiscard]] auto wrapped_caption(
            const widget::BuildContext& ui,
            const PageChrome& chrome,
            const std::string_view text
        ) -> widget::authoring::NodeBuilder<widget::Label> {
            return caption_text(ui, chrome, text).configure([](widget::Label& label) {
                label.set_overflow(text::TextOverflow::wrap);
                label.set_max_lines(3);
            });
        }

        auto gen_brief_intro_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            auto info_table =
                ComponentInfoTable {context}
                    .set_file_location("nandina/widget/image.hpp")
                    .set_inherits("scene::NanControl")
                    .set_tips({
                        "res:// sources resolve through the page's ResourceManager; the package "
                        "must be mounted before the first draw.",
                        "The decoded texture keeps its natural size; width/height and the scale "
                        "mode decide how it is fitted into the control.",
                        "Image loads lazily on first draw and reuses the window texture cache.",
                    })
                    .build()
                    .width(widget::authoring::fill);

            return ui.column()
                .width(widget::authoring::fill)
                .gap(chrome.group)
                .cross_alignment(widget::LayoutAlignment::stretch)
                .children(page_heading(ui, chrome, "Image"))
                .children(supporting_text(
                    ui,
                    chrome,
                    "Image displays a texture loaded from a file path or a res:// resource. "
                    "It supports stretch, contain and cover fitting, source-rect cropping, tint, "
                    "and synchronous or asynchronous loading.",
                    4
                ))
                .children(info_table);
        }

        auto gen_basic_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            auto images = ui.column()
                              .width(widget::authoring::fill)
                              .gap(chrome.block)
                              .cross_alignment(widget::LayoutAlignment::stretch);
            for (const auto source: kCoverSources) {
                images.children(
                    ui.column()
                        .width(widget::authoring::fill)
                        .gap(chrome.item)
                        .cross_alignment(widget::LayoutAlignment::stretch)
                        .children(scaled_image(ui, source, kHeroHeight, widget::ImageScale::cover))
                        .children(
                            caption_text(ui, chrome, std::string(source) + " — cover, fixed height")
                        )
                );
            }

            return section_block(
                ui,
                chrome,
                "Basic usage",
                "A res:// source is resolved by the resource manager injected by the page "
                "context. The preview fixes the height and uses cover so the wide artwork fills "
                "the card without changing its aspect ratio.",
                images
            );
        }

        auto gen_scale_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            struct ScaleSpecimen {
                std::string_view name;
                widget::ImageScale mode;
                std::string_view note;
            };
            constexpr std::array specimens {
                ScaleSpecimen {
                    .name = "Stretch",
                    .mode = widget::ImageScale::stretch,
                    .note = "Fills the control; the aspect ratio may change.",
                },
                ScaleSpecimen {
                    .name = "Contain",
                    .mode = widget::ImageScale::contain,
                    .note = "Fits the whole image; the empty space uses the card surface.",
                },
                ScaleSpecimen {
                    .name = "Cover",
                    .mode = widget::ImageScale::cover,
                    .note = "Fills the control and crops the overflow.",
                },
            };

            auto columns = ui.row()
                               .width(widget::authoring::fill)
                               .gap(chrome.group)
                               .cross_alignment(widget::LayoutAlignment::start);
            for (const auto& [name, mode, note]: specimens) {
                columns.children(ui.expanded().child(
                    ui.column()
                        .width(widget::authoring::fill)
                        .gap(chrome.item)
                        .cross_alignment(widget::LayoutAlignment::stretch)
                        .children(scaled_image(ui, kLogoSource, kScaleHeight, mode))
                        .children(caption_text(ui, chrome, name))
                        .children(wrapped_caption(ui, chrome, note))
                ));
            }

            return section_block(
                ui,
                chrome,
                "Scale modes",
                "The same square artwork at the same control size. Stretch ignores the aspect "
                "ratio, contain letterboxes and cover crops; the choice is per instance.",
                columns
            );
        }

        auto gen_alignment_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            struct AlignmentSpecimen {
                std::string_view name;
                widget::ImageAlignment alignment;
            };
            constexpr std::array specimens {
                AlignmentSpecimen {.name = "Start", .alignment = widget::ImageAlignment::start},
                AlignmentSpecimen {.name = "Center", .alignment = widget::ImageAlignment::center},
                AlignmentSpecimen {.name = "End", .alignment = widget::ImageAlignment::end},
            };

            auto columns = ui.row()
                               .width(widget::authoring::fill)
                               .gap(chrome.group)
                               .cross_alignment(widget::LayoutAlignment::start);
            for (const auto& [name, alignment]: specimens) {
                columns.children(
                    ui.expanded().child(ui.column()
                                            .width(widget::authoring::fill)
                                            .gap(chrome.item)
                                            .cross_alignment(widget::LayoutAlignment::stretch)
                                            .children(scaled_image(
                                                ui,
                                                kLogoSource,
                                                kAlignmentHeight,
                                                widget::ImageScale::contain,
                                                alignment
                                            ))
                                            .children(caption_text(ui, chrome, name)))
                );
            }

            return section_block(
                ui,
                chrome,
                "Contain alignment",
                "Alignment only applies when contain leaves unused space. The same square logo "
                "in a wide control makes the three positions visible without changing the image.",
                columns
            );
        }

        auto gen_tint_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            struct TintSpecimen {
                std::string_view caption;
                std::optional<foundation::NanColor> tint;
            };
            const std::array specimens {
                TintSpecimen {.caption = "Default tint", .tint = std::nullopt},
                TintSpecimen {
                    .caption = "Tint #D8B4F0",
                    .tint = foundation::NanColor::from_hex(0xD8B4F0),
                },
            };

            auto columns = ui.row()
                               .width(widget::authoring::fill)
                               .gap(chrome.group)
                               .cross_alignment(widget::LayoutAlignment::start);
            for (const auto& [caption, tint]: specimens) {
                columns.children(
                    ui.expanded().child(ui.column()
                                            .width(widget::authoring::fill)
                                            .gap(chrome.item)
                                            .cross_alignment(widget::LayoutAlignment::stretch)
                                            .children(scaled_image(
                                                ui,
                                                kLogoSource,
                                                kScaleHeight,
                                                widget::ImageScale::contain,
                                                widget::ImageAlignment::center,
                                                tint
                                            ))
                                            .children(caption_text(ui, chrome, caption)))
                );
            }

            return section_block(
                ui,
                chrome,
                "Tint",
                "Tint multiplies the sampled RGBA and composes with inherited opacity. It is "
                "useful for monochrome icon sets without shipping a second asset.",
                columns
            );
        }
    } // namespace

    auto ImagePage::build(app::PageContext& context) -> widget::View {
        const auto& ui = context.ui();
        const auto chrome = make_page_chrome(ui);

        auto content = ui.column()
                           .width(widget::authoring::fill)
                           .max_width(chrome.content_max)
                           .gap(chrome.block)
                           .cross_alignment(widget::LayoutAlignment::stretch);

        content.children(gen_brief_intro_section(context))
            .children(gen_basic_section(context))
            .children(gen_scale_section(context))
            .children(gen_alignment_section(context))
            .children(gen_tint_section(context));

        return ui.scroll_view()
            .width(widget::authoring::fill)
            .height(widget::authoring::fill)
            .child(ui.padding(foundation::NanInsets::all(chrome.page)).child(content))
            .build();
    }
} // namespace nandina::showcase
