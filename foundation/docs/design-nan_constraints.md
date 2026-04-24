# NanConstraints 设计文档

> 版本：1.0
> 对应 Issue：007
> 标签：`area:foundation`, `area:layout`, `kind:architecture`, `priority:p0`

## 1. 设计目标

为后续布局与 widget 测量机制建立统一约束模型。Constraints（约束）描述了一个 widget 在布局过程中可以接受的最小/最大尺寸范围。

### 核心语义

- **min/max width/height**：布局系统告诉 widget 它"可以"占用多少空间。
- **tight 约束**：`min == max`，widget 必须精确填充该尺寸。
- **loose 约束**：`min == 0, max == 约束值`，widget 可以在 0~max 之间自由选择。
- **unbounded 语义**：某一方向上没有上限（max 为极大值，如 `kInfinity`），布局系统不对该方向做限制。

## 2. 类型定义

### NanConstraints 类

```
namespace nandina::geometry {

class NanConstraints {
public:
    // 特殊值：表示无界限
    static constexpr float kInfinity = std::numeric_limits<float>::infinity();

    // ── 构造函数 ──
    constexpr NanConstraints() noexcept;                                     // 默认：tight(0,0)
    constexpr NanConstraints(float minWidth, float maxWidth,
                             float minHeight, float maxHeight) noexcept;     // 全字段构造
    constexpr explicit NanConstraints(NanSize size) noexcept;                // 从 Size 构造 tight 约束
    constexpr NanConstraints(float width, float height) noexcept;            // tight(width, height)

    // ── 静态工厂方法 ──
    [[nodiscard]] static constexpr auto tight(NanSize size) noexcept -> NanConstraints;
    [[nodiscard]] static constexpr auto tight(float width, float height) noexcept -> NanConstraints;
    [[nodiscard]] static constexpr auto loose(NanSize size) noexcept -> NanConstraints;
    [[nodiscard]] static constexpr auto loose(float width, float height) noexcept -> NanConstraints;
    [[nodiscard]] static constexpr auto expand() noexcept -> NanConstraints; // 全部 unbounded

    // ── 访问器 ──
    [[nodiscard]] constexpr auto minWidth() const noexcept -> float;
    [[nodiscard]] constexpr auto maxWidth() const noexcept -> float;
    [[nodiscard]] constexpr auto minHeight() const noexcept -> float;
    [[nodiscard]] constexpr auto maxHeight() const noexcept -> float;

    // ── 查询 ──
    [[nodiscard]] constexpr auto isTight() const noexcept -> bool;       // min == max 在宽高上
    [[nodiscard]] constexpr auto isTightWidth() const noexcept -> bool;
    [[nodiscard]] constexpr auto isTightHeight() const noexcept -> bool;
    [[nodiscard]] constexpr auto isLoose() const noexcept -> bool;       // 宽高皆 loose
    [[nodiscard]] constexpr auto isBounded() const noexcept -> bool;     // 非 unbounded
    [[nodiscard]] constexpr auto hasUnboundedWidth() const noexcept -> bool;
    [[nodiscard]] constexpr auto hasUnboundedHeight() const noexcept -> bool;
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool;       // min <= max

    // ── 约束变换 ──
    [[nodiscard]] constexpr auto tighten(NanSize size) const noexcept -> NanConstraints;  // 收紧到给定 size
    [[nodiscard]] constexpr auto loosen() const noexcept -> NanConstraints;               // 转换为 loose
    [[nodiscard]] constexpr auto intersect(NanConstraints other) const noexcept -> NanConstraints;

    // ── 尺寸适配 ──
    [[nodiscard]] constexpr auto constrain(NanSize size) const noexcept -> NanSize;       // 将尺寸限制在约束内
    [[nodiscard]] constexpr auto constrainWidth(float width) const noexcept -> float;
    [[nodiscard]] constexpr auto constrainHeight(float height) const noexcept -> float;
    [[nodiscard]] constexpr auto constrain(NanPoint point) const noexcept -> NanPoint;    // 将点限制在约束范围内

    // ── 最佳尺寸 ──
    [[nodiscard]] constexpr auto minSize() const noexcept -> NanSize;     // (minWidth, minHeight)
    [[nodiscard]] constexpr auto maxSize() const noexcept -> NanSize;     // (maxWidth, maxHeight)
    [[nodiscard]] constexpr auto middle() const noexcept -> NanSize;      // 中间值（用于宽松布局）

    // ── 运算符 ──
    [[nodiscard]] constexpr auto operator==(const NanConstraints &rhs) const noexcept -> bool = default;
    [[nodiscard]] constexpr auto operator!=(const NanConstraints &rhs) const noexcept -> bool = default;

    [[nodiscard]] auto toString() const -> std::string;
};
} // namespace nandina::geometry
```

## 3. 语义说明

### 3.1 tight 与 loose

| 类型 | 含义 | 示例 |
|------|------|------|
| tight(100, 200) | 强制宽为100、高为200 | `min==max` |
| loose(NanSize(100, 200)) | 宽 0-100，高 0-200 | `min=0, max=约束值` |

### 3.2 unbounded 语义

- `hasUnboundedWidth()` 返回 `true` 当 `maxWidth` 为 `kInfinity`
- 表示该方向上 widget 可以无限制扩展
- `expand()` 工厂方法创建全部 unbounded 的约束

### 3.3 constrain 语义

- `constrain(NanSize)` 将尺寸夹在 `[min, max]` 范围内
- 用于 widget 在布局时根据约束决定自己的实际尺寸

### 3.4 intersect 语义

- 取两个约束的交集：各方向取更严格的 min 和更宽松的 max
- 如果交集无效（min > max），则返回无效约束

## 4. 使用场景

```
Widget 布局流程：
1. Parent 通过 Constraints 告诉 Child "你可以在这些范围内选择尺寸"
2. Child 根据 Constraints 计算自己的 Size（通过 constrain）
3. Child 返回 Size 给 Parent
4. Parent 根据返回的 Size 确定 Child 的最终位置
```

## 5. 与现有类型的关系

```
NanSize        → 表示具体尺寸
NanConstraints → 表示尺寸的"允许范围"
```

- NanConstraints 是 NanSize 的超集描述
- `constrain(NanSize)` 将 Size 映射到约束定义的范围内
- `tight(NanSize)` 构造一个精确等于该 Size 的约束