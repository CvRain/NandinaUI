/// showcase/components/page_chrome.hpp — 组件文档页共用的排版尺度与 section 骨架。
///
/// 两条规则：
///   1. chrome（页面标题、section 标题、说明、留白）取主题令牌；**示例文本**才用字面量字号 ——
///      文档页演示的正是 `font_size`，样本必须能给出具体数值，而令牌只有 14/16/18 三档。
///   2. 间距只用 4px 网格（spacing xs/sm/md/lg/xl = 4/8/12/16/24），并且保持
///      「组内 ≤ 组间 ≤ section 内 ≤ section 间」的单调关系。
#pragma once

#include <nandina/widget/card.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/label.hpp>

#include <string>
#include <string_view>
#include <utility>

namespace nandina::showcase
{
    /// 定义列表的键列宽度：让所有值在同一竖直线上起排（元数据表与语义字段共用）。
    inline constexpr float kKeyColumnWidth = 120.0F;

    /// 文档页的排版尺度。默认值全部来自令牌或 4px 网格，构造时由 `make_page_chrome` 覆盖。
    struct PageChrome {
        float tight = 4.0F; ///< spacing.xs —— 标签与它的图注之间
        float item = 8.0F; ///< spacing.sm —— 同一组内的行距
        float group = 12.0F; ///< spacing.md —— 组与组之间
        float section = 16.0F; ///< spacing.lg —— section 内部（标题 / 说明 / 面板）
        float block = 24.0F; ///< spacing.xl —— section 与 section 之间
        float page = 32.0F; ///< spacing.xl + spacing.sm —— 页面四周留白
        /// 正文可读宽度。宽屏下不设上限会让行长失控（2560px 显示器上接近 2000px）。
        /// 令牌里没有"内容宽度"这一档，故用字面量；它属于页面布局决策，不属于设计系统。
        float content_max = 960.0F;
        /// 页面主标题：令牌最大字号是 18，页面级标题需要更大的层级跨度，故用字面量。
        float page_title = 32.0F;
        /// section 标题：令牌的 18 与正文 16 差距不足以分层，取 20 作为中间档。
        float section_title = 20.0F;
        float body = 16.0F; ///< typography.label_md
        float supporting = 14.0F; ///< typography.label_sm
        float caption = 12.0F; ///< 令牌没有 12 这一档；用于元数据与图注
    };

    [[nodiscard]] inline auto make_page_chrome(const widget::BuildContext& ui) -> PageChrome {
        const auto& tokens = ui.theme().tokens;
        return PageChrome {
            .tight = tokens.spacing.xs,
            .item = tokens.spacing.sm,
            .group = tokens.spacing.md,
            .section = tokens.spacing.lg,
            .block = tokens.spacing.xl,
            .page = tokens.spacing.xl + tokens.spacing.sm,
            .content_max = 960.0F,
            .page_title = 32.0F,
            .section_title = 20.0F,
            .body = tokens.typography.label_md,
            .supporting = tokens.typography.label_sm,
            .caption = 12.0F,
        };
    }

    /// 页面主标题（每页一个）。
    [[nodiscard]] inline auto
    page_heading(const widget::BuildContext& ui, const PageChrome& chrome, std::string_view text)
        -> widget::authoring::NodeBuilder<widget::Label> {
        return ui.make<widget::Label>(std::string(text))
            .color_token(theme::ColorToken::foreground)
            .font_size(chrome.page_title)
            .width(widget::authoring::fill);
    }

    /// section 标题。
    [[nodiscard]] inline auto
    section_heading(const widget::BuildContext& ui, const PageChrome& chrome, std::string_view text)
        -> widget::authoring::NodeBuilder<widget::Label> {
        return ui.make<widget::Label>(std::string(text))
            .color_token(theme::ColorToken::foreground)
            .font_size(chrome.section_title)
            .width(widget::authoring::fill);
    }

    /// section 说明 / 页面导语：`muted_foreground` + 换行。
    [[nodiscard]] inline auto supporting_text(
        const widget::BuildContext& ui,
        const PageChrome& chrome,
        std::string_view text,
        int max_lines = 12
    ) -> widget::authoring::NodeBuilder<widget::Label> {
        return ui.make<widget::Label>(std::string(text))
            .color_token(theme::ColorToken::muted_foreground)
            .font_size(chrome.supporting)
            .width(widget::authoring::fill)
            .configure([max_lines](widget::Label& label) {
                label.set_overflow(text::TextOverflow::wrap);
                label.set_max_lines(max_lines);
            });
    }

    /// 元数据 / 图注：比说明更弱一档，用于键名与尺寸标注。
    [[nodiscard]] inline auto
    caption_text(const widget::BuildContext& ui, const PageChrome& chrome, std::string_view text)
        -> widget::authoring::NodeBuilder<widget::Label> {
        return ui.make<widget::Label>(std::string(text))
            .color_token(theme::ColorToken::muted_foreground)
            .font_size(chrome.caption)
            .width(widget::authoring::fill);
    }

    /// 示例表面：与页面底色分离的一层。Card 自带配方内边距，不需要再包 Padding。
    template<typename Content>
    [[nodiscard]] auto example_surface(const widget::BuildContext& ui, Content&& content)
        -> widget::authoring::NodeBuilder<widget::Card> {
        return ui.make<widget::Card>()
            .width(widget::authoring::fill)
            .child(std::forward<Content>(content));
    }

    /// 统一的 section 骨架：标题 → 说明 → 预览面板。
    /// 标题与说明留在面板外，示例进面板 —— 这是"文档 + 预览"的常见分层，
    /// 也让每个 section 的高度差异只体现在面板内部。
    template<typename Content>
    [[nodiscard]] auto section_block(
        const widget::BuildContext& ui,
        const PageChrome& chrome,
        std::string_view title,
        std::string_view description,
        Content&& content
    ) -> widget::authoring::NodeBuilder<widget::Column> {
        return ui.column()
            .width(widget::authoring::fill)
            .gap(chrome.section)
            .cross_alignment(widget::LayoutAlignment::stretch)
            .children(section_heading(ui, chrome, title))
            .children(supporting_text(ui, chrome, description))
            .children(example_surface(ui, std::forward<Content>(content)));
    }
} // namespace nandina::showcase
