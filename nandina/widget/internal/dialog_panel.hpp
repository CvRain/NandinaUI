#ifndef NANDINA_EXPERIMENT_WIDGET_INTERNAL_DIALOG_PANEL_HPP
#define NANDINA_EXPERIMENT_WIDGET_INTERNAL_DIALOG_PANEL_HPP

#include "../../scene/control.hpp"
#include "../../theme/design_system.hpp"
#include "../primitives/text.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::widget::internal
{
    /**
     * 对话框的面板本体：绘制面板外观，并按 header / content / footer 三个固定语义槽位纵向
     * 布局。遮罩、输入阻断与焦点限制都不在这里，由 DismissLayer 与 FocusScope 承担。
     *
     * 三个槽位都接受普通控件；`set_title()` 是 header 的文本便捷入口，与 `set_header()`
     * 互斥，后设置者生效。槽位缺省为空且不占高度。
     */
    class DialogPanel final: public scene::NanControl {
    public:
        DialogPanel();

        auto set_header(std::shared_ptr<scene::NanControl> header) -> scene::NanControl&;
        auto set_content(std::shared_ptr<scene::NanControl> content) -> scene::NanControl&;
        auto set_footer(std::shared_ptr<scene::NanControl> footer) -> scene::NanControl&;

        /// header 槽位的文本便捷入口。
        void set_title(std::string title);
        [[nodiscard]] auto title() const -> std::string_view;

        void set_style(theme::ResolvedDialogStyle style);
        [[nodiscard]] auto style() const noexcept -> const theme::ResolvedDialogStyle&;

        void set_text_pipeline(primitives::TextPipeline pipeline);
        void apply_default_text_pipeline(const primitives::TextPipeline& pipeline);
        void apply_font_context(text::FontPipelineCache& context);

        auto on_draw(render::DrawContext& context) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;
        /// dialog 语义挂在面板上：两种承载方式下它都是可见的那个模态表面，边界就是面板边界。
        /// Dialog 节点在浮层承载时只是不可见的锚点，在那里报告零尺寸的 dialog 反而误导辅助技术。
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        /// 面板两侧至少保留的边距，避免贴住视口边缘。
        static constexpr float kViewportMargin = 16.0F;

        /// 用解析后的配方与样式上下文刷新标题文本样式。
        void apply_title_style();
        /// 按 header / content / footer 的顺序返回当前非空槽位。
        [[nodiscard]] auto slot_nodes() const -> std::vector<scene::NanControl*>;
        [[nodiscard]] auto inner_constraints(float panel_width, float max_height) const
            -> scene::LayoutConstraints;

        std::shared_ptr<primitives::Text> title_;
        std::weak_ptr<scene::NanControl> header_;
        std::weak_ptr<scene::NanControl> content_;
        std::weak_ptr<scene::NanControl> footer_;
        theme::ResolvedDialogStyle style_;
    };
}

#endif
