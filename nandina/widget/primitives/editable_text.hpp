//
// widget/primitives/editable_text — focused text editing primitive.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_EDITABLE_TEXT_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_EDITABLE_TEXT_HPP

#include "../../scene/control.hpp"
#include "../../scene/clipboard.hpp"
#include "text.hpp"

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::widget::primitives
{
    struct TextSelection {
        std::size_t anchor = 0;
        TextAffinity anchor_affinity = TextAffinity::downstream;
        std::size_t focus = 0;
        TextAffinity focus_affinity = TextAffinity::downstream;
    };

    struct TextComposition {
        std::string text;
        std::size_t selection_start = 0;
        std::size_t selection_end = 0;
    };

    /**
     * 插入符在布局坐标系中的几何位置（逻辑像素）。
     *
     * 外层控件（TextField / TextArea）用它把插入符滚入可视区，也让多行编辑的
     * 插入符绘制与命中测试共用同一份布局事实来源。
     */
    struct TextCaretGeometry {
        std::size_t line_index = 0; // 插入符所在布局行
        float x = 0.0F;             // 行内 x（相对布局原点）
        float top = 0.0F;           // 行顶 y（相对布局原点）
        float height = 0.0F;        // 所在行高
    };

    class EditableText: public scene::NanControl {
    public:
        explicit EditableText(std::string value = {});

        void set_value(std::string value);
        [[nodiscard]] auto value() const -> std::string_view;

        void set_style(TextStyle style);
        [[nodiscard]] auto style() const -> const TextStyle&;

        void set_caret(std::size_t offset, TextAffinity affinity = TextAffinity::downstream);
        [[nodiscard]] auto caret() const -> std::size_t;
        [[nodiscard]] auto caret_affinity() const -> TextAffinity;

        void set_selection(TextSelection selection);
        [[nodiscard]] auto selection() const -> TextSelection;
        [[nodiscard]] auto has_selection() const -> bool;
        void select_all();
        void clear_selection();
        [[nodiscard]] auto selected_text() const -> std::string;

        void set_read_only(bool value);
        [[nodiscard]] auto read_only() const -> bool;
        void set_selection_color(foundation::NanColor color);

        void set_composition(TextComposition composition);
        void clear_composition();
        [[nodiscard]] auto composition() const -> const std::optional<TextComposition>&;

        /// 在插入符处插入文本（替换选区，记录撤销，发出 change）。
        /// 多行输入（TextArea 的 Enter）与单行输入共用这一条写入路径。
        void insert_text(std::string_view text);

        [[nodiscard]] auto caret_geometry() const -> TextCaretGeometry;

        void set_on_change(std::function<void(std::string_view)> callback);

        auto execute_edit_command(scene::EditCommand command, scene::IClipboard* clipboard) -> bool;
        [[nodiscard]] auto can_undo() const noexcept -> bool;
        [[nodiscard]] auto can_redo() const noexcept -> bool;

        [[nodiscard]] auto text_node() -> Text&;
        [[nodiscard]] auto text_node() const -> const Text&;

        void set_text_pipeline(TextPipeline pipeline);
        [[nodiscard]] auto text_pipeline() const -> TextPipeline;
        void apply_default_text_pipeline(const TextPipeline& pipeline) override;
        void apply_font_context(text::FontPipelineCache& context) override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;

        void draw_at(render::DrawContext& ctx, foundation::NanPoint position);

        [[nodiscard]] auto is_focusable() const -> bool override;
        auto handle_input(scene::InputEvent& event, scene::IClipboard* clipboard)
            -> bool;
        auto on_input(scene::InputEvent& event) -> bool override;
        auto on_draw(render::DrawContext& ctx) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;

    private:
        struct EditSnapshot {
            std::string value;
            std::size_t caret = 0;
            TextAffinity caret_affinity = TextAffinity::downstream;
            TextSelection selection;
        };

        /// 一个 source 偏移落到布局的哪一行、落在该行的哪个视觉停靠点。
        struct LineStop {
            std::size_t line_index = 0;
            TextCaretStop stop;
        };

        [[nodiscard]] auto snapshot() const -> EditSnapshot;
        [[nodiscard]] auto line_stop_for(std::size_t offset, TextAffinity affinity) const -> LineStop;
        void record_undo();
        void restore(EditSnapshot snapshot);
        void undo();
        void redo();
        void erase_before_caret();
        void erase_after_caret();
        void erase_selection();
        void move_caret_visual(int direction, bool extend);
        void move_caret_vertical(int direction, bool extend);
        void move_caret_to_visual_edge(bool end, bool extend);
        void update_selection_focus(TextCaretStop stop, bool extend);
        void sync_text();
        void emit_change();

        std::string value_;
        std::size_t caret_ = 0;
        TextAffinity caret_affinity_ = TextAffinity::downstream;
        TextSelection selection_ {};
        bool read_only_ = false;
        bool focused_ = false;
        foundation::NanColor selection_color_ = foundation::NanColor::from(
            foundation::NanHexRgb {.red = 80, .green = 130, .blue = 220, .alpha = 96}
        );
        std::optional<TextComposition> composition_;
        Text text_;
        std::function<void(std::string_view)> on_change_;
        std::vector<EditSnapshot> undo_stack_;
        std::vector<EditSnapshot> redo_stack_;
    };

} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_EDITABLE_TEXT_HPP
