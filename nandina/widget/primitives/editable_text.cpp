//
// widget/primitives/editable_text — focused text editing primitive.
//

#include "editable_text.hpp"

#include "../key_codes.hpp"

#include "../../foundation/utf8.hpp"
#include "../../render/draw_context.hpp"
#include "../../scene/input_event.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace nandina::widget::primitives
{
    namespace
    {
        constexpr float caret_width = 1.0F;
        constexpr std::size_t history_limit = 100;

        [[nodiscard]] auto
        clamp_grapheme_boundary(const std::string_view text, const std::size_t offset)
            -> std::size_t {
            const auto requested = std::min(offset, text.size());
            for (const auto& grapheme: foundation::utf8::grapheme_ranges(text)) {
                const auto end = grapheme.offset + grapheme.length;
                if (requested < end) {
                    return grapheme.offset;
                }
            }
            return text.size();
        }

        [[nodiscard]] auto
        next_grapheme_boundary(const std::string_view text, const std::size_t offset)
            -> std::size_t {
            const auto requested = std::min(offset, text.size());
            for (const auto& grapheme: foundation::utf8::grapheme_ranges(text)) {
                const auto end = grapheme.offset + grapheme.length;
                if (requested == grapheme.offset) {
                    return requested;
                }
                if (requested < end) {
                    return end;
                }
            }
            return text.size();
        }
    } // namespace

    EditableText::EditableText(std::string value):
        value_(std::move(value)),
        caret_(value_.size()),
        text_(value_) {
        clear_selection();
    }

    void EditableText::set_value(std::string value) {
        if (value_ == value) {
            return;
        }
        value_ = std::move(value);
        caret_ = clamp_grapheme_boundary(value_, caret_);
        caret_affinity_ = TextAffinity::downstream;
        clear_selection();
        clear_composition();
        undo_stack_.clear();
        redo_stack_.clear();
        sync_text();
    }

    auto EditableText::value() const -> std::string_view {
        return value_;
    }

    void EditableText::set_style(TextStyle style) {
        text_.set_style(style);
        mark_layout_dirty();
    }

    auto EditableText::style() const -> const TextStyle& {
        return text_.style();
    }

    void EditableText::set_caret(const std::size_t offset, const TextAffinity affinity) {
        caret_ = clamp_grapheme_boundary(value_, offset);
        caret_affinity_ = affinity;
        clear_selection();
    }

    auto EditableText::caret() const -> std::size_t {
        return caret_;
    }

    auto EditableText::caret_affinity() const -> TextAffinity {
        return caret_affinity_;
    }

    void EditableText::set_selection(TextSelection selection) {
        selection.anchor = clamp_grapheme_boundary(value_, selection.anchor);
        selection.focus = clamp_grapheme_boundary(value_, selection.focus);
        selection_ = selection;
        caret_ = selection.focus;
        caret_affinity_ = selection.focus_affinity;
    }

    auto EditableText::selection() const -> TextSelection {
        return selection_;
    }

    auto EditableText::has_selection() const -> bool {
        return selection_.anchor != selection_.focus;
    }

    void EditableText::select_all() {
        selection_ = TextSelection {
            .anchor = 0,
            .anchor_affinity = TextAffinity::downstream,
            .focus = value_.size(),
            .focus_affinity = TextAffinity::upstream,
        };
        caret_ = selection_.focus;
        caret_affinity_ = selection_.focus_affinity;
    }

    void EditableText::clear_selection() {
        selection_ = TextSelection {
            .anchor = caret_,
            .anchor_affinity = caret_affinity_,
            .focus = caret_,
            .focus_affinity = caret_affinity_,
        };
    }

    auto EditableText::selected_text() const -> std::string {
        if (!has_selection()) {
            return {};
        }
        const auto lower = std::min(selection_.anchor, selection_.focus);
        const auto upper = std::max(selection_.anchor, selection_.focus);
        return value_.substr(lower, upper - lower);
    }

    void EditableText::set_read_only(const bool value) {
        read_only_ = value;
    }
    auto EditableText::read_only() const -> bool {
        return read_only_;
    }
    void EditableText::set_selection_color(const foundation::NanColor color) {
        selection_color_ = color;
    }

    void EditableText::set_composition(TextComposition composition) {
        composition.selection_start =
            std::min(composition.selection_start, composition.text.size());
        composition.selection_end = std::min(composition.selection_end, composition.text.size());
        composition_ = std::move(composition);
    }
    void EditableText::clear_composition() {
        composition_.reset();
    }
    auto EditableText::composition() const -> const std::optional<TextComposition>& {
        return composition_;
    }

    void EditableText::set_on_change(std::function<void(std::string_view)> callback) {
        on_change_ = std::move(callback);
    }

    auto EditableText::execute_edit_command(
        const scene::EditCommand command,
        scene::IClipboard* clipboard
    ) -> bool {
        switch (command) {
            case scene::EditCommand::select_all:
                select_all();
                return true;
            case scene::EditCommand::copy:
                if (clipboard != nullptr && has_selection()) {
                    (void)clipboard->write_text(selected_text());
                }
                return true;
            case scene::EditCommand::cut:
                if (!read_only_ && clipboard != nullptr && has_selection()
                    && clipboard->write_text(selected_text()))
                {
                    erase_selection();
                }
                return true;
            case scene::EditCommand::paste:
                if (!read_only_ && clipboard != nullptr) {
                    if (const auto text = clipboard->read_text(); text && !text->empty()) {
                        insert_text(*text);
                    }
                }
                return true;
            case scene::EditCommand::undo:
                if (!read_only_) {
                    undo();
                }
                return true;
            case scene::EditCommand::redo:
                if (!read_only_) {
                    redo();
                }
                return true;
        }
        return false;
    }

    auto EditableText::can_undo() const noexcept -> bool {
        return !undo_stack_.empty();
    }

    auto EditableText::can_redo() const noexcept -> bool {
        return !redo_stack_.empty();
    }

    auto EditableText::text_node() -> Text& {
        return text_;
    }

    auto EditableText::text_node() const -> const Text& {
        return text_;
    }

    void EditableText::set_text_pipeline(TextPipeline pipeline) {
        text_.set_text_pipeline(pipeline);
        mark_layout_dirty();
        (void)text_.measure_layout(last_layout_constraints());
        set_size(text_.size());
    }

    auto EditableText::text_pipeline() const -> TextPipeline {
        return text_.text_pipeline();
    }

    void EditableText::apply_default_text_pipeline(const TextPipeline& pipeline) {
        text_.apply_default_text_pipeline(pipeline);
        mark_layout_dirty();
    }

    void EditableText::apply_font_context(text::FontPipelineCache& context) {
        text_.apply_font_context(context);
        mark_layout_dirty();
    }

    void EditableText::on_style_context_changed(const theme::ResolvedStyleContext& context) {
        text_.on_style_context_changed(context);
        mark_layout_dirty();
    }

    void EditableText::draw_at(render::DrawContext& ctx, foundation::NanPoint position) {
        const auto& layout = text_.layout_result();
        if (has_selection()) {
            const auto lower = std::min(selection_.anchor, selection_.focus);
            const auto upper = std::max(selection_.anchor, selection_.focus);
            const auto color =
                selection_color_.with_alpha(selection_color_.alpha() * ctx.opacity());
            float line_top = 0.0F;
            for (const auto& line: layout.lines) {
                for (std::size_t index = 1; index < line.caret_stops.size(); ++index) {
                    const auto& left = line.caret_stops[index - 1];
                    const auto& right = line.caret_stops[index];
                    const auto source_begin = std::min(left.source_offset, right.source_offset);
                    const auto source_end = std::max(left.source_offset, right.source_offset);
                    if (source_end <= lower || source_begin >= upper || left.x == right.x) {
                        continue;
                    }
                    ctx.device().draw_rect(
                        foundation::NanRect::from_xywh(
                            position.get_x() + ctx.logical_to_screen(std::min(left.x, right.x)),
                            position.get_y() + ctx.logical_to_screen(line_top),
                            ctx.logical_to_screen(std::abs(right.x - left.x)),
                            ctx.logical_to_screen(line.size.get_height())
                        ),
                        color
                    );
                }
                line_top += line.size.get_height();
            }
        }
        text_.draw_at(ctx, position);

        if (!focused_ || layout.lines.empty()) {
            return;
        }

        const auto geometry = caret_geometry();
        const auto color = text_.color().with_alpha(text_.color().alpha() * ctx.opacity());
        const float x = position.get_x() + ctx.logical_to_screen(geometry.x);
        ctx.device().draw_line(
            foundation::NanPoint(x, position.get_y() + ctx.logical_to_screen(geometry.top)),
            foundation::NanPoint(
                x,
                position.get_y() + ctx.logical_to_screen(geometry.top + geometry.height)
            ),
            ctx.logical_to_screen(caret_width),
            color
        );
    }

    auto EditableText::is_focusable() const -> bool {
        return true;
    }

    auto EditableText::handle_input(
        scene::InputEvent& event,
        scene::IClipboard* clipboard
    ) -> bool {
        switch (event.type()) {
            case scene::EventType::focus_enter:
                focused_ = true;
                event.accept();
                return true;
            case scene::EventType::focus_leave:
                focused_ = false;
                event.accept();
                return true;
            case scene::EventType::text_input: {
                if (read_only_) {
                    event.accept();
                    return true;
                }
                auto& text_event = static_cast<scene::TextInputEvent&>(event);
                insert_text(text_event.text());
                event.accept();
                return true;
            }
            case scene::EventType::key: {
                auto& key_event = static_cast<scene::KeyEvent&>(event);
                const auto modifiers = key_event.modifiers();
                const bool primary = modifiers.ctrl || modifiers.super;
                if (key_event.is_pressed() && primary) {
                    std::optional<scene::EditCommand> command;
                    switch (key_event.keycode()) {
                        case keys::a:
                            command = scene::EditCommand::select_all;
                            break;
                        case keys::c:
                            command = scene::EditCommand::copy;
                            break;
                        case keys::x:
                            command = scene::EditCommand::cut;
                            break;
                        case keys::v:
                            command = scene::EditCommand::paste;
                            break;
                        case keys::y:
                            command = scene::EditCommand::redo;
                            break;
                        case keys::z:
                            command = modifiers.shift ? scene::EditCommand::redo
                                                      : scene::EditCommand::undo;
                            break;
                        default:
                            break;
                    }
                    if (command) {
                        (void)execute_edit_command(*command, clipboard);
                        event.accept();
                        return true;
                    }
                }
                if (key_event.is_pressed() && key_event.keycode() == keys::backspace) {
                    if (!read_only_) {
                        has_selection() ? erase_selection() : erase_before_caret();
                    }
                    event.accept();
                    return true;
                }
                if (!key_event.is_pressed()) {
                    return false;
                }
                const bool extend = key_event.modifiers().shift;
                switch (key_event.keycode()) {
                    case keys::delete_key:
                        if (!read_only_) {
                            has_selection() ? erase_selection() : erase_after_caret();
                        }
                        break;
                    case keys::left:
                        move_caret_visual(-1, extend);
                        break;
                    case keys::right:
                        move_caret_visual(1, extend);
                        break;
                    case keys::up:
                        move_caret_vertical(-1, extend);
                        break;
                    case keys::down:
                        move_caret_vertical(1, extend);
                        break;
                    case keys::home:
                        move_caret_to_visual_edge(false, extend);
                        break;
                    case keys::end:
                        move_caret_to_visual_edge(true, extend);
                        break;
                    default:
                        return false;
                }
                event.accept();
                return true;
            }
            case scene::EventType::mouse_button:
            case scene::EventType::mouse_move:
            case scene::EventType::mouse_enter:
            case scene::EventType::mouse_leave:
            case scene::EventType::mouse_wheel:
                return false;
        }
        return false;
    }

    auto EditableText::on_input(scene::InputEvent& event) -> bool {
        auto* clipboard = is_inside_tree() ? get_tree()->clipboard() : nullptr;
        return handle_input(event, clipboard);
    }

    void EditableText::on_draw(render::DrawContext& ctx) {
        const auto pos = ctx.world_transform().transform_point(foundation::NanPoint::zero());
        draw_at(ctx, pos);
    }

    auto EditableText::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto measured = text_.measure_layout(constraints);
        set_size(measured);
        return measured;
    }

    auto EditableText::snapshot() const -> EditSnapshot {
        return EditSnapshot {
            .value = value_,
            .caret = caret_,
            .caret_affinity = caret_affinity_,
            .selection = selection_,
        };
    }

    void EditableText::record_undo() {
        if (undo_stack_.size() == history_limit) {
            undo_stack_.erase(undo_stack_.begin());
        }
        undo_stack_.push_back(snapshot());
        redo_stack_.clear();
    }

    void EditableText::restore(EditSnapshot state) {
        value_ = std::move(state.value);
        caret_ = state.caret;
        caret_affinity_ = state.caret_affinity;
        selection_ = state.selection;
        clear_composition();
        sync_text();
        emit_change();
    }

    void EditableText::undo() {
        if (undo_stack_.empty()) {
            return;
        }
        redo_stack_.push_back(snapshot());
        auto previous = std::move(undo_stack_.back());
        undo_stack_.pop_back();
        restore(std::move(previous));
    }

    void EditableText::redo() {
        if (redo_stack_.empty()) {
            return;
        }
        if (undo_stack_.size() == history_limit) {
            undo_stack_.erase(undo_stack_.begin());
        }
        undo_stack_.push_back(snapshot());
        auto next = std::move(redo_stack_.back());
        redo_stack_.pop_back();
        restore(std::move(next));
    }

    void EditableText::insert_text(std::string_view text) {
        if (read_only_ || text.empty()) {
            return;
        }
        record_undo();
        if (has_selection()) {
            const auto lower = std::min(selection_.anchor, selection_.focus);
            const auto upper = std::max(selection_.anchor, selection_.focus);
            value_.erase(lower, upper - lower);
            caret_ = lower;
            caret_affinity_ = TextAffinity::downstream;
            clear_selection();
        }
        value_.insert(caret_, text);
        caret_ += text.size();
        caret_ = next_grapheme_boundary(value_, caret_);
        caret_affinity_ = TextAffinity::downstream;
        clear_selection();
        clear_composition();
        sync_text();
        emit_change();
    }

    void EditableText::erase_before_caret() {
        if (caret_ == 0 || value_.empty()) {
            return;
        }
        const auto graphemes = foundation::utf8::grapheme_ranges(value_);
        const foundation::utf8::ByteRange* previous = nullptr;
        for (const auto& grapheme: graphemes) {
            if (grapheme.offset + grapheme.length <= caret_) {
                previous = &grapheme;
            }
        }
        if (previous == nullptr) {
            return;
        }
        record_undo();
        value_.erase(previous->offset, previous->length);
        caret_ = previous->offset;
        caret_affinity_ = TextAffinity::downstream;
        clear_selection();
        sync_text();
        emit_change();
    }

    void EditableText::erase_after_caret() {
        const auto graphemes = foundation::utf8::grapheme_ranges(value_);
        const auto next = std::ranges::find_if(graphemes, [&](const auto& grapheme) {
            return grapheme.offset >= caret_;
        });
        if (next == graphemes.end()) {
            return;
        }
        record_undo();
        value_.erase(next->offset, next->length);
        caret_affinity_ = TextAffinity::downstream;
        clear_selection();
        sync_text();
        emit_change();
    }

    void EditableText::erase_selection() {
        if (!has_selection()) {
            return;
        }
        record_undo();
        const auto lower = std::min(selection_.anchor, selection_.focus);
        const auto upper = std::max(selection_.anchor, selection_.focus);
        value_.erase(lower, upper - lower);
        caret_ = lower;
        caret_affinity_ = TextAffinity::downstream;
        clear_selection();
        clear_composition();
        sync_text();
        emit_change();
    }

    auto EditableText::line_stop_for(const std::size_t offset, const TextAffinity affinity) const
        -> LineStop {
        const auto& layout = text_.layout_result();
        if (layout.lines.empty()) {
            return {};
        }
        for (std::size_t index = 0; index < layout.lines.size(); ++index) {
            const auto& line = layout.lines[index];
            const auto end = line.text_offset + line.text_length;
            if (offset < line.text_offset) {
                const auto previous = index == 0 ? 0 : index - 1;
                const auto& source = layout.lines[previous];
                return {.line_index = previous, .stop = source.caret_for_source(offset, affinity)};
            }
            if (offset > end) {
                continue;
            }
            // 软换行边界上同一个偏移同时属于前一行的行尾与后一行的行首：
            // downstream 归属后一行行首，upstream 归属前一行行尾。
            if (offset == end && affinity == TextAffinity::downstream
                && index + 1 < layout.lines.size()
                && layout.lines[index + 1].text_offset == end)
            {
                return {
                    .line_index = index + 1,
                    .stop = layout.lines[index + 1].caret_for_source(offset, affinity),
                };
            }
            return {.line_index = index, .stop = line.caret_for_source(offset, affinity)};
        }
        const auto last = layout.lines.size() - 1;
        return {
            .line_index = last,
            .stop = layout.lines[last].caret_for_source(offset, affinity),
        };
    }

    auto EditableText::caret_geometry() const -> TextCaretGeometry {
        const auto& layout = text_.layout_result();
        if (layout.lines.empty()) {
            return {};
        }
        const auto located = line_stop_for(caret_, caret_affinity_);
        float top = 0.0F;
        for (std::size_t index = 0; index < located.line_index; ++index) {
            top += layout.lines[index].size.get_height();
        }
        return {
            .line_index = located.line_index,
            .x = located.stop.x,
            .top = top,
            .height = layout.lines[located.line_index].size.get_height(),
        };
    }

    void EditableText::move_caret_visual(const int direction, const bool extend) {
        const auto& layout = text_.layout_result();
        if (layout.lines.empty() || direction == 0) {
            return;
        }
        const auto current = line_stop_for(caret_, caret_affinity_);
        const auto& line = layout.lines[current.line_index];
        if (line.caret_stops.empty()) {
            return;
        }

        if (has_selection() && !extend) {
            const auto anchor = line_stop_for(selection_.anchor, selection_.anchor_affinity);
            const auto focus = line_stop_for(selection_.focus, selection_.focus_affinity);
            const auto visual_less = [](const LineStop& lhs, const LineStop& rhs) {
                if (lhs.line_index != rhs.line_index) {
                    return lhs.line_index < rhs.line_index;
                }
                return lhs.stop.x < rhs.stop.x;
            };
            const bool anchor_first = visual_less(anchor, focus);
            const LineStop& target =
                direction < 0 ? (anchor_first ? anchor : focus) : (anchor_first ? focus : anchor);
            update_selection_focus(target.stop, false);
            return;
        }

        auto current_index = std::size_t {0};
        float nearest = std::numeric_limits<float>::infinity();
        for (std::size_t index = 0; index < line.caret_stops.size(); ++index) {
            const auto& stop = line.caret_stops[index];
            if (stop.source_offset != current.stop.source_offset) {
                continue;
            }
            const float distance = std::abs(stop.x - current.stop.x);
            if (distance < nearest) {
                current_index = index;
                nearest = distance;
            }
        }

        if (direction < 0) {
            if (current_index > 0) {
                update_selection_focus(line.caret_stops[current_index - 1], extend);
                return;
            }
            if (current.line_index == 0) {
                return;
            }
            const auto& previous = layout.lines[current.line_index - 1];
            if (!previous.caret_stops.empty()) {
                update_selection_focus(previous.caret_stops.back(), extend);
            }
            return;
        }

        if (current_index + 1 < line.caret_stops.size()) {
            update_selection_focus(line.caret_stops[current_index + 1], extend);
            return;
        }
        if (current.line_index + 1 >= layout.lines.size()) {
            return;
        }
        const auto& next = layout.lines[current.line_index + 1];
        if (!next.caret_stops.empty()) {
            update_selection_focus(next.caret_stops.front(), extend);
        }
    }

    void EditableText::move_caret_vertical(const int direction, const bool extend) {
        const auto& layout = text_.layout_result();
        if (layout.lines.empty() || direction == 0) {
            return;
        }
        const auto current = line_stop_for(caret_, caret_affinity_);
        if (direction < 0) {
            if (current.line_index == 0) {
                return;
            }
        }
        else if (current.line_index + 1 >= layout.lines.size()) {
            return;
        }
        const auto target_index =
            direction < 0 ? current.line_index - 1 : current.line_index + 1;
        const auto& target_line = layout.lines[target_index];
        if (target_line.caret_stops.empty()) {
            return;
        }
        update_selection_focus(target_line.caret_for_x(current.stop.x), extend);
    }

    void EditableText::move_caret_to_visual_edge(const bool end, const bool extend) {
        const auto& layout = text_.layout_result();
        if (layout.lines.empty()) {
            return;
        }
        const auto current = line_stop_for(caret_, caret_affinity_);
        const auto& stops = layout.lines[current.line_index].caret_stops;
        if (stops.empty()) {
            return;
        }
        update_selection_focus(end ? stops.back() : stops.front(), extend);
    }

    void EditableText::update_selection_focus(const TextCaretStop stop, const bool extend) {
        if (!extend) {
            selection_.anchor = stop.source_offset;
            selection_.anchor_affinity = stop.affinity;
        }
        selection_.focus = stop.source_offset;
        selection_.focus_affinity = stop.affinity;
        caret_ = stop.source_offset;
        caret_affinity_ = stop.affinity;
    }

    void EditableText::sync_text() {
        text_.set_text(value_);
        mark_layout_dirty();
        (void)text_.measure_layout(last_layout_constraints());
        set_size(text_.size());
    }

    void EditableText::emit_change() {
        if (on_change_) {
            on_change_(value_);
        }
    }

} // namespace nandina::widget::primitives
