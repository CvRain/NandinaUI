//
// widget/toggle - two-state button (button appearance, switch semantics).
//

#include "toggle.hpp"

#include "toggle_group.hpp"

#include "primitives/box_painter.hpp"
#include "primitives/focus_ring_painter.hpp"
#include "../render/draw_context.hpp"
#include "../scene/input_event.hpp"
#include "../theme/theme_manager.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        [[nodiscard]] auto near(const float lhs, const float rhs) -> bool {
            return std::abs(lhs - rhs) <= foundation::nan_epsilon;
        }

        [[nodiscard]] auto
        same_text_style(const primitives::TextStyle& lhs, const primitives::TextStyle& rhs)
            -> bool {
            return lhs.color.approx_equals(rhs.color) && near(lhs.font_size, rhs.font_size)
                && lhs.font == rhs.font && lhs.overflow == rhs.overflow
                && lhs.max_lines == rhs.max_lines;
        }
    } // namespace

    Toggle::Toggle(std::string text, theme::NanTheme theme):
        text_(std::move(text)) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        apply_metrics();
    }

    Toggle::Toggle(std::string text, std::shared_ptr<ToggleGroup> group, theme::NanTheme theme):
        text_(std::move(text)) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        set_group(std::move(group));
        apply_metrics();
    }

    Toggle::~Toggle() {
        if (group_) {
            group_->unregister_toggle(this);
        }
    }

    auto Toggle::create(std::string text, theme::NanTheme theme) -> std::shared_ptr<Toggle> {
        return std::make_shared<Toggle>(std::move(text), theme);
    }

    auto Toggle::create(
        std::string text,
        std::shared_ptr<ToggleGroup> group,
        theme::NanTheme theme
    ) -> std::shared_ptr<Toggle> {
        return std::make_shared<Toggle>(std::move(text), std::move(group), theme);
    }

    void Toggle::set_text(std::string text) {
        text_.set_text(std::move(text));
        apply_metrics();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto Toggle::text() const -> std::string_view {
        return text_.text();
    }

    void Toggle::set_checked(const bool checked) {
        if (checked_ == checked) {
            return;
        }
        checked_ = checked;
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto Toggle::checked() const -> bool {
        return checked_;
    }

    void Toggle::toggle() {
        if (disabled()) {
            return;
        }
        const bool previous = checked_;
        if (group_) {
            // 组按模式落地：single 先撤销其它成员，multiple 各自独立。
            group_->toggle_member(this);
        }
        else {
            set_checked(!checked_);
        }
        if (checked_ != previous) {
            if (on_change_) {
                on_change_(checked_);
            }
            checked_changed_.emit(checked_);
        }
    }

    void Toggle::set_tone(const theme::ButtonTone tone) {
        tone_ = tone;
        mark_layout_dirty();
        apply_metrics();
    }

    auto Toggle::tone() const -> theme::ButtonTone {
        return tone_;
    }

    void Toggle::set_treatment(const theme::ButtonTreatment treatment) {
        treatment_ = treatment;
        mark_layout_dirty();
        apply_metrics();
    }

    auto Toggle::treatment() const -> theme::ButtonTreatment {
        return treatment_;
    }

    void Toggle::set_on_change(std::function<void(bool)> callback) {
        on_change_ = std::move(callback);
    }

    auto Toggle::checked_changed() const -> const reactive::Event<bool>& {
        return checked_changed_;
    }

    void Toggle::set_group(std::shared_ptr<ToggleGroup> group) {
        if (group_ == group) {
            return;
        }
        if (group_) {
            group_->unregister_toggle(this);
        }
        group_ = std::move(group);
        if (group_) {
            group_->register_toggle(this);
        }
    }

    auto Toggle::group() const -> const std::shared_ptr<ToggleGroup>& {
        return group_;
    }

    void Toggle::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        apply_metrics();
        mark_layout_dirty();
    }

    auto Toggle::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Toggle::set_override(theme::ToggleRecipeRule rule) {
        override_ = std::move(rule);
        apply_metrics();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto Toggle::visual_state() const -> theme::ToggleVisualState {
        if (disabled()) {
            return theme::ToggleVisualState::disabled;
        }
        if (pressed()) {
            return theme::ToggleVisualState::pressed;
        }
        if (hovered()) {
            return theme::ToggleVisualState::hovered;
        }
        if (focused()) {
            return theme::ToggleVisualState::focused;
        }
        return theme::ToggleVisualState::normal;
    }

    auto Toggle::resolved_style() const -> theme::ResolvedToggleStyle {
        auto style =
            theme::resolve_toggle(*system_, appearance_, tone_, treatment_, checked_, visual_state());
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_, tone_);
        }
        return style;
    }

    void Toggle::set_text_pipeline(primitives::TextPipeline pipeline) {
        text_.set_text_pipeline(std::move(pipeline));
        apply_metrics();
        mark_layout_dirty();
    }

    auto Toggle::text_pipeline() const -> primitives::TextPipeline {
        return text_.text_pipeline();
    }

    void Toggle::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        text_.apply_default_text_pipeline(pipeline);
        apply_metrics();
        mark_layout_dirty();
    }

    void Toggle::apply_font_context(text::FontPipelineCache& context) {
        text_.apply_font_context(context);
        apply_metrics();
        mark_layout_dirty();
    }

    void Toggle::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_metrics();
        mark_layout_dirty();
    }

    void Toggle::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_metrics();
        mark_layout_dirty();
        // 组不是场景节点，收不到主题广播；由成员把快照刷新转交给它（幂等）。
        if (group_) {
            group_->on_theme_changed(manager);
        }
    }

    auto Toggle::on_draw(render::DrawContext& context) -> void {
        const auto style = resolved_style();
        const auto world = render::world_bounds_from_local(context.world_transform(), local_rect());
        const float opacity = context.opacity();

        // 基础容器 / 状态层 / 边框分层绘制（与 Button 同款）：半透明的 hover / pressed
        // 反馈不会改写描边颜色。
        primitives::BoxPainter::paint_fill(context, world, style.container, opacity);
        auto state_layer = style.container;
        state_layer.fill = theme::toggle_state_layer_color(style, visual_state());
        primitives::BoxPainter::paint_fill(context, world, state_layer, opacity);
        primitives::BoxPainter::paint_outline(context, world, style.container, opacity);

        apply_text_style();
        // 文本布局用逻辑尺寸；world 已含视口变换，不能再作为 measure 输入。
        const float content_width = std::max(0.0F, width() - style.metrics.padding_x * 2.0F);
        (void)text_.measure_layout(
            scene::LayoutConstraints {
                .min_width = 0.0F,
                .max_width = content_width,
                .min_height = 0.0F,
                .max_height = height(),
            }
        );
        const float text_width = context.logical_to_screen(text_.measured_text_width());
        const float text_height = context.logical_to_screen(text_.measured_text_height());
        const auto text_position = foundation::NanPoint(
            world.get_left() + (world.get_width() - text_width) * 0.5F,
            world.get_top() + (world.get_height() - text_height) * 0.5F
        );
        text_.draw_at(context, text_position);

        if (focused() && !disabled() && style.focus.width > 0.0F) {
            primitives::FocusRingPainter::paint(context, world, style.focus, opacity);
        }
    }

    auto Toggle::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto style = resolved_style();
        apply_text_style();
        const float max_text_width = std::isfinite(constraints.max_width)
            ? std::max(0.0F, constraints.max_width - style.metrics.padding_x * 2.0F)
            : constraints.max_width;
        (void)text_.measure_layout(
            scene::LayoutConstraints {
                .min_width = 0.0F,
                .max_width = max_text_width,
                .min_height = 0.0F,
                .max_height = constraints.max_height,
            }
        );
        return constraints.constrain(foundation::NanSize(
            text_.width() + style.metrics.padding_x * 2.0F,
            std::max(style.metrics.min_height, style.metrics.height)
        ));
    }

    auto Toggle::is_focusable() const -> bool {
        // 与 Pressable 默认一致；显式覆写把 Toggle 的 Tab 焦点策略固定在头文件里
        // （契约第 5 条要求交互组件明确定义焦点策略）。
        return !disabled();
    }

    auto Toggle::on_input(scene::InputEvent& event) -> bool {
        if (!disabled() && group_) {
            if (event.type() == scene::EventType::key) {
                // 方向键漫游交给组（内部走共享的 RovingFocus）：按键原样转发，由共享
                // 设施按 orientation 过滤并回答"下一个是谁"，本类不自己算下一个是谁，
                // 也不自己判断哪个方向键有效。
                auto& key = static_cast<scene::KeyEvent&>(event);
                if (key.is_pressed() && group_->handle_key(this, key)) {
                    event.accept();
                    return true;
                }
            }
            else if (event.type() == scene::EventType::text_input) {
                if (group_->handle_text(this, static_cast<scene::TextInputEvent&>(event))) {
                    event.accept();
                    return true;
                }
            }
        }
        // Enter / Space / 指针按下-抬起（原地释放）由 Pressable 统一处理。
        return primitives::Pressable::on_input(event);
    }

    void Toggle::on_process(const float dt) {
        // 组不是场景节点，没有自己的 dt 来源。焦点在场景树里是排他的，因此由持焦点的
        // 成员代为推进组的 typeahead 时钟，每帧恰好一次。
        if (group_ && focused()) {
            group_->advance_time(dt);
        }
    }

    void Toggle::on_click() {
        toggle();
    }

    void Toggle::on_pressable_state_changed() {
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto Toggle::semantics_properties() const -> semantics::Properties {
        return {
            // Role 枚举里没有 toggle / button_pressed：两态按钮按 checkbox 暴露
            // （带 checked 状态 + activate 动作），而不是 switch_control —— 后者表示
            // "立即生效的设置"，语义不同（那种情况用 Switch）。
            .role = semantics::Role::checkbox,
            .label = std::string(text()),
            .state =
                {
                    .focusable = !disabled(),
                    .focused = focused(),
                    .disabled = disabled(),
                    .checked = checked_,
                },
            .actions = disabled() ? semantics::Action::none
                                  : semantics::Action::activate | semantics::Action::focus,
        };
    }

    auto Toggle::on_semantics_action(const semantics::ActionRequest& request) -> bool {
        if (request.action != semantics::Action::activate || disabled()) {
            return false;
        }
        activate();
        return true;
    }

    void Toggle::apply_metrics() {
        apply_text_style();
        const auto style = resolved_style();
        (void)text_.measure_layout(scene::LayoutConstraints::loose());
        set_size(foundation::NanSize(
            text_.width() + style.metrics.padding_x * 2.0F,
            std::max(style.metrics.min_height, style.metrics.height)
        ));
    }

    void Toggle::apply_text_style() {
        const auto style = resolved_style();
        const auto& context = resolved_style_context();
        const primitives::TextStyle text_style {
            .color = context.text_color_from_context ? context.text_color : style.label.color,
            .font_size =
                context.font_size_from_context ? context.font_size : style.label.font_size,
            .font = context.font_from_context ? context.font : text_.font(),
            .overflow = primitives::TextOverflow::clip,
            .max_lines = 1,
        };
        if (!same_text_style(text_.style(), text_style)) {
            text_.set_style(text_style);
        }
    }
} // namespace nandina::widget
