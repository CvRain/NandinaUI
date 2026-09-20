//
// widget/toggle_group - shared selection + roaming coordination for Toggle members.
//
// 与 RadioGroup 同形：一个协调对象（不是场景节点），跟踪成员的注册顺序，按模式维护
// 选中集合，并驱动方向键漫游。Toggle 通过 shared_ptr 持有组，因此组与成员同生命周期。
//
// 与 RadioGroup 的差别只有两点：
//   * 两种选中模式（single / multiple），single 下"点已选中的成员"是否允许取消由
//     `allow_empty` 决定；
//   * 方向键漫游只移动**焦点**，不改选中值 —— toggle 的值语义属于显式激活
//     （Enter / Space / 点击），漫游不应隐式改写用户的选择。
//
// 漫游本身**不在这里实现**：成员顺序 ↔ 索引、跳过 disabled 成员、循环、Home/End、
// typeahead 全部由共享的 `RovingFocus` 回答，本类只负责把索引映射回 Toggle 指针并落地
// 焦点。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_TOGGLE_GROUP_HPP
#define NANDINA_EXPERIMENT_WIDGET_TOGGLE_GROUP_HPP

#include "../reactive/event.hpp"
#include "../theme/design_system.hpp"
#include "roving_focus.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace nandina::scene
{
    class KeyEvent;
    class TextInputEvent;
} // namespace nandina::scene

namespace nandina::theme
{
    class ThemeManager;
} // namespace nandina::theme

namespace nandina::widget
{
    class Toggle;

    /** 组的选中模式。 */
    enum class ToggleGroupMode: std::uint8_t {
        /** 至多一个成员选中（选择新成员会取消旧成员）。 */
        single,
        /** 成员各自独立开关，组只报告选中集合。 */
        multiple,
    };

    class ToggleGroup {
    public:
        ToggleGroup();

        [[nodiscard]] static auto create() -> std::shared_ptr<ToggleGroup>;

        // ─── 成员 ────────────────────────────────────────────────────────

        /// 成员注册（Toggle 构造 / set_group 时调用，保持注册顺序 = 视觉顺序）。
        void register_toggle(Toggle* toggle);
        /// 成员注销（Toggle 析构 / set_group 换组时调用）。
        void unregister_toggle(Toggle* toggle);
        [[nodiscard]] auto member_count() const -> std::size_t;
        [[nodiscard]] auto index_of(const Toggle* toggle) const -> int;

        // ─── 选中 ────────────────────────────────────────────────────────

        void set_mode(ToggleGroupMode mode);
        [[nodiscard]] auto mode() const -> ToggleGroupMode;

        /**
         * single 模式下是否允许取消最后一个选中项（默认 `true`）。
         *
         * 默认值与工具栏习惯一致：再点一次已按下的 Bold 就恢复未选中。设为 false 时
         * single 模式保证恰好一个选中（初值仍需宿主自行 select()）。
         */
        void set_allow_empty(bool allow_empty);
        [[nodiscard]] auto allow_empty() const -> bool;

        /// 当前选中成员的索引集合，升序（注册顺序）。
        [[nodiscard]] auto checked_indices() const -> std::vector<int>;

        /**
         * 程序化选中：single 模式先取消其它成员再选中目标，multiple 模式只打开目标。
         * 与 RadioGroup::select 一致，走 `set_checked`（不发成员事件），只发组事件。
         */
        void select(int index);

        /// 选中集合变化事件（用户操作或 `select` 触发），载荷 = 变化后的索引集合。
        [[nodiscard]] auto selection_changed() const -> const reactive::Event<std::vector<int>>&;

        /**
         * 成员的用户激活入口（由 `Toggle::toggle()` 调用）。按当前模式落地：
         * single 先取消其它成员，`allow_empty == false` 时忽略"取消唯一选中项"的请求。
         */
        void toggle_member(Toggle* toggle);

        // ─── 漫游（全部委托给共享的 RovingFocus） ─────────────────────────

        /**
         * 成员按键入口：把按键原样交给共享的漫游设施，由它回答"下一个是谁"、
         * 按 orientation 过滤方向键，并处理 Home / End / PageUp / PageDown。
         * 命中后把焦点交给目标成员（方向键只移动焦点，不改选中值）。
         */
        [[nodiscard]] auto handle_key(Toggle* from, const scene::KeyEvent& event) -> bool;

        /**
         * 方向键漫游（方向语义入口，供程序化调用）：direction -1 上一个 / +1 下一个，
         * 到边界循环。
         *
         * 用户输入路径请用 `handle_key` —— 它把原始按键交给 RovingFocus，因此
         * orientation（纵向 / 横向 / 双向）决定哪些键参与漫游；本入口只有"方向"，
         * 所以按当前 orientation 合成一个对应轴上的按键再委托给它。
         */
        [[nodiscard]] auto move_focus(Toggle* from, int direction) -> bool;

        /**
         * typeahead：按成员文本前缀移动焦点（不改变选中值）。
         *
         * 与 `handle_key` 同款：命中后把焦点交给目标成员。
         */
        [[nodiscard]] auto handle_text(Toggle* from, const scene::TextInputEvent& event) -> bool;

        /// typeahead 缓冲超时推进（组不是节点，由持焦点的成员在 on_process 里代为转发）。
        void advance_time(float dt);

        /// 当前 typeahead 缓冲（委托给 `RovingFocus`；供测试与可访问性读取）。
        [[nodiscard]] auto typeahead_buffer() const -> std::string_view;

        /// 哪些方向键参与漫游（委托给 RovingFocus，不重复定义方向枚举）。
        void set_orientation(RovingOrientation orientation);
        [[nodiscard]] auto orientation() const -> RovingOrientation;

        // ─── 主题 ────────────────────────────────────────────────────────

        /// 高级接口：以完整 NanTheme 覆盖组主题（不再跟随系统切换）。
        void set_theme(theme::NanTheme theme);
        /// 当前生效主题视图（tokens + 当前外观 palette），遗留读取兼容。
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        /// 类型化字段覆盖：只覆盖明确指定的配方字段，系统切换后保留并重解析。
        void set_override(theme::ToggleGroupRecipeRule rule);
        [[nodiscard]] auto visual_state() const -> theme::ToggleGroupVisualState;
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedToggleGroupStyle;
        /// 组不是节点：系统主题切换时由成员代为转发（幂等）。
        void on_theme_changed(const theme::ThemeManager& manager);

    private:
        /// 把成员表重建进共享漫游设施（成员增删后、每次按键前调用）。
        void sync_focus();

        std::vector<Toggle*> members_;
        /// 组内漫游由共享设施负责：注册顺序即视觉顺序，索引 ↔ 控件由本类映射。
        RovingFocus focus_;
        ToggleGroupMode mode_ = ToggleGroupMode::single;
        bool allow_empty_ = true;

        /// 解析用的设计系统快照（detached 默认 = 框架默认主题）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        theme::NanTheme theme_view_;
        std::optional<theme::ToggleGroupRecipeRule> override_;
        /// set_theme(NanTheme) 整份覆盖后不再跟随系统切换。
        bool system_explicit_ = false;

        reactive::Event<std::vector<int>> selection_changed_;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_TOGGLE_GROUP_HPP
