//
// widget/roving_focus — 组内键盘漫游（roving focus）与 typeahead。
//
// 适用的组件形态：一组同类条目，同一时刻只有一个"当前项"，方向键在条目间移动。
// RadioGroup、Tabs、Select 的弹出列表、以及后续的 Menu / Combobox 都属于这一类。
// 此前 RadioGroup 与 Tabs 各写了一套（而且语义不同：一个移动控件焦点，一个只改选中
// 值），Select 又是第三套 —— 本类把"下一个是谁"抽出来，只回答焦点归属。
//
// 职责边界：
//   * 本类**不做**布局、绘制、事件派发，也不持有控件；
//   * 成员身份用**索引**表示（注册顺序 = 视觉顺序 = 方向键顺序），因此成员增删不会
//     留下悬垂指针；索引 → 控件的映射由容器负责；
//   * 按键命中时返回 Intent，**由容器决定怎么落地** —— 因为"成员是什么"（裸索引还是
//     控件指针）只有容器知道。
//
// 移动模型（RovingMovement）把两个彼此独立的决定命名清楚：
//   * 「控件焦点是否移动到目标成员」-> Intent::move_widget_focus；
//   * 「选中是否跟随焦点」        -> Intent::selection_follows_focus。
// 三种组合覆盖现有组件：
//   * focus_and_selection 两者都动（RadioGroup：焦点环跟着走，选中值也跟着改）；
//   * focus_only          只移动控件焦点，选中不动（ToggleGroup：Enter / Space 才改值）；
//   * selection_only      只改选中值，焦点留在组容器上（Tabs / Select 弹出列表）。
// 容器分别按这两个字段落地，不必自己记住"模式 -> 动作"的映射，也不必为了"只移动焦点"
// 而在容器里绕开枚举值。
//
// 输入入口分两类：
//   * handle_key / handle_text —— 真实输入路径；handle_key 按 RovingOrientation 过滤键码；
//   * step(delta)              —— 与键码无关的"沿配置轴走一步"，供程序化调用复用同一套
//                                 loop / 跳过不可聚焦项 / Intent 语义，调用方无需合成键码。
//
// 已知空白（本轮不做，见 docs/components/selection_and_navigation.md）：
//   * RTL：horizontal 方向在 RTL 下应反转左右键极性。项目已接 FriBidi，但三处现有
//     实现都未处理，故这里不引入，避免把行为变更混进重构；
//   * Tabs 目前是 selection_only，未对齐 ARIA tablist 的"焦点跟随"模式。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_ROVING_FOCUS_HPP
#define NANDINA_EXPERIMENT_WIDGET_ROVING_FOCUS_HPP

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::scene
{
    class KeyEvent;
    class TextInputEvent;
} // namespace nandina::scene

namespace nandina::widget
{
    /// 方向键的落地语义：命名「控件焦点是否移动」与「选中是否跟随」两个独立决定的组合。
    enum class RovingMovement {
        /// 只移动控件焦点，选中不跟随（ToggleGroup：方向键漫游不改值）。
        focus_only,
        /// 移动控件焦点，选中同时跟随（RadioGroup：焦点环与选中值一起走）。
        focus_and_selection,
        /// 不移动控件焦点，只改选中值，焦点留在组容器上（Tabs / Select 弹出列表）。
        selection_only,
    };

    /// 哪些方向键参与漫游。
    enum class RovingOrientation {
        vertical,
        horizontal,
        both,
    };

    class RovingFocus {
    public:
        /// 一次按键 / 步进被本类识别后的结果。
        struct Intent {
            /// 目标成员索引。
            int index = -1;
            /// 容器是否应当把控件焦点移到该成员。
            bool move_widget_focus = false;
            /// 容器是否应当让选中跟随焦点（把选中值改成 index）。
            bool selection_follows_focus = false;
        };

        RovingFocus() = default;

        // ─── 成员 ────────────────────────────────────────────────────────

        /**
         * 按视觉顺序重建成员表。容器在布局 / 数据变化后调用即可，
         * 不需要自己维护增删簿记。
         *
         * @param count          成员数量
         * @param accepts_focus  (i) -> 第 i 个成员是否可聚焦（默认全部可聚焦）
         * @param label          (i) -> 第 i 个成员用于 typeahead 的文本（可空）
         */
        void sync(
            std::size_t count,
            const std::function<bool(std::size_t)>& accepts_focus = {},
            const std::function<std::string_view(std::size_t)>& label = {}
        );

        [[nodiscard]] auto member_count() const noexcept -> std::size_t {
            return members_.size();
        }

        // ─── 配置 ────────────────────────────────────────────────────────

        void set_movement(RovingMovement movement) noexcept {
            movement_ = movement;
        }
        [[nodiscard]] auto movement() const noexcept -> RovingMovement {
            return movement_;
        }

        void set_orientation(RovingOrientation orientation) noexcept {
            orientation_ = orientation;
        }
        [[nodiscard]] auto orientation() const noexcept -> RovingOrientation {
            return orientation_;
        }

        /// 到边界后是否环绕（默认 true）。
        void set_loop(bool loop) noexcept {
            loop_ = loop;
        }
        [[nodiscard]] auto loop() const noexcept -> bool {
            return loop_;
        }

        /// typeahead 缓冲的存活时长；超过它再输入视为新的一次查找（默认 0.5 秒）。
        void set_typeahead_timeout(float seconds) noexcept;
        [[nodiscard]] auto typeahead_timeout() const noexcept -> float {
            return typeahead_timeout_;
        }

        // ─── 活动项 ──────────────────────────────────────────────────────

        /// 当前活动成员索引；无可用成员时返回 -1。
        [[nodiscard]] auto active_index() const -> int;

        /// 直接设置活动项（-1 表示清空）。不做可聚焦性校验。
        void set_active_index(int index);

        // ─── 输入 ────────────────────────────────────────────────────────

        /**
         * 处理按键。只有命中导航键（方向键 / Home / End / PageUp / PageDown）时才返回
         * Intent；其他键返回 nullopt，表示"不是我的键"，调用方继续按原逻辑处理
         * （Enter / Space / Escape 因此仍归组件自己）。
         *
         * 方向键按 orientation 过滤后走 `step()`；Home / End / PageUp / PageDown 跳首尾。
         */
        [[nodiscard]] auto handle_key(const scene::KeyEvent& event) -> std::optional<Intent>;

        /**
         * 沿配置的 orientation 轴移动一步，与具体键码无关：delta < 0 为上一个，
         * delta > 0 为下一个，delta == 0 视为"不动"。
         *
         * 遵守 `set_loop(...)`、跳过 `accepts_focus` 为 false 的成员；没有可移动的目标时
         * 返回 nullopt 且不改变 active_index()。命中时更新 active_index()、结束当前
         * typeahead 查找，并返回与 handle_key 同形的 Intent —— 容器按同一套
         * move_widget_focus / selection_follows_focus 落地即可。
         *
         * 程序化的"方向语义"入口应直接调用它，不要再按 orientation 合成上下 / 左右键。
         */
        [[nodiscard]] auto step(int delta) -> std::optional<Intent>;

        /// 处理文本输入（typeahead）。
        [[nodiscard]] auto handle_text(const scene::TextInputEvent& event)
            -> std::optional<Intent>;

        /// typeahead 缓冲超时推进。容器在 on_process(dt) 里调用。
        void advance_time(float dt);

        /// 当前 typeahead 缓冲（供测试与可访问性读取）。
        [[nodiscard]] auto typeahead_buffer() const -> std::string_view {
            return buffer_;
        }

        /// 清空 typeahead 缓冲（例如失焦时）。
        void reset_typeahead();

    private:
        struct Member {
            bool accepts_focus = true;
            std::function<std::string_view(std::size_t)> label;
        };

        [[nodiscard]] auto focusable(std::size_t index) const -> bool;
        /// 从 `from` 出发沿 `direction` 找下一个可聚焦成员；找不到返回 -1。
        [[nodiscard]] auto next_focusable(int from, int direction) const -> int;
        /// 首个 / 最后一个可聚焦成员；没有时返回 -1。
        [[nodiscard]] auto edge_focusable(bool from_end) const -> int;
        /// 首个 / 末尾 matched 标签以当前缓冲开头的成员；没有时返回 -1。
        [[nodiscard]] auto typeahead_match() const -> int;
        [[nodiscard]] auto label_of(std::size_t index) const -> std::string_view;
        [[nodiscard]] auto make_intent(int index) const -> Intent;
        [[nodiscard]] auto has_members() const noexcept -> bool {
            return !members_.empty();
        }

        std::vector<Member> members_;
        int active_ = 0;
        RovingMovement movement_ = RovingMovement::selection_only;
        RovingOrientation orientation_ = RovingOrientation::vertical;
        bool loop_ = true;
        float typeahead_timeout_ = 0.5F;
        float typeahead_elapsed_ = 0.0F;
        std::string buffer_;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_ROVING_FOCUS_HPP
