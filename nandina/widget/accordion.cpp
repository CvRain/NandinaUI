#include "accordion.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <unordered_set>

namespace nandina::widget
{
    Accordion::Accordion(theme::NanTheme theme): theme_(theme) {
        set_overflow(scene::ControlOverflow::clip);
    }

    Accordion::~Accordion() = default;

    auto Accordion::create(theme::NanTheme theme) -> std::shared_ptr<Accordion> {
        return std::make_shared<Accordion>(theme);
    }

    // ─── sections ──────────────────────────────────────────────────────────

    void Accordion::validate_sections(const std::vector<AccordionSection>& sections) const {
        std::unordered_set<std::string> ids;
        std::unordered_set<const scene::NanControl*> contents;
        ids.reserve(sections.size());
        contents.reserve(sections.size());
        for (const auto& section: sections) {
            if (section.id.empty()) {
                throw std::invalid_argument("Accordion section id must not be empty");
            }
            if (!ids.insert(section.id).second) {
                throw std::invalid_argument("Accordion section ids must be unique");
            }
            if (section.title.empty()) {
                throw std::invalid_argument("Accordion section title must not be empty");
            }
            if (!section.content) {
                throw std::invalid_argument("Accordion section content must not be null");
            }
            if (section.content.get() == this || section.content->is_ancestor_of(*this)
                || is_ancestor_of(*section.content))
            {
                throw std::logic_error(
                    "Accordion section content must not contain or be contained by the accordion"
                );
            }
            if (section.content->parent() != nullptr || section.content->is_inside_tree()) {
                throw std::logic_error("Accordion section content must be a detached control");
            }
            if (!contents.insert(section.content.get()).second) {
                throw std::invalid_argument("Accordion section contents must be unique");
            }
        }
    }

    void Accordion::set_sections(std::vector<AccordionSection> sections) {
        const auto keep_alive = weak_from_this().lock();
        validate_sections(sections);
        auto prepared =
            std::make_shared<std::vector<Section>>(prepare_sections(std::move(sections)));
        if (auto* tree = get_tree(); tree != nullptr && tree->defers_tree_mutation()) {
            auto self = std::static_pointer_cast<Accordion>(shared_from_this());
            tree->defer_tree_mutation([self = std::move(self),
                                       prepared = std::move(prepared)]() mutable {
                self->apply_sections(std::move(*prepared));
            });
            return;
        }
        apply_sections(std::move(*prepared));
    }

    auto Accordion::prepare_sections(std::vector<AccordionSection> sections)
        -> std::vector<Section> {
        std::vector<Section> built;
        built.reserve(sections.size());
        for (auto& descriptor: sections) {
            auto collapsible = Collapsible::create(descriptor.title, theme_);
            collapsible->set_disabled(descriptor.disabled);
            collapsible->set_child(descriptor.content);
            built.push_back(
                Section {
                    .descriptor = std::move(descriptor),
                    .collapsible = std::move(collapsible),
                    .open_sub = {},
                }
            );
        }
        return built;
    }

    void Accordion::apply_sections(std::vector<Section> built) {
        std::vector<std::string> retained_open;
        for (const auto& old: sections_) {
            if (!old.collapsible->open()) {
                continue;
            }
            const auto& id = old.descriptor.id;
            if (std::ranges::any_of(
                    built,
                    [&id](const Section& section) { return section.descriptor.id == id; }
                ))
            {
                retained_open.push_back(id);
            }
        }

        bool opened = false;
        for (auto& section: built) {
            const bool want =
                std::ranges::find(retained_open, section.descriptor.id) != retained_open.end();
            if (!want) {
                continue;
            }
            if (mode_ == AccordionMode::single && opened) {
                continue;
            }
            section.collapsible->set_open(true);
            opened = true;
        }

        for (auto& old: sections_) {
            if (old.collapsible && old.collapsible->parent() == this) {
                remove_and_delete(*old.collapsible);
            }
        }

        sections_ = std::move(built);
        for (auto& section: sections_) {
            const auto id = section.descriptor.id;
            section.open_sub =
                section.collapsible->open_changed().subscribe([this, id](const bool open) {
                    on_section_toggled(id, open);
                });
            add_child(section.collapsible);
        }
        rebuild_index();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    void Accordion::clear_sections() {
        set_sections({});
    }

    auto Accordion::sections() const -> std::vector<AccordionSection> {
        std::vector<AccordionSection> result;
        result.reserve(sections_.size());
        for (const auto& section: sections_) {
            result.push_back(section.descriptor);
        }
        return result;
    }

    auto Accordion::section_count() const noexcept -> std::size_t {
        return sections_.size();
    }

    auto Accordion::section_node(const std::string_view id) const -> const Collapsible* {
        const auto* section = find_section(id);
        return section != nullptr ? section->collapsible.get() : nullptr;
    }

    auto Accordion::trigger_node(const std::string_view id) const -> const Button* {
        const auto* section = find_section(id);
        return section != nullptr ? &section->collapsible->trigger() : nullptr;
    }

    auto Accordion::focus_section(const std::string_view id) -> bool {
        const auto keep_alive = weak_from_this().lock();
        auto* section = find_section(id);
        auto* tree = get_tree();
        if (section == nullptr || tree == nullptr) {
            return false;
        }
        const auto collapsible = section->collapsible;
        auto& trigger = collapsible->trigger_control();
        if (!trigger.is_visible_in_tree() || trigger.disabled()) {
            return false;
        }
        tree->set_focus(&trigger);
        return tree->focused_node() == &trigger;
    }

    // ─── 展开状态 ──────────────────────────────────────────────────────────

    void Accordion::set_mode(const AccordionMode mode) {
        const auto keep_alive = weak_from_this().lock();
        if (mode != AccordionMode::single && mode != AccordionMode::multiple) {
            throw std::invalid_argument("Accordion mode is invalid");
        }
        if (mode_ == mode) {
            return;
        }
        mode_ = mode;
        if (mode_ != AccordionMode::single) {
            return;
        }
        std::vector<std::shared_ptr<Collapsible>> close;
        close.reserve(sections_.size());
        bool kept = false;
        for (auto& section: sections_) {
            if (!section.collapsible->open()) {
                continue;
            }
            if (kept) {
                close.push_back(section.collapsible);
                continue;
            }
            kept = true;
        }
        for (auto& collapsible: close) {
            collapsible->set_open(false);
        }
    }

    auto Accordion::mode() const noexcept -> AccordionMode {
        return mode_;
    }

    auto Accordion::set_open(const std::string_view id, const bool open) -> bool {
        const auto keep_alive = weak_from_this().lock();
        auto* section = find_section(id);
        if (section == nullptr) {
            return false;
        }
        section->collapsible->set_open(open);
        if (open) {
            enforce_single(section->descriptor.id);
        }
        return true;
    }

    auto Accordion::toggle(const std::string_view id) -> bool {
        const auto keep_alive = weak_from_this().lock();
        auto* section = find_section(id);
        if (section == nullptr) {
            return false;
        }
        // Collapsible::toggle 是用户式切换：发出 open_changed，由本类收敛互斥。
        section->collapsible->toggle();
        return true;
    }

    auto Accordion::is_open(const std::string_view id) const -> bool {
        const auto* section = find_section(id);
        return section != nullptr && section->collapsible->open();
    }

    auto Accordion::open_ids() const -> std::vector<std::string> {
        std::vector<std::string> ids;
        for (const auto& section: sections_) {
            if (section.collapsible->open()) {
                ids.push_back(section.descriptor.id);
            }
        }
        return ids;
    }

    auto Accordion::set_open_ids(std::vector<std::string> ids) -> bool {
        const auto keep_alive = weak_from_this().lock();
        std::unordered_set<std::string> requested;
        requested.reserve(ids.size());
        for (const auto& id: ids) {
            if (!index_.contains(id)) {
                return false;
            }
            requested.insert(id);
        }
        struct Action {
            std::shared_ptr<Collapsible> collapsible;
            bool open = false;
        };
        std::vector<Action> actions;
        actions.reserve(sections_.size());
        bool opened = false;
        for (auto& section: sections_) {
            const bool want = requested.contains(section.descriptor.id);
            if (want && mode_ == AccordionMode::single && opened) {
                actions.push_back({.collapsible = section.collapsible, .open = false});
                continue;
            }
            actions.push_back({.collapsible = section.collapsible, .open = want});
            if (want) {
                opened = true;
            }
        }
        for (auto& action: actions) {
            action.collapsible->set_open(action.open);
        }
        return true;
    }

    void Accordion::clear_open() {
        (void)set_open_ids({});
    }

    auto Accordion::open_changed() const noexcept -> const reactive::Event<>& {
        return open_changed_;
    }

    void Accordion::set_on_open_changed(std::function<void()> callback) {
        user_open_sub_.disconnect();
        if (callback) {
            user_open_sub_ = open_changed_.subscribe(std::move(callback));
        }
    }

    // ─── 单项更新 / 布局 ───────────────────────────────────────────────────

    auto Accordion::set_section_disabled(const std::string_view id, const bool disabled) -> bool {
        const auto keep_alive = weak_from_this().lock();
        auto* section = find_section(id);
        if (section == nullptr) {
            return false;
        }
        section->descriptor.disabled = disabled;
        section->collapsible->set_disabled(disabled);
        return true;
    }

    auto Accordion::set_section_title(const std::string_view id, std::string title) -> bool {
        auto* section = find_section(id);
        if (section == nullptr) {
            return false;
        }
        // set_text 先校验非空，失败时描述与控件都不变。
        section->collapsible->set_text(title);
        section->descriptor.title = std::move(title);
        return true;
    }

    void Accordion::set_gap(const float gap) {
        if (!std::isfinite(gap) || gap < 0.0F) {
            throw std::invalid_argument("Accordion gap must be finite and non-negative");
        }
        gap_ = gap;
        mark_layout_dirty();
    }

    auto Accordion::gap() const noexcept -> float {
        return gap_;
    }

    void Accordion::set_label(std::string label) {
        label_ = std::move(label);
        mark_semantics_dirty();
    }

    auto Accordion::label() const -> std::string_view {
        return label_;
    }

    auto Accordion::on_measure(const foundation::NanLayoutConstraints constraints)
        -> foundation::NanSize {
        float width = constraints.max_width;
        if (!std::isfinite(width)) {
            float natural = 0.0F;
            for (auto& section: sections_) {
                const auto size = section.collapsible->measure_layout(
                    foundation::NanLayoutConstraints {
                        .min_width = 0.0F,
                        .max_width = std::numeric_limits<float>::infinity(),
                        .min_height = 0.0F,
                        .max_height = std::numeric_limits<float>::infinity(),
                    }
                );
                natural = std::max(natural, size.get_width());
            }
            width = std::max(natural, constraints.min_width);
        }
        double total = 0.0;
        for (std::size_t index = 0; index < sections_.size(); ++index) {
            const auto size = sections_[index].collapsible->measure_layout(
                foundation::NanLayoutConstraints {
                    .min_width = width,
                    .max_width = width,
                    .min_height = 0.0F,
                    .max_height = std::numeric_limits<float>::infinity(),
                }
            );
            total += static_cast<double>(size.get_height());
            if (index > 0) {
                total += static_cast<double>(gap_);
            }
        }
        if (!std::isfinite(width) || width < 0.0F || !std::isfinite(total) || total < 0.0
            || total > static_cast<double>(std::numeric_limits<float>::max()))
        {
            throw std::invalid_argument(
                "Accordion content extent must fit finite layout coordinates"
            );
        }
        return constraints.constrain(foundation::NanSize(width, static_cast<float>(total)));
    }

    void Accordion::on_layout() {
        const float width = this->width();
        float y = 0.0F;
        for (std::size_t index = 0; index < sections_.size(); ++index) {
            auto& section = sections_[index];
            const auto size = section.collapsible->measure_layout(
                foundation::NanLayoutConstraints {
                    .min_width = width,
                    .max_width = width,
                    .min_height = 0.0F,
                    .max_height = std::numeric_limits<float>::infinity(),
                }
            );
            section.collapsible->layout_to(
                foundation::NanRect::from_xywh(0.0F, y, width, size.get_height())
            );
            y += size.get_height();
            if (index + 1 < sections_.size()) {
                y += gap_;
            }
        }
    }

    auto Accordion::on_input(scene::InputEvent& event) -> bool {
        if (event.type() != scene::EventType::key) {
            return false;
        }
        auto& key = static_cast<scene::KeyEvent&>(event);
        if (!key.is_press_or_repeat()) {
            return false;
        }
        using foundation::LogicalKey;
        const auto logical = key.logical_key();
        const bool navigation = logical == LogicalKey::up || logical == LogicalKey::down
            || logical == LogicalKey::home || logical == LogicalKey::end;
        if (!navigation) {
            return false;
        }
        auto* tree = get_tree();
        if (tree == nullptr) {
            return false;
        }
        const auto* focused = tree->focused_node();
        if (focused == nullptr) {
            return false;
        }
        std::optional<std::size_t> current;
        for (std::size_t index = 0; index < sections_.size(); ++index) {
            if (focused == &sections_[index].collapsible->trigger()) {
                current = index;
                break;
            }
        }
        if (!current.has_value()) {
            return false;
        }
        const auto enabled = [this](const std::size_t index) {
            return !sections_[index].descriptor.disabled;
        };
        std::size_t next = *current;
        if (logical == LogicalKey::home) {
            for (std::size_t index = 0; index < sections_.size(); ++index) {
                if (enabled(index)) {
                    next = index;
                    break;
                }
            }
        }
        else if (logical == LogicalKey::end) {
            for (std::size_t index = sections_.size(); index-- > 0;) {
                if (enabled(index)) {
                    next = index;
                    break;
                }
            }
        }
        else if (logical == LogicalKey::up) {
            for (std::size_t index = *current; index-- > 0;) {
                if (enabled(index)) {
                    next = index;
                    break;
                }
            }
        }
        else {
            for (std::size_t index = *current + 1; index < sections_.size(); ++index) {
                if (enabled(index)) {
                    next = index;
                    break;
                }
            }
        }
        if (next != *current) {
            (void)focus_section(sections_[next].descriptor.id);
        }
        event.accept();
        return true;
    }

    void Accordion::on_theme_changed(const theme::ThemeManager& manager) {
        theme_ = manager.theme();
        mark_layout_dirty();
    }

    auto Accordion::semantics_properties() const -> semantics::Properties {
        return {.role = semantics::Role::generic, .label = label_};
    }

    // ─── 内部辅助 ──────────────────────────────────────────────────────────

    auto Accordion::find_section(const std::string_view id) const -> const Section* {
        const auto found = index_.find(std::string(id));
        return found == index_.end() ? nullptr : &sections_[found->second];
    }

    auto Accordion::find_section(const std::string_view id) -> Section* {
        const auto found = index_.find(std::string(id));
        return found == index_.end() ? nullptr : &sections_[found->second];
    }

    void Accordion::on_section_toggled(const std::string& id, const bool open) {
        const auto keep_alive = weak_from_this().lock();
        if (open && mode_ == AccordionMode::single) {
            enforce_single(id);
        }
        open_changed_.emit();
    }

    void Accordion::enforce_single(const std::string_view keep) {
        if (mode_ != AccordionMode::single) {
            return;
        }
        std::vector<std::shared_ptr<Collapsible>> close;
        close.reserve(sections_.size());
        for (auto& section: sections_) {
            if (section.descriptor.id == keep) {
                continue;
            }
            if (section.collapsible->open()) {
                close.push_back(section.collapsible);
            }
        }
        for (auto& collapsible: close) {
            collapsible->set_open(false);
        }
    }

    void Accordion::rebuild_index() {
        index_.clear();
        index_.reserve(sections_.size());
        for (std::size_t index = 0; index < sections_.size(); ++index) {
            index_.emplace(sections_[index].descriptor.id, index);
        }
    }
} // namespace nandina::widget
