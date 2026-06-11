module;

#include <functional>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

export module nandina.layout.positioned;

export import nandina.foundation.nan_rect;
export import nandina.foundation.nan_size;
export import nandina.foundation.nan_constraints;
export import nandina.runtime.nan_widget;

export namespace nandina::layout {

// ── AnchorExpr ───────────────────────────────────────────────────────────────
// 单个 anchor 属性的值：未设置 / 静态 float / 响应式 lambda
using AnchorExpr = std::variant<std::monostate, float, std::function<float()>>;

[[nodiscard]] inline auto anchor_has_value(const AnchorExpr& a) noexcept -> bool {
    return !std::holds_alternative<std::monostate>(a);
}

[[nodiscard]] inline auto anchor_resolve(const AnchorExpr& a) noexcept -> float {
    if (const auto* v = std::get_if<float>(&a))
        return *v;
    if (const auto* fn = std::get_if<std::function<float()>>(&a))
        return (*fn)();
    return 0.0f;
}

// ── GeomRef ──────────────────────────────────────────────────────────────────
// 对已布置子组件几何信息的只读引用。
// Positioned::layout() 在布置完每个子节点后填充此结构；
// 后续子节点的 anchor lambda 可读取其值（如 .anchor_top([&ref]{ return ref.bottom() + 8; })）。
// 坐标系：相对于 Positioned 容器的左上角（0,0）。
class GeomRef {
public:
    [[nodiscard]] auto is_valid()  const noexcept -> bool  { return m_valid; }
    [[nodiscard]] auto left()      const noexcept -> float { return m_left; }
    [[nodiscard]] auto top()       const noexcept -> float { return m_top; }
    [[nodiscard]] auto width()     const noexcept -> float { return m_width; }
    [[nodiscard]] auto height()    const noexcept -> float { return m_height; }
    [[nodiscard]] auto right()     const noexcept -> float { return m_left + m_width; }
    [[nodiscard]] auto bottom()    const noexcept -> float { return m_top + m_height; }
    [[nodiscard]] auto center_x()  const noexcept -> float { return m_left + m_width / 2.0f; }
    [[nodiscard]] auto center_y()  const noexcept -> float { return m_top + m_height / 2.0f; }

private:
    friend class Positioned;
    auto update(float l, float t, float w, float h) noexcept -> void {
        m_left = l; m_top = t; m_width = w; m_height = h;
        m_valid = true;
    }

    float m_left{};
    float m_top{};
    float m_width{};
    float m_height{};
    bool  m_valid{false};
};

// ── PositionedProps ──────────────────────────────────────────────────────────
// 一个子组件在 Positioned 容器内的 anchor 配置。
//
// Anchor 值语义（相对于父容器，0 = 边缘）：
//   left   — 子组件左边缘距父容器左边缘的距离
//   top    — 子组件上边缘距父容器上边缘的距离
//   right  — 子组件右边缘距父容器右边缘的距离（向内量）
//   bottom — 子组件下边缘距父容器下边缘的距离（向内量）
//   width  — 子组件显式宽度（当不能从 left+right 推导时）
//   height — 子组件显式高度（当不能从 top+bottom 推导时）
//
// 解析优先级（每轴独立，同 QML）：
//   left + right  → x=left,  w=parent.w - left - right
//   left + width  → x=left,  w=width
//   right + width → w=width, x=parent.w - right - w
//   left only     → x=left,  w=preferred_w
//   right only    → w=preferred_w, x=parent.w - right - w
//   width only    → w=width, x=(parent.w - w)/2  （居中）
//   none          → x=0, w=preferred_w
//   （top/bottom/height 对称）
struct PositionedProps {
    AnchorExpr left{};
    AnchorExpr top{};
    AnchorExpr right{};
    AnchorExpr bottom{};
    AnchorExpr width{};
    AnchorExpr height{};
    // 可选：布置完成后将实际 bounds 写入此 GeomRef，供后续兄弟节点引用
    GeomRef* geom_ref{nullptr};
};

// ── Positioned ───────────────────────────────────────────────────────────────
// 自由定位容器：子组件通过 anchor 表达式声明相对于父容器（或兄弟节点）的位置。
// 子组件按声明顺序布置；后声明的子节点可通过 GeomRef 引用先声明兄弟的 bounds。
class Positioned final : public runtime::NanWidget {
public:
    [[nodiscard]] static auto Create() -> std::unique_ptr<Positioned> {
        return std::make_unique<Positioned>();
    }

    // 添加带 anchor 配置的子组件（由 positioned() factory 调用）
    auto add_positioned_child(runtime::NanWidget::Ptr child,
                              PositionedProps props) -> void {
        auto* raw = child.get();
        add_child(std::move(child));
        m_props.push_back(std::move(props));
        m_child_ptrs.push_back(raw);
    }

    // Positioned 容器自身不约束尺寸：填满父容器分配的全部 bounds
    [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
        return geometry::NanSize{};
    }

    // 阶段一：先以 loose 约束 measure 每个子节点（获取 preferred_size）
    auto measure(const geometry::NanConstraints& constraints) -> void override {
        const auto child_constraints = constraints.loosen();
        for (auto* child : m_child_ptrs) {
            child->measure(child_constraints);
        }
        const auto w = constraints.max_width();
        const auto h = constraints.max_height();
        set_measured_layout_state(constraints, geometry::NanSize{w, h});
    }

    // 阶段二：解析每个子节点的 anchor，调用 set_bounds + layout
    auto layout() -> void override {
        const float pw = width();
        const float ph = height();
        const float px = x();
        const float py = y();

        for (std::size_t i = 0; i < m_child_ptrs.size(); ++i) {
            auto* child         = m_child_ptrs[i];
            const auto& props   = m_props[i];
            const auto  pref    = measured_or_preferred(*child);

            auto [rx, rw] = resolve_axis(pw, props.left,   props.right,  props.width,  pref.width());
            auto [ry, rh] = resolve_axis(ph, props.top,    props.bottom, props.height, pref.height());

            // rx/ry 为相对于父容器的坐标，set_bounds 需要绝对坐标
            child->set_bounds(px + rx, py + ry, rw, rh);
            child->layout();

            if (props.geom_ref) {
                // 写入相对坐标，便于兄弟节点 anchor lambda 直接使用
                props.geom_ref->update(rx, ry, rw, rh);
            }
        }
        clear_layout_dirty();
    }

private:
    // 取 measured_size 或 preferred_size，保证非零
    [[nodiscard]] static auto measured_or_preferred(
        const runtime::NanWidget& child) noexcept -> geometry::NanSize {
        const auto ms = child.measured_size();
        if (ms.width() > 0.0f || ms.height() > 0.0f) return ms;
        return child.preferred_size();
    }

    // 单轴 anchor 解析，返回 (origin_relative_to_parent, size)
    [[nodiscard]] static auto resolve_axis(
        float parent_size,
        const AnchorExpr& start,
        const AnchorExpr& end,
        const AnchorExpr& sz,
        float preferred) noexcept -> std::pair<float, float> {
        const bool hs = anchor_has_value(start);
        const bool he = anchor_has_value(end);
        const bool hw = anchor_has_value(sz);

        if (hs && he) {
            const float s = anchor_resolve(start);
            const float e = anchor_resolve(end);
            return {s, parent_size - s - e};
        }
        if (hs && hw) {
            return {anchor_resolve(start), anchor_resolve(sz)};
        }
        if (he && hw) {
            const float w = anchor_resolve(sz);
            return {parent_size - anchor_resolve(end) - w, w};
        }
        if (hs) {
            return {anchor_resolve(start), hw ? anchor_resolve(sz) : preferred};
        }
        if (he) {
            const float w = hw ? anchor_resolve(sz) : preferred;
            return {parent_size - anchor_resolve(end) - w, w};
        }
        if (hw) {
            const float w = anchor_resolve(sz);
            return {(parent_size - w) / 2.0f, w};   // 仅 size：居中
        }
        // 无任何约束：使用 preferred size，锚点在原点
        return {0.0f, preferred};
    }

    std::vector<PositionedProps>     m_props;
    std::vector<runtime::NanWidget*> m_child_ptrs;  // 原始指针，所有权在父类 m_children
};

}  // namespace nandina::layout
