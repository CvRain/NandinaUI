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
                        "res:// 资源由页面的 ResourceManager 解析；资源包必须在首次绘制前挂载。",
                        "解码后的纹理保留原始尺寸；控件宽高与缩放模式决定它如何适配。",
                        "Image 在首次绘制时惰性加载，并复用窗口的纹理缓存。",
                    })
                    .build()
                    .width(widget::authoring::fill);

            return ui.column()
                .width(widget::authoring::fill)
                .gap(chrome.group)
                .cross_alignment(widget::LayoutAlignment::stretch)
                .children(page_heading(ui, chrome, "图片（Image）"))
                .children(supporting_text(
                    ui,
                    chrome,
                    "Image 显示从文件路径或 res:// 资源加载的纹理，支持拉伸、包含、覆盖三种适配，"
                    "以及源矩形裁剪、着色和同步或异步加载。",
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
                            caption_text(ui, chrome, std::string(source) + " —— 覆盖，固定高度")
                        )
                );
            }

            return section_block(
                ui,
                chrome,
                "基础用法",
                "res:// 资源由页面上下文注入的资源管理器解析。预览固定高度并使用覆盖模式，"
                "让宽幅作品填满卡片且不改变宽高比。",
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
                    .name = "拉伸",
                    .mode = widget::ImageScale::stretch,
                    .note = "填满控件，宽高比可能改变。",
                },
                ScaleSpecimen {
                    .name = "包含",
                    .mode = widget::ImageScale::contain,
                    .note = "完整显示整张图片，空白处沿用卡片表面。",
                },
                ScaleSpecimen {
                    .name = "覆盖",
                    .mode = widget::ImageScale::cover,
                    .note = "填满控件并裁掉溢出部分。",
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
                "缩放模式",
                "同一张方形作品、同样的控件尺寸。拉伸忽略宽高比，包含会留出空白，"
                "覆盖会裁掉多余部分；模式按实例设置。",
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
                AlignmentSpecimen {.name = "起始", .alignment = widget::ImageAlignment::start},
                AlignmentSpecimen {.name = "居中", .alignment = widget::ImageAlignment::center},
                AlignmentSpecimen {.name = "末尾", .alignment = widget::ImageAlignment::end},
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
                "包含对齐",
                "只有包含模式留下未用空间时对齐才生效。在宽控件里放同一张方形作品，"
                "不更换图片也能看出三种位置。",
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
                TintSpecimen {.caption = "默认着色", .tint = std::nullopt},
                TintSpecimen {
                    .caption = "着色 #D8B4F0",
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
                "着色",
                "着色会把采样到的 RGBA 相乘，并与继承的不透明度叠加；适合单色图标集，"
                "无需再准备第二份资源。",
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
