//
// widget/toggle_group - shared selection + roaming coordination for Toggle members.
//

#include "toggle_group.hpp"

#include "key_codes.hpp"
#include "toggle.hpp"

#include "../scene/input_event.hpp"
#include "../theme/theme_manager.hpp"

#include <algorithm>
#include <string_view>
#include <utility>

namespace nandina::widget
{
    ToggleGroup::ToggleGroup() {
        theme_view_ = theme::default_theme();
        system_ = std::make_shared<const theme::DesignSystem>(
            theme::design_system_from_theme(theme_view_)
        );
    }

    auto ToggleGroup::create() -> std::shared_ptr<ToggleGroup> {
        return std::make_shared<ToggleGroup>();
    }

    void ToggleGroup::register_toggle(Toggle* toggle) {
        if (toggle == nullptr || std::ranges::find(members_, toggle) != members_.end()) {
            return;
        }
        members_.push_back(toggle);
        sync_focus();
        // single 模式：新成员若已选中，撤销其它成员，保持"至多一个"不变量。注册不是
        // 用户选择，所以不发 selection_changed。
        if (mode_ == ToggleGroupMode::single && toggle->checked()) {
            for (auto* member: members_) {
                if (member != nullptr && member != toggle) {
                    member->set_checked(false);
                }
            }
        }
    }

    void ToggleGroup::unregister_toggle(Toggle* toggle) {
        std::erase(members_, toggle);
        sync_focus();
    }

    auto ToggleGroup::member_count() const -> std::size_t {
        return members_.size();
    }

    auto ToggleGroup::index_of(const Toggle* toggle) const -> int {
        for (std::size_t i = 0; i < members_.size(); ++i) {
            if (members_[i] == toggle) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    void ToggleGroup::set_mode(const ToggleGroupMode mode) {
        if (mode_ == mode) {
            return;
        }
        mode_ = mode;
        if (mode_ != ToggleGroupMode::single) {
            return;
        }
        // 收敛到第一个选中项（注册顺序），保持 single 的"至多一个"不变量。
        const auto before = checked_indices();
        bool seen = false;
        for (auto* member: members_) {
            if (member == nullptr || !member->checked()) {
                continue;
            }
            if (seen) {
                member->set_checked(false);
            }
            seen = true;
        }
        auto after = checked_indices();
        if (after != before) {
            selection_changed_.emit(std::move(after));
        }
    }

    auto ToggleGroup::mode() const -> ToggleGroupMode {
        return mode_;
    }

    void ToggleGroup::set_allow_empty(const bool allow_empty) {
        allow_empty_ = allow_empty;
    }

    auto ToggleGroup::allow_empty() const -> bool {
        return allow_empty_;
    }

    auto ToggleGroup::checked_indices() const -> std::vector<int> {
        std::vector<int> indices;
        for (std::size_t i = 0; i < members_.size(); ++i) {
            if (members_[i] != nullptr && members_[i]->checked()) {
                indices.push_back(static_cast<int>(i));
            }
        }
        return indices;
    }

    void ToggleGroup::select(const int index) {
        if (index < 0 || static_cast<std::size_t>(index) >= members_.size()) {
            return;
        }
        auto* target = members_[static_cast<std::size_t>(index)];
        if (target == nullptr) {
            return;
        }
        // 程序化路径不因 disabled 提前返回：disabled 只屏蔽用户输入，程序仍需要能
        // 还原外部状态（与 Toggle::set_checked 一致）。
        const auto before = checked_indices();
        if (mode_ == ToggleGroupMode::single) {
            for (auto* member: members_) {
                if (member != nullptr && member != target) {
                    member->set_checked(false);
                }
            }
        }
        target->set_checked(true);
        auto after = checked_indices();
        if (after != before) {
            selection_changed_.emit(std::move(after));
        }
    }

    auto ToggleGroup::selection_changed() const -> const reactive::Event<std::vector<int>>& {
        return selection_changed_;
    }

    void ToggleGroup::toggle_member(Toggle* toggle) {
        if (toggle == nullptr || toggle->disabled()) {
            return;
        }
        const auto before = checked_indices();
        if (std::ranges::find(members_, toggle) == members_.end()) {
            // 未注册的成员：退化为独立切换，而不是静默吞掉用户的点击。
            toggle->set_checked(!toggle->checked());
        }
        else if (mode_ == ToggleGroupMode::multiple) {
            toggle->set_checked(!toggle->checked());
        }
        else {
            const bool next = !toggle->checked();
            if (!next && !allow_empty_) {
                return; // single + 不允许空组：取消唯一选中项是 no-op
            }
            for (auto* member: members_) {
                if (member != nullptr && member != toggle) {
                    member->set_checked(false);
                }
            }
            toggle->set_checked(next);
        }
        auto after = checked_indices();
        if (after != before) {
            selection_changed_.emit(std::move(after));
        }
    }

    void ToggleGroup::sync_focus() {
        focus_.set_movement(RovingMovement::widget_focus);
        // 回调持有 members_ 的引用（不是快照）：注册顺序 = 视觉顺序 = 漫游顺序，
        // 索引 → 控件的映射始终由本类负责。sync() 会把同一个回调分发给每个成员。
        const auto& members = members_;
        focus_.sync(
            members.size(),
            [&members](const std::size_t index) {
                return members[index] != nullptr && !members[index]->disabled();
            },
            [&members](const std::size_t index) -> std::string_view {
                return members[index] != nullptr ? members[index]->text() : std::string_view {};
            }
        );
    }

    auto ToggleGroup::handle_key(Toggle* from, const scene::KeyEvent& event) -> bool {
        if (members_.empty()) {
            return false;
        }
        const int current = index_of(from);
        if (current < 0) {
            return false;
        }
        sync_focus();
        focus_.set_active_index(current);
        // 原样交给共享设施：方向键按 orientation 过滤，Home / End / PageUp / PageDown
        // 由它统一回答；本类只把索引映射回控件并落地焦点。
        const auto intent = focus_.handle_key(event);
        if (!intent.has_value() || intent->index < 0) {
            return false;
        }
        const auto target = static_cast<std::size_t>(intent->index);
        if (target >= members_.size() || members_[target] == nullptr) {
            return false;
        }
        // widget_focus：方向键只把焦点交给目标成员，不改写 checked —— toggle 的值
        // 语义属于 Enter / Space / 点击这类显式激活。
        if (intent->move_widget_focus) {
            members_[target]->request_focus();
        }
        return true;
    }

    auto ToggleGroup::move_focus(Toggle* from, const int direction) -> bool {
        if (members_.empty() || direction == 0) {
            return false;
        }
        const int current = index_of(from);
        if (current < 0) {
            return false;
        }
        sync_focus();
        focus_.set_active_index(current);
        // RovingFocus 只接受**按键**而不是"方向"：横向组必须喂左右键、纵向组必须喂
        // 上下键，否则 handle_key 会认为不是它的键而返回 nullopt。用户输入路径
        // （handle_key）不需要这段映射，只有这个"方向语义"入口需要（见报告摩擦点）。
        const bool horizontal = focus_.orientation() == RovingOrientation::horizontal;
        const int keycode = horizontal ? (direction < 0 ? keys::left : keys::right)
                                       : (direction < 0 ? keys::up : keys::down);
        const auto intent =
            focus_.handle_key(scene::KeyEvent(keycode, scene::KeyEvent::Action::press));
        if (!intent.has_value() || intent->index < 0) {
            return false;
        }
        const auto target = static_cast<std::size_t>(intent->index);
        if (target >= members_.size() || members_[target] == nullptr) {
            return false;
        }
        if (intent->move_widget_focus) {
            members_[target]->request_focus();
        }
        return true;
    }

    auto ToggleGroup::handle_text(Toggle* from, const scene::TextInputEvent& event) -> bool {
        if (members_.empty()) {
            return false;
        }
        const int current = index_of(from);
        if (current < 0) {
            return false;
        }
        sync_focus();
        focus_.set_active_index(current);
        const auto intent = focus_.handle_text(event);
        if (!intent.has_value() || intent->index < 0) {
            return false;
        }
        const auto target = static_cast<std::size_t>(intent->index);
        if (target >= members_.size() || members_[target] == nullptr) {
            return false;
        }
        if (intent->move_widget_focus) {
            members_[target]->request_focus();
        }
        return true;
    }

    void ToggleGroup::advance_time(const float dt) {
        focus_.advance_time(dt);
    }

    auto ToggleGroup::typeahead_buffer() const -> std::string_view {
        return focus_.typeahead_buffer();
    }

    void ToggleGroup::set_orientation(const RovingOrientation orientation) {
        focus_.set_orientation(orientation);
    }

    auto ToggleGroup::orientation() const -> RovingOrientation {
        return focus_.orientation();
    }

    void ToggleGroup::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
    }

    auto ToggleGroup::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void ToggleGroup::set_override(theme::ToggleGroupRecipeRule rule) {
        override_ = std::move(rule);
    }

    auto ToggleGroup::visual_state() const -> theme::ToggleGroupVisualState {
        return theme::ToggleGroupVisualState::normal;
    }

    auto ToggleGroup::resolved_style() const -> theme::ResolvedToggleGroupStyle {
        auto style = theme::resolve_toggle_group(*system_, appearance_, visual_state());
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void ToggleGroup::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
    }
} // namespace nandina::widget
