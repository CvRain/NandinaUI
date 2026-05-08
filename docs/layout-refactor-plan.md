# Layout 系统重构方案

> 基于项目当前的 Layout 架构分析，提出具体可执行的重构方案。
> 目标：消除手动布局计算，引入 measure/layout 两阶段协议，统一组件布局行为。

---

## 目录

1. [Phase 0：LayoutConstraints 类型定义](#phase-0layoutconstraints-类型定义)
2. [Phase 1：NanWidget 基类增加 measure/layout 协议](#phase-1nanwidget-基类增加-measurelayout-协议)
3. [Phase 2：LayoutCore 升级 — 约束感知引擎](#phase-2layoutcore-升级--约束感知引擎)
4. [Phase 3：LayoutContainer 改用新协议](#phase-3layoutcontainer-改用新协议)
5. [Phase 4：FlexWidgets 适配](#phase-4flexwidgets-适配)
6. [Phase 5：Pressable 去布局化](#phase-5pressable-去布局化)
7. [Phase 6：Button 采用组合而非布局层叠](#phase-6button-采用组合而非布局层叠)
8. [Phase 7：Card 使用内部 Column 容器消除手算](#phase-7card-使用内部-column-容器消除手算)
9. [Phase 8：Showcase 消除硬编码尺寸](#phase-8showcase-消除硬编码尺寸)

---

## Phase 0：引入 NanConstraints + 扩展 LayoutChildSpec

**文件**：`layout/src/layout_core.cppm`

### 背景

`foundation/src/nan_constraints.cppm` 中已存在 `nandina::geometry::NanConstraints`，提供了完整的约束能力：
- `tight(size)` / `loose(size)` / `expand()` 工厂方法
- `is_bounded()` / `has_unbounded_width()` 查询
- `constrain(NanSize)` / `constrain_width()` / `constrain_height()` 适配
- `min_size()` / `max_size()` 边界获取

因此无需重新定义 `LayoutConstraints`，Phase 0 的核心工作是将 `NanConstraints` 导入布局模块，并扩展 `LayoutChildSpec`。

### LayoutChildSpec 扩展为：

```cpp
struct LayoutChildSpec {
    geometry::NanSize preferred_size{};
    geometry::NanSize min_size{};          // 新增
    geometry::NanSize max_size{};          // 新增
    int flex_factor = 0;
    bool can_shrink = true;                // 新增，默认允许 shrink
};
```

---

## Phase 1：NanWidget 基类增加 measure/layout 协议

**文件**：`runtime/src/nan_widget.cppm`

### 新增虚方法

```cpp
// 文件：runtime/src/nan_widget.cppm 中 NanWidget 类

/// 阶段一：测量 — 通知子节点可用约束，收集尺寸需求。
/// 基类默认实现：遍历子节点，传递约束，更新 m_measured_size。
/// 子类可 override 来调整向子节点传递的约束。
virtual auto measure(const LayoutConstraints& constraints) -> void {
    m_measured_constraints = constraints;
    for_each_child([&](NanWidget& child) {
        child.measure(adjust_constraints_for_child(constraints, child));
    });
    m_measured_size = calculate_intrinsic_size(constraints);
}

/// 阶段二：布局 — 父容器分配好 bounds 后，子节点递归布局。
/// 基类默认实现：什么也不做（叶子节点不需要布局）。
/// 容器子类 override 此方法，调用 layout engine 分配子节点位置。
virtual auto layout() -> void {
    // 默认空实现 — 叶子节点布局就是 set_bounds 本身
}
```

### 辅助方法

```cpp
/// 子类可重写，以调整传递给特定子节点的约束
/// （例如 Flex 容器为不同子节点计算不同约束）
[[nodiscard]] virtual auto adjust_constraints_for_child(
    const LayoutConstraints& parent_constraints,
    const NanWidget& child) const -> LayoutConstraints
{
    return parent_constraints; // 默认透传
}

/// 默认的 intrinsic size 计算：取所有子节点 preferred_size 的最大宽高
/// 叶子节点（如 Label）应 override 此方法
[[nodiscard]] virtual auto calculate_intrinsic_size(
    const LayoutConstraints& constraints) const -> geometry::NanSize
{
    float max_w = 0.0f, max_h = 0.0f;
    for_each_child([&](const NanWidget& child) {
        auto s = child.preferred_size();
        max_w = std::max(max_w, s.width());
        max_h = std::max(max_h, s.height());
    });
    return {constraints.clamp_width(max_w), constraints.clamp_height(max_h)};
}
```

### 新成员变量

```cpp
// 新增到 NanWidget 类
private:
    LayoutConstraints m_measured_constraints;
    geometry::NanSize m_measured_size{};
    bool m_layout_dirty = true;

public:
    [[nodiscard]] auto measured_size() const noexcept -> const geometry::NanSize& {
        return m_measured_size;
    }

    auto mark_layout_dirty() -> void { m_layout_dirty = true; }
    [[nodiscard]] auto is_layout_dirty() const noexcept -> bool { return m_layout_dirty; }
```

### set_bounds 方法修改

`set_bounds` 回归纯 setter，不再执行布局计算：

```cpp
auto set_bounds(float x, float y, float w, float h) noexcept -> NanWidget& override {
    m_bounds = geometry::NanRect{x, y, x + w, y + h};
    m_layout_dirty = false; // bounds 已设置完毕
    return *this;
}
```

### 移除原有的虚方法

- 保留 `preferred_size()` 作为兼容 fallback（measure 未实现时的退化路径）
- 新增的 `measure()` 是首选路径

---

## Phase 2：LayoutCore 升级 — 约束感知引擎

**文件**：`layout/src/layout_core.cppm`

### LayoutRequest 扩展

```cpp
struct LayoutRequest {
    LayoutAxis axis = LayoutAxis::column;
    geometry::NanRect container_bounds{};
    LayoutConstraints constraints{};      // 新增：父容器约束
    LayoutInsets padding{};
    float gap = 0.0f;
    LayoutAlignment cross_alignment = LayoutAlignment::start;
    LayoutAlignment main_alignment  = LayoutAlignment::start;
    std::vector<LayoutChildSpec> children;

    // content_bounds() 保持不变
    [[nodiscard]] auto content_bounds() const noexcept -> geometry::NanRect {
        // ... 同现有 ...
    }
};
```

### compute_* 方法接收约束

以 `compute_column` 为例，修改 flex 子节点的尺寸计算：

```cpp
// 在 compute_column / compute_row 中：
// 当 child.flex_factor > 0 时：
//   1. flex 子节点的 main-axis 尺寸由 flex_remaining 分配
//   2. flex 子节点的 cross-axis 尺寸由 constraints 决定
//   3. 分配后的尺寸必须 clamp 到 child.min_size / child.max_size

// 改造 resolve_cross_extent 增加约束参数：
[[nodiscard]] inline auto resolve_cross_extent(
    const LayoutAlignment align,
    const float available,
    const float desired,
    const geometry::NanSize& min_size,      // 新增
    const geometry::NanSize& max_size       // 新增
) noexcept -> float
{
    float result;
    if (align == LayoutAlignment::stretch) {
        result = available;
    } else {
        result = std::min(desired, available);
    }
    // 应用 min/max 约束
    result = std::clamp(result,
        align == LayoutAlignment::stretch ? 0.0f : min_size.width(),
        max_size.width());
    return std::max(0.0f, result);
}
```

### 增加 shrink 支持

```cpp
// 在剩余空间不足时，flex_total > 0 且有 can_shrink=true 的子节点会缩小
float shrink_remaining = 0.0f;
if (used_h > avail_h && flex_total > 0) {
    shrink_remaining = used_h - avail_h;
    // 按 flex_factor 比例从 flex 子节点减去 shrink_remaining
}
```

---

## Phase 3：LayoutContainer 改用新协议

**文件**：`layout/src/layout_container.cppm`

### LayoutContainer::measure()

```cpp
auto measure(const LayoutConstraints& constraints) -> void override {
    // 1. 计算 padding 占用的空间
    float pad_h = padding_left_ + padding_right_;
    float pad_v = padding_top_ + padding_bottom_;

    // 2. 子节点可用空间
    auto child_constraints = LayoutConstraints{
        .min_width  = 0.0f,
        .max_width  = constraints.max_width - pad_h,
        .min_height = 0.0f,
        .max_height = constraints.max_height - pad_v,
    };

    // 3. 收集子节点规格
    m_child_specs.clear();
    for_each_child([&](NanWidget& child) {
        child.measure(child_constraints);
        auto ms = child.measured_size();
        m_child_specs.push_back({
            .preferred_size = ms,
            .flex_factor    = child.flex_factor(),
        });
    });

    // 4. 计算自身 intrinsic size（同现有 preferred_size 逻辑）
    // ... 同现有 LayoutContainer::preferred_size()
    m_measured_size = compute_measured_size();
}
```

### LayoutContainer::layout()

```cpp
auto layout() -> void override {
    // 1. 收集当前约束下的子节点 specs（已由 measure 阶段准备）
    // 2. 调用 layout backend 计算 frames
    // 3. 应用 frames 到子节点
    auto targets  = collect_child_targets();
    auto request  = build_layout_request(layout_axis());
    auto frames   = default_layout_backend().compute(request);

    size_t count = std::min(targets.size(), frames.size());
    for (size_t i = 0; i < count; ++i) {
        auto* child = targets[i];
        const auto& frame = frames[i];
        child->set_bounds(frame.x(), frame.y(), frame.width(), frame.height());
        child->layout();  // 递归布局
    }
}
```

### LayoutContainer::set_bounds() 精简

```cpp
auto set_bounds(float x, float y, float w, float h) noexcept -> NanWidget& override {
    NanWidget::set_bounds(x, y, w, h);
    // 不再直接调 layout() — layout 由外部触发
    return *this;
}
```

### Row/Column/Stack 的 measure 重写

```cpp
// Row 重写以在 cross-axis 传递 stretch 约束
auto measure(const LayoutConstraints& constraints) -> void override {
    LayoutContainer::measure(constraints);
    // Row 的 cross-axis = height，stretch 时强制子节点高度填满
    if (align_items_ == LayoutAlignment::stretch) {
        for_each_child([&](NanWidget& child) {
            // 不需要额外操作，backend 的 resolve_cross_extent 已处理 stretch
        });
    }
}
```

### add() 触发 measure/layout 重排

```cpp
auto add(std::unique_ptr<NanWidget> child) -> LayoutContainer& {
    add_child(std::move(child));
    mark_layout_dirty();
    request_layout();
    return *this;
}
```

其中 `request_layout()` 改为：

```cpp
auto request_layout() -> void {
    mark_dirty();
    mark_layout_dirty();
    // 如果有 bounds，触发 measure → layout 链
    if (width() > 0.0f || height() > 0.0f) {
        auto constraints = LayoutConstraints::tight(width(), height());
        measure(constraints);
        layout();
    }
}
```

---

## Phase 4：FlexWidgets 适配

**文件**：`layout/src/flex_widgets.cppm`

### Center 改为使用 measure/layout

```cpp
class Center : public runtime::NanWidget {
public:
    auto measure(const LayoutConstraints& constraints) -> void override {
        m_child_constraints = constraints;
        if (auto* child = get_only_child()) {
            child->measure(constraints);
        }
        m_measured_size = {
            constraints.clamp_width(child ? child->measured_size().width() : 0.0f),
            constraints.clamp_height(child ? child->measured_size().height() : 0.0f)
        };
    }

    auto layout() -> void override {
        auto* child = get_only_child();
        if (!child) return;

        auto child_size = child->measured_size();
        float x = (m_bounds.width() - child_size.width()) / 2.0f;
        float y = (m_bounds.height() - child_size.height()) / 2.0f;

        child->set_bounds(x, y, child_size.width(), child_size.height());
        child->layout();
    }

    // set_bounds 只需设置自身，不手算子节点
    auto set_bounds(float x, float y, float w, float h) noexcept -> NanWidget& override {
        NanWidget::set_bounds(x, y, w, h);
        return *this;
    }
};
```

### Expanded 同理

Expanded 只需设置 `flex_factor`，由父容器在 `measure()` 阶段处理。

### Padding

Padding 在 measure 阶段减少传递给子节点的约束，在 layout 阶段偏移子节点位置。

---

## Phase 5：Pressable 去布局化

**文件**：`widgets/src/nan_pressable.cppm`

核心变化：Pressable **不再继承 NanWidget**，改为纯组合状态机类。

```cpp
// Pressable — 纯状态机，不参与布局
class Pressable {
public:
    using Ptr = std::unique_ptr<Pressable>;
    using Callback = std::function<void()>;

    static auto create() -> Ptr { return Ptr{new Pressable()}; }

    // ── 事件处理（由持有者委托调用） ──
    auto handle_pointer_move(const PointerMoveEvent& event) -> bool { /* ... */ }
    auto handle_pointer_down(const PointerButtonEvent& event) -> bool { /* ... */ }
    auto handle_pointer_up(const PointerButtonEvent& event) -> bool { /* ... */ }

    // ── 状态查询 ──
    [[nodiscard]] auto state() const noexcept -> PressableState { /* ... */ }

    // ── 回调注册（同上） ──
    auto on_click(Callback cb) -> Pressable& { /* ... */ }
    // ... 其他回调

private:
    bool m_hovered{false};
    bool m_pressed_inside{false};
    bool m_focused{false};
    reactive::State<bool> m_disabled{false};
    // 回调 ...
};
```

**这样改动的好处**：
- 不再有 `Pressable::set_bounds()` 透传问题
- Button/Surface 内部持有 Pressable 实例，在 `on_pointer_*` 中委托
- 不增加布局层级

---

## Phase 6：Button 采用组合而非布局层叠

**文件**：`widgets/src/nan_button.cppm` + `widgets/src/nan_button.cpp`

### Button 的新结构

```
Button (继承 Surface)
  ├── Center (flex_widgets)
  │     └── Label
  └── [持有 Pressable 实例，非 Widget]
```

### 事件委托

```cpp
// Button 从 Surface 继承事件方法，转发给 Pressable
auto Button::on_pointer_down(const PointerButtonEvent& event) -> bool override {
    m_pressable->handle_pointer_down(event);
    update_visual_state();
    return true;
}
```

### 构造代码

```cpp
Button::Button() {
    set_corner_radius(m_colors.corner_radius);
    set_padding(m_colors.padding);
    set_bg_color(m_colors.bg);

    // 创建 Center + Label (唯一布局子节点)
    auto center = layout::Center::Create();
    auto label = Label::create();
    m_label = label.get();
    center->add_child(std::move(label));
    add_child(std::move(center));

    // Pressable 非 widget，不参与 add_child
    m_pressable = Pressable::create();
    m_pressable->on_click([this] {
        if (m_on_click) m_on_click();
        m_clicked_signal.emit();
    });
}
```

---

## Phase 7：Card 使用内部 Column 容器消除手算

**文件**：`widgets/src/nan_card.cppm`

### Card 内部结构

```
Card (继承 Surface)
  └── Column (内部布局容器)
        ├── TitleRow (Row: [accent_bar] + Label + spacer)
        ├── Divider (thin Shape widget)
        └── ContentSlot (占位，开发者 add_child 的子节点放这里)
```

### 关键实现

```cpp
Card::Card() : Surface() {
    m_bg_color.set(NanColor::from(NanRgb{50, 52, 72}));
    m_corner_radius.set(8.0f);

    // 内部布局容器
    auto column = Column::Create();
    column->gap(0.0f);
    // Column 作为内部容器 — 隐藏于 Card 内部

    // 标题行
    auto title_row = Row::Create();
    title_row->align_items(LayoutAlignment::center);
    // accent_bar + label
    auto accent = create_accent_bar();
    auto label = Label::create();
    m_title_label = label.get();
    title_row->add(std::move(accent));
    title_row->add(std::move(label));

    // 分隔线
    auto divider = Divider::create();

    m_internal_column = column.get();
    m_internal_column->add(std::move(title_row));
    m_internal_column->add(std::move(divider));

    // 内容区域通过 m_content_container 存放用户子节点
    auto content = ContentContainer::create();
    m_content_container = content.get();
    m_internal_column->add(std::move(content));
    // 注意：m_content_container 接收用户通过 Card::add_child 添加的节点
    add_child(std::move(column));
}
```

### Card::set_bounds() 精简

```cpp
auto set_bounds(float x, float y, float w, float h) noexcept -> NanWidget& override {
    runtime::NanWidget::set_bounds(x, y, w, h);
    // 不需要手算了
    if (m_internal_column) {
        m_internal_column->set_bounds(x, y, w, h);
        // 测量+布局由 Column 自身处理
    }
    return *this;
}
```

### 用户 add_child 的重定向

```cpp
// Card 重写 add_child 使其子节点落到内容容器中
auto add_child(std::unique_ptr<NanWidget> child) -> void {
    if (m_content_container) {
        m_content_container->add_child(std::move(child));
    } else {
        NanWidget::add_child(std::move(child));
    }
}
```

---

## Phase 8：Showcase 消除硬编码尺寸

**文件**：`showcase/components/sections/*`

### StatsSection 示例

```cpp
class StatsSection final : public runtime::NanWidget {
    // ... 
    [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
        // 改为：返回 measured_size() 或让父容器强制约束
        // 不再返回 {0.0f, 100.0f}
        if (m_row) {
            LayoutConstraints loose;
            m_row->measure(loose);
            return m_row->measured_size();
        }
        return {0.0f, 0.0f};
    }
};
```

更优方案：StatsSection 改为继承 LayoutContainer 或使用内部 Column：

```cpp
StatsSection() {
    auto row = Row::Create();
    row->gap(16.0f).align_items(LayoutAlignment::stretch);

    // 添加 4 个卡片（同现有逻辑）
    for (size_t i = 0; i < 4; ++i) {
        // ... 创建 card ...
        auto expanded = Expanded::Create();
        expanded->child(std::move(card));
        row->add(std::move(expanded));
    }

    add_child(std::move(row));
}
```

这样 `preferred_size()` 由 Row 自动推导。

---

## 执行顺序

| 阶段 | 依赖 | 预估工作量 |
|------|------|-----------|
| Phase 0: LayoutConstraints 类型 | 无 | 0.5 天 |
| Phase 1: NanWidget measure/layout 协议 | Phase 0 | 1 天 |
| Phase 2: LayoutCore 升级 | Phase 0 | 1 天 |
| Phase 3: LayoutContainer 改用新协议 | Phase 1 + Phase 2 | 1.5 天 |
| Phase 4: FlexWidgets 适配 | Phase 3 | 0.5 天 |
| Phase 5: Pressable 去布局化 | 无 (独立) | 0.5 天 |
| Phase 6: Button 重构 | Phase 5 | 1 天 |
| Phase 7: Card 重构 | Phase 3 | 1 天 |
| Phase 8: Showcase 清理 | Phase 3 + Phase 6 + Phase 7 | 0.5 天 |

**总计核心工作量**：约 7.5 天

**推荐并行路径**：
- 路径 A (Layout 核心)：Phase 0 → 1 → 2 → 3 → 4
- 路径 B (组件清理)：Phase 5 → 6 (可与 A 并行)
- 路径 C (Card + Showcase)：Phase 7 → 8 (依赖 A 完成)

---

## 验证方法

每阶段完成后运行：

```bash
# 编译验证
cmake --build build -- -j$(nproc)

# 布局测试
./build/tests/layout/test_layout_core

# 运行时测试
./build/tests/runtime/test_bounds_hit_test

# 集成测试
./build/tests/showcase/test_showcase_layout

# 启动 showcase 视觉验证
./build/showcase/nandina_showcase
```

视觉需要验证的点：
1. Button 在 idle/hover/pressed/disabled 状态下视觉正常
2. Card 标题、装饰色条、分隔线、内容区域布局正确
3. Dashboard 页面整体布局（header + stats + sidebar + content）不变