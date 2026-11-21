//
// widget/accordion — vertically stacked disclosure sections with grouped state.
//
// experimental public header (batch 7). Composes Collapsible sections: the accordion adds
// stable section ids, single/multiple open state, optional Up/Down/Home/End focus roaming and
// an aggregate open event. It does not add a recipe: section separators are author-composed
// with Surface/Divider or custom nodes.
//
// Contract: docs/references/disclosure_components.md §7.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_ACCORDION_HPP
#define NANDINA_EXPERIMENT_WIDGET_ACCORDION_HPP

#include "../reactive/event.hpp"
#include "collapsible.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nandina::widget
{
    enum class AccordionMode : std::uint8_t {
        /// 至多一个 section 展开；允许全部收起（再次点击已展开项即收起）。
        single,
        /// 多个 section 可同时展开。
        multiple,
    };

    /// 一个 section 的构造描述。`id` 是稳定身份（非空、唯一）；`content` 必须是非空、
    /// 已脱离树、未设置 anchors 且不与 Accordion 形成祖先/后代环的控件。构造后由
    /// Accordion 内部的 Collapsible 持有。
    struct AccordionSection {
        std::string id;
        std::string title;
        std::shared_ptr<scene::NanControl> content;
        bool disabled = false;
    };

    /// 纵向堆叠的 disclosure 分组。容器本身不聚焦、不绘制；触发器是各 Collapsible 的
    /// Button，焦点/语义 expanded/禁用沿用 Collapsible。
    class Accordion final: public scene::NanControl {
    public:
        explicit Accordion(theme::NanTheme theme = theme::default_theme());
        ~Accordion() override;

        [[nodiscard]] static auto create(theme::NanTheme theme = theme::default_theme())
            -> std::shared_ptr<Accordion>;

        // ─── sections ───────────────────────────────────────────────────
        /// 事务性替换全部 section：先校验 id/title/content，再构建新 Collapsible；
        /// 相同 id 的 open 状态保留。程序化替换不发 open_changed。
        void set_sections(std::vector<AccordionSection> sections);
        void clear_sections();
        [[nodiscard]] auto sections() const -> std::vector<AccordionSection>;
        [[nodiscard]] auto section_count() const noexcept -> std::size_t;
        [[nodiscard]] auto section_node(std::string_view id) const -> const Collapsible*;
        [[nodiscard]] auto trigger_node(std::string_view id) const -> const Button*;
        /// 将焦点移到目标 section 的触发器；未知/禁用/脱离树时返回 false。
        auto focus_section(std::string_view id) -> bool;

        // ─── 展开状态 ───────────────────────────────────────────────────
        void set_mode(AccordionMode mode);
        [[nodiscard]] auto mode() const noexcept -> AccordionMode;
        /// 静默程序同步；未知 id 返回 false。single 模式下打开一项会静默关闭其它项。
        auto set_open(std::string_view id, bool open) -> bool;
        /// 用户式切换：经 Collapsible::toggle 发出 open_changed；未知 id 返回 false。
        auto toggle(std::string_view id) -> bool;
        [[nodiscard]] auto is_open(std::string_view id) const -> bool;
        /// 按 section 顺序返回当前展开的 id。
        [[nodiscard]] auto open_ids() const -> std::vector<std::string>;
        /// 静默批量设置；任一 id 未知则整体失败且不改状态。single 模式保留 section 序首个。
        auto set_open_ids(std::vector<std::string> ids) -> bool;
        void clear_open();
        /// 无载荷（用户动作触发）：订阅方重新读取 open_ids()。
        [[nodiscard]] auto open_changed() const noexcept -> const reactive::Event<>&;
        void set_on_open_changed(std::function<void()> callback);

        // ─── 单项更新 / 布局 ────────────────────────────────────────────
        auto set_section_disabled(std::string_view id, bool disabled) -> bool;
        auto set_section_title(std::string_view id, std::string title) -> bool;
        void set_gap(float gap);
        [[nodiscard]] auto gap() const noexcept -> float;
        void set_label(std::string label);
        [[nodiscard]] auto label() const -> std::string_view;

    protected:
        [[nodiscard]] auto on_measure(foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;
        auto on_input(scene::InputEvent& event) -> bool override;
        void on_theme_changed(const theme::ThemeManager& manager) override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        struct Section {
            AccordionSection descriptor;
            std::shared_ptr<Collapsible> collapsible;
            reactive::Subscription open_sub;
        };

        void validate_sections(const std::vector<AccordionSection>& sections) const;
        [[nodiscard]] auto prepare_sections(std::vector<AccordionSection> sections)
            -> std::vector<Section>;
        void apply_sections(std::vector<Section> sections);
        [[nodiscard]] auto find_section(std::string_view id) const -> const Section*;
        [[nodiscard]] auto find_section(std::string_view id) -> Section*;
        void on_section_toggled(const std::string& id, bool open);
        void enforce_single(std::string_view keep);
        void rebuild_index();

        std::vector<Section> sections_;
        std::unordered_map<std::string, std::size_t> index_;
        AccordionMode mode_ = AccordionMode::single;
        reactive::Event<> open_changed_;
        reactive::Subscription user_open_sub_;
        theme::NanTheme theme_;
        std::string label_;
        float gap_ = 0.0F;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_ACCORDION_HPP
