module;

#include <memory>
#include <thorvg-1/thorvg.h>
#include <variant>
#include <vector>

export module nandina.runtime.nan_widget;

export import nandina.runtime.nan_event;
export import nandina.foundation.nan_rect;
export import nandina.foundation.nan_size;
export import nandina.foundation.nan_constraints;

export namespace nandina::runtime {
    /**
     * @brief NanWidget — 所有 UI 元素的基类（M1）
     *
     * 负责：
     * - 空间属性（position / size / bounds）
     * - 组合模型（unique_ptr 树形所有权）
     * - 绘制遍历（draw → on_draw）
     * - 事件分发（dispatch_event / bubble_event）
     * - 可见性与命中测试（Issue 013 / 014）
     * - Dirty 状态传播（Issue 013）
     * - 布局协议（measure / layout / preferred_size / set_bounds / flex_factor / for_each_child）
     */
    class NanWidget {
    public:
        using Ptr = std::unique_ptr<NanWidget>;

        NanWidget()          {}
        virtual ~NanWidget() = default;

        // ── 空间属性 ──────────────────────────────────────────
        [[nodiscard]] auto x() const noexcept -> float {
            return m_x;
        }

        [[nodiscard]] auto y() const noexcept -> float {
            return m_y;
        }

        [[nodiscard]] auto width() const noexcept -> float {
            return m_width;
        }

        [[nodiscard]] auto height() const noexcept -> float {
            return m_height;
        }

        /** 返回当前 Widget 的包围盒（基于绝对坐标） */
        [[nodiscard]] auto bounds() const noexcept -> geometry::NanRect {
            return geometry::NanRect{geometry::NanPoint{m_x, m_y}, geometry::NanSize{m_width, m_height}};
        }

        auto set_position(const float x, const float y) noexcept -> void {
            m_x = x;
            m_y = y;
            mark_dirty();
        }

        auto set_size(const float w, const float h) noexcept -> void {
            m_width  = w;
            m_height = h;
            mark_dirty();
        }

        /**
         * @brief 设置位置和大小（布局系统最终分配入口）
         *
         * 由父布局容器调用，为 widget 分配最终的位置和尺寸。
         * 子类可覆盖此方法以实现自定义布局行为（如 Padding 对 child 进行收缩）。
         *
         * 注意：本方法仅负责 bounds 赋值，不执行子节点布局。
         * 子节点的布局由 layout() 方法在 measure() 之后调用。
         */
        virtual auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& {
            m_x = x;
            m_y = y;
            m_width  = w;
            m_height = h;
            m_layout_dirty = false; // bounds 已由父容器分配完毕
            mark_dirty();
            return *this;
        }

        // ── 可见性（Issue 013）────────────────────────────────
        [[nodiscard]] auto visible() const noexcept -> bool {
            return m_visible;
        }

        auto set_visible(const bool v) noexcept -> void {
            m_visible = v;
            mark_dirty();
        }

        // 是否参与命中测试
        // 设为 false 可使 Widget 不响应点击但保持可见
        [[nodiscard]] auto hit_test_visible() const noexcept -> bool {
            return m_hit_test_visible;
        }

        auto set_hit_test_visible(const bool v) noexcept -> void {
            m_hit_test_visible = v;
        }

        // ── Dirty 状态（Issue 013）────────────────────────────
        // 标记 Widget 需要重新布局/绘制
        [[nodiscard]] auto dirty() const noexcept -> bool {
            return m_dirty;
        }

        auto clear_dirty() noexcept -> void {
            m_dirty = false;
        }

        auto clear_dirty_recursive() noexcept -> void {
            m_dirty = m_layout_dirty;
            for (auto& child : m_children) {
                child->clear_dirty_recursive();
            }
        }

        auto mark_dirty() noexcept -> void {
            m_dirty = true;
            if (m_parent) {
                m_parent->mark_dirty();
            }
        }

        // ── 布局 Dirty 状态（Phase 1）────────────────────────
        /// 标记此 widget 需要重新执行 measure + layout
        auto mark_layout_dirty() noexcept -> void {
            m_layout_dirty = true;
            mark_dirty();
            if (m_parent) {
                m_parent->mark_layout_dirty();
            }
        }

        /// 查询是否需要重新 layout
        [[nodiscard]] auto is_layout_dirty() const noexcept -> bool {
            return m_layout_dirty;
        }

        // ── 父节点 ────────────────────────────────────────────
        [[nodiscard]] auto parent() const noexcept -> const NanWidget* {
            return m_parent;
        }

        [[nodiscard]] auto parent() noexcept -> NanWidget* {
            return m_parent;
        }

        // ── 组合能力 ──────────────────────────────────────────
        auto add_child(Ptr child) -> NanWidget* {
            if (!child)
                return nullptr;
            auto* ptr = child.get();
            ptr->m_parent = this;
            m_children.push_back(std::move(child));
            mark_layout_dirty();
            return ptr;
        }

        auto clear_children() -> void {
            for (auto& child : m_children) {
                child->m_parent = nullptr;
            }
            m_children.clear();
            mark_layout_dirty();
        }

        // ── 绘制 ──────────────────────────────────────────────
        virtual void draw(tvg::SwCanvas& canvas) {
            if (!m_visible)
                return;
            on_draw(canvas);
            for (const auto& child : m_children) {
                child->draw(canvas);
            }
        }

        // ── 事件分发（Issue 011/015）─────────────────────────
        // 将 Event 分发给对应的 on_xxx 虚方法。
        // 返回 true 表示事件已被消费。
        virtual auto dispatch_event(const Event& ev) -> bool {
            switch (event_type(ev)) {
            case EventType::PointerMove:
                return on_pointer_move(std::get<PointerMoveEvent>(ev));
            case EventType::PointerDown:
                return on_pointer_down(std::get<PointerButtonEvent>(ev));
            case EventType::PointerUp:
                return on_pointer_up(std::get<PointerButtonEvent>(ev));
            case EventType::PointerClick:
                return on_pointer_click(std::get<PointerButtonEvent>(ev));
            case EventType::PointerWheel:
                return on_pointer_wheel(std::get<PointerWheelEvent>(ev));
            case EventType::KeyDown:
                return on_key_down(std::get<KeyEvent>(ev));
            case EventType::KeyUp:
                return on_key_up(std::get<KeyEvent>(ev));
            case EventType::TextInput:
                return on_text_input(std::get<TextInputEvent>(ev));
            case EventType::FocusIn:
                return on_focus_in();
            case EventType::FocusOut:
                return on_focus_out();
            case EventType::WindowResize:
                return on_window_resize(std::get<WindowResizeEvent>(ev));
            case EventType::WindowClose:
                return on_window_close();
            }
            return false;
        }

        // ── 子事件冒泡 ────────────────────────────────────────
        // 将事件分发给所有子节点，直到某个子节点消费。
        // 返回 true 表示事件被某个子节点消费。
        [[nodiscard]] auto bubble_event(const Event& ev) const -> bool {
            for (auto& child : m_children) {
                if (child->dispatch_event(ev)) {
                    return true;
                }
            }
            return false;
        }

        // ── 命中测试（Issue 014）──────────────────────────────
        // 递归查找最上层（Z-order 最高）的命中子节点。
        //
        // 判定规则：
        //   1. 不可见 或 hit_test_visible=false → 不参与
        //   2. 坐标不在 bounds 内 → 不参与
        //   3. 逆序遍历子节点（后添加的在上层优先）
        //   4. 没有子节点命中时返回自身
        //
        // 返回 nullptr 表示未命中任何节点。
        [[nodiscard]] virtual auto hit_test(const float px, const float py) noexcept -> NanWidget* {
            if (!m_visible || !m_hit_test_visible) {
                return nullptr;
            }

            const auto rect = bounds();
            if (!rect.contains(geometry::NanPoint{px, py})) {
                return nullptr;
            }

            // 逆序遍历子节点（Z-order：后添加的在上层）
            for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
                auto* result = (*it)->hit_test(px, py);
                if (result) {
                    return result;
                }
            }

            // 没有子节点命中，返回自身
            return this;
        }

        // ── 子节点列表访问 ──
        [[nodiscard]] auto child_count() const noexcept -> std::size_t {
            return m_children.size();
        }

        [[nodiscard]] auto children() noexcept -> std::vector<Ptr>& {
            return m_children;
        }

        [[nodiscard]] auto children() const noexcept -> const std::vector<Ptr>& {
            return m_children;
        }

    public:
        // ── 两阶段布局协议（Phase 1）─────────────────────────

        /**
         * @brief 阶段一：测量 — 根据父容器约束计算自身所需尺寸
         *
         * 基类默认实现：
         *   1. 存储 m_measured_constraints
         *   2. 遍历子节点，调用每个子节点的 measure()
         *   3. 调用 calculate_intrinsic_size() 作为自身测量尺寸
         *
         * 子类可覆盖以调整向子节点传递的约束，或修改自身的测量逻辑。
         */
        virtual auto measure(const geometry::NanConstraints& constraints) -> void {
            m_measured_constraints = constraints;
            for_each_child([&](NanWidget& child) {
                child.measure(child_constraints_from(constraints, child));
            });
            m_measured_size = calculate_intrinsic_size(constraints);
            m_layout_dirty = false;
        }

        /**
         * @brief 阶段二：布局 — 在已分配的 bounds 内布置子节点位置
         *
         * 基类默认实现：
         *   1. 清除 m_layout_dirty
         *   2. 遍历子节点，递归调用每个子节点的 layout()
         *
         * 容器子类（LayoutContainer）覆盖此方法，调用 layout engine 计算子节点 frames。
         */
        virtual auto layout() -> void {
            m_layout_dirty = false;
            for_each_child([&](NanWidget& child) {
                child.layout();
            });
        }

        /**
         * @brief 子类可重写以调整传递给特定子节点的约束
         *
         * 例如 Flex 容器为不同子节点计算不同约束、Padding 收缩约束等。
         */
        [[nodiscard]] virtual auto child_constraints_from(
            const geometry::NanConstraints& parent_constraints,
            const NanWidget& /*child*/) const -> geometry::NanConstraints
        {
            return parent_constraints; // 默认透传
        }

        /**
         * @brief 计算在给定约束下的自身期望尺寸
         *
         * 基类默认实现：取所有子节点 preferred_size 的最大宽高，
         * 然后 clamp 到约束范围内。
         *
         * 叶子节点（如 Label、Spacer）应覆盖此方法返回真实需要的大小。
         */
        [[nodiscard]] virtual auto calculate_intrinsic_size(
            const geometry::NanConstraints& constraints) const -> geometry::NanSize
        {
            float child_max_w = 0.0f;
            float child_max_h = 0.0f;
            for_each_child([&](const NanWidget& child) {
                auto child_size = child.preferred_size();
                child_max_w = std::max(child_max_w, child_size.width());
                child_max_h = std::max(child_max_h, child_size.height());
            });

            const auto own_preferred = preferred_size();
            const float resolved_w = own_preferred.width() > 0.0f ? own_preferred.width() : child_max_w;
            const float resolved_h = own_preferred.height() > 0.0f ? own_preferred.height() : child_max_h;
            return {
                constraints.constrain_width(resolved_w),
                constraints.constrain_height(resolved_h)
            };
        }

        /// 返回最近一次 measure 的结果
        [[nodiscard]] auto measured_size() const noexcept -> const geometry::NanSize& {
            return m_measured_size;
        }

        /// 返回最近一次 measure 的约束
        [[nodiscard]] auto measured_constraints() const noexcept -> const geometry::NanConstraints& {
            return m_measured_constraints;
        }

    public:
        // ── 布局协议（兼容旧路径，子类可覆盖）────────────────

        /**
         * @brief 返回 widget 的自然（首选）尺寸
         *
         * 布局系统在分配空间时会参考此值。默认实现返回当前 size（如果 > 0）
         * 或零。子类（如 SizedBox）应覆盖以报告正确尺寸。
         *
         * 注意：建议新代码覆盖 calculate_intrinsic_size() 而非 preferred_size()，
         * 因为前者能感知约束。preferred_size() 保留为兼容 fallback。
         */
        [[nodiscard]] virtual auto preferred_size() const noexcept -> geometry::NanSize {
            return geometry::NanSize{m_width, m_height};
        }

        /**
         * @brief 弹性因子，供 Row/Column 等的 flex 布局使用
         *
         * 返回 0 表示不具有弹性（固定尺寸），> 0 表示扩展因子。
         * Spacer 和 Expanded 覆盖此方法返回各自的 flex 值。
         */
        [[nodiscard]] virtual auto flex_factor() const noexcept -> int {
            return 0;
        }

        /**
         * @brief 遍历所有直接子节点（只读）
         */
        template<typename F>
        auto for_each_child(F&& fn) -> void {
            for (auto& child : m_children) {
                if (child) {
                    std::forward<F>(fn)(*child);
                }
            }
        }

        /**
         * @brief 遍历所有直接子节点（const 版本）
         */
        template<typename F>
        auto for_each_child(F&& fn) const -> void {
            for (const auto& child : m_children) {
                if (child) {
                    std::forward<F>(fn)(*child);
                }
            }
        }

    protected:
        auto set_measured_layout_state(
            const geometry::NanConstraints& constraints,
            const geometry::NanSize& size) noexcept -> void {
            m_measured_constraints = constraints;
            m_measured_size = size;
            m_layout_dirty = false;
        }

        /// 仅清除 layout dirty 标志，不触发子节点 layout 递归。
        /// 在自定义 layout() 实现中，子节点已通过 set_bounds 链完成布局后，
        /// 用此方法代替 NanWidget::layout() 避免重复递归。
        auto clear_layout_dirty() noexcept -> void {
            m_layout_dirty = false;
        }

        // ── 自定义绘制（子类覆盖）────────────────────────────
        virtual void on_draw(tvg::SwCanvas& /*canvas*/) {
        }

        // ── 事件处理虚方法（默认不做响应）────────────────────
        virtual auto on_pointer_move(const PointerMoveEvent& /*event*/) -> bool {
            return false;
        }

        virtual auto on_pointer_down(const PointerButtonEvent& /*event*/) -> bool {
            return false;
        }

        virtual auto on_pointer_up(const PointerButtonEvent& /*event*/) -> bool {
            return false;
        }

        virtual auto on_pointer_click(const PointerButtonEvent& /*event*/) -> bool {
            return false;
        }

        virtual auto on_pointer_wheel(const PointerWheelEvent& /*event*/) -> bool {
            return false;
        }

        virtual auto on_key_down(const KeyEvent& /*event*/) -> bool {
            return false;
        }

        virtual auto on_key_up(const KeyEvent& /*event*/) -> bool {
            return false;
        }

        virtual auto on_text_input(const TextInputEvent& /*event*/) -> bool {
            return false;
        }

        virtual auto on_focus_in() -> bool {
            return false;
        }

        virtual auto on_focus_out() -> bool {
            return false;
        }

        virtual auto on_window_resize(const WindowResizeEvent& /*event*/) -> bool {
            return false;
        }

        virtual auto on_window_close() -> bool {
            return false;
        }

    private:
        float m_x{0.0f};
        float m_y{0.0f};
        float m_width{0.0f};
        float m_height{0.0f};

        NanWidget* m_parent{nullptr};
        bool m_visible{true};
        bool m_hit_test_visible{true};
        bool m_dirty{true}; // 初始为脏，首次绘制前需要 layout

        // ── 两阶段布局状态 ──
        geometry::NanConstraints m_measured_constraints{};
        geometry::NanSize m_measured_size{};
        bool m_layout_dirty{true};

        std::vector<Ptr> m_children;
    };
} // namespace nandina::runtime