//
// widget/internal/breadcrumb_link - Breadcrumb 内部的可聚焦链接条目。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_INTERNAL_BREADCRUMB_LINK_HPP
#define NANDINA_EXPERIMENT_WIDGET_INTERNAL_BREADCRUMB_LINK_HPP

#include "../../scene/control.hpp"
#include "../../theme/design_system.hpp"
#include "../primitives/pressable.hpp"
#include "../primitives/text.hpp"

#include <string>
#include <string_view>

namespace nandina::widget::internal
{
    /**
     * Breadcrumb 的可点击条目：`Pressable` 派生的内部子控件，只在条目带回调时挂载。
     *
     * 键盘可达（Tab 焦点 + Enter / Space 激活）与指针点击走同一条 `Pressable::on_click`
     * 路径，因此"鼠标能点、辅助技术点不了"的缺口不存在。平台没有 link role，本控件
     * 暴露 `Role::button`（见 Breadcrumb 文档的"无障碍语义"一节）。
     *
     * 颜色 / 字体 / 焦点环全部由 Breadcrumb 从解析后的配方传入，本控件不持有主题快照，
     * 也不写任何颜色字面量。
     */
    class BreadcrumbLink final: public primitives::Pressable {
    public:
        explicit BreadcrumbLink(std::string label);

        void set_label(std::string label);
        [[nodiscard]] auto label() const -> std::string_view;

        /// 基础文字样式（Breadcrumb 传入解析后的 `link` 片段）。
        void set_text_style(primitives::TextStyle style);
        [[nodiscard]] auto text_style() const -> const primitives::TextStyle&;
        /// 当前字体请求（Breadcrumb 组装 TextStyle 时保留链接自身的字体）。
        [[nodiscard]] auto font() const -> const text::FontRequest&;
        /// hover / pressed 时的文字色（Breadcrumb 传入配方 `link_hover_color`）。
        void set_hover_color(foundation::NanColor color);
        /// 焦点环（Breadcrumb 传入配方 `link_focus`）。
        void set_focus_ring(theme::ResolvedFocusRing ring);

        void apply_default_text_pipeline(const primitives::TextPipeline& pipeline) override;
        void apply_font_context(text::FontPipelineCache& context) override;

        auto on_draw(render::DrawContext& context) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_pressable_state_changed() override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        primitives::Text text_;
        foundation::NanColor hover_color_ {};
        theme::ResolvedFocusRing focus_ {};
    };
} // namespace nandina::widget::internal

#endif // NANDINA_EXPERIMENT_WIDGET_INTERNAL_BREADCRUMB_LINK_HPP
