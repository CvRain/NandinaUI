## Nandina Rect 类功能接口文档

### 1. 设计概述

- **模板类 `BaseRect<T>`**：通用二维矩形，存储于 `std::array<T, 4>`，依次表示 **左 (left)**、**上 (top)**、**右 (right)**、**下 (bottom)**（或 **x**、**y**、**width**、**height**）。UI 库中常用 `(x, y, width, height)` 表示，但边界坐标 `(l, t, r, b)` 在几何运算上更为方便。建议内部存储采用边界表示，同时提供两种构造与访问方式。
- **具体类 `NanRect`**：特化为 `float` 坐标，继承自 `BaseRect<float>`，用于窗口像素矩形。
- **目标**：提供直观的矩形表示、丰富的几何关系判断、以及便捷的布局调整函数。

---

### 2. 内部表示方案选择

| 表示方法 | 存储内容 | 优点 | 缺点 |
| :--- | :--- | :--- | :--- |
| **边界坐标 (LTRB)** | `left, top, right, bottom` | 几何运算（相交、合并、包含）直接高效。 | 修改大小时需要同时调整 `right`/`bottom`。 |
| **位置尺寸 (XYWH)** | `x, y, width, height` | 直观符合 UI 习惯，调整尺寸简单。 | 几何运算需转换，且 `width`/`height` 可能为负。 |

**建议方案**：**内部存储采用 LTRB（边界坐标）**，但对外提供 `x()`, `y()`, `width()`, `height()` 便捷访问，以及 `setX()`, `setY()`, `setWidth()`, `setHeight()` 修改函数。这样既保证了运算效率，又保持了接口的直观性。

---

### 3. 核心接口（必须实现）

#### 3.1 构造与赋值

| 函数签名 | 说明 |
| :--- | :--- |
| `NanRect()` | 默认构造，矩形退化为空 `(0,0,0,0)`。 |
| `NanRect(T left, T top, T right, T bottom)` | 通过边界坐标构造。 |
| `NanRect(const NanPoint& topLeft, const NanPoint& bottomRight)` | 通过两个对角点构造。 |
| `NanRect(const NanPoint& topLeft, const NanSize& size)` | 通过左上角位置与大小构造（需引入 `NanSize` 类）。 |
| `NanRect(const NanRect&)` | 拷贝构造（默认生成）。 |
| `NanRect(NanRect&&)` | 移动构造（默认生成）。 |
| `NanRect& operator=(const NanRect&)` | 拷贝赋值（默认生成）。 |
| `NanRect& operator=(NanRect&&)` | 移动赋值（默认生成）。 |
| `static NanRect fromLTRB(T l, T t, T r, T b)` | 静态工厂，明确使用边界坐标。 |
| `static NanRect fromXYWH(T x, T y, T w, T h)` | 静态工厂，使用位置与尺寸。 |

#### 3.2 属性访问

| 函数签名 | 说明 |
| :--- | :--- |
| `T left() const` | 左边界坐标。 |
| `T top() const` | 上边界坐标。 |
| `T right() const` | 右边界坐标。 |
| `T bottom() const` | 下边界坐标。 |
| `T x() const` | 同 `left()`，UI 惯用。 |
| `T y() const` | 同 `top()`。 |
| `T width() const` | 宽度，保证非负 `right() >= left() ? right() - left() : 0`。 |
| `T height() const` | 高度，保证非负。 |
| `NanPoint topLeft() const` | 返回左上角点。 |
| `NanPoint bottomRight() const` | 返回右下角点。 |
| `NanPoint center() const` | 返回中心点坐标。 |
| `NanSize size() const` | 返回尺寸对象（若引入 `NanSize`）。 |
| `bool isEmpty() const` | 是否为空矩形（`width() <= 0` 或 `height() <= 0`）。 |
| `bool isValid() const` | 是否为规范矩形（`left <= right && top <= bottom`）。 |

#### 3.3 修改器

| 函数签名 | 说明 |
| :--- | :--- |
| `void setLeft(T value)` | 设置左边界（可能需调整右边界以保证有效）。 |
| `void setTop(T value)` | 设置上边界。 |
| `void setRight(T value)` | 设置右边界。 |
| `void setBottom(T value)` | 设置下边界。 |
| `void setX(T value)` | 同 `setLeft()`。 |
| `void setY(T value)` | 同 `setTop()`。 |
| `void setWidth(T value)` | 设置宽度，保持 `left` 不变，调整 `right`。 |
| `void setHeight(T value)` | 设置高度，保持 `top` 不变，调整 `bottom`。 |
| `void setTopLeft(const NanPoint& pt)` | 设置左上角位置。 |
| `void setBottomRight(const NanPoint& pt)` | 设置右下角位置。 |
| `void setRect(T l, T t, T r, T b)` | 一次性设置所有边界。 |
| `void setEmpty()` | 清空矩形。 |

#### 3.4 比较运算符

| 函数签名 | 说明 |
| :--- | :--- |
| `bool operator==(const NanRect& rhs) const` | 所有边界相等。 |
| `bool operator!=(const NanRect& rhs) const` | |

---

### 4. 扩展接口（推荐实现）

#### 4.1 几何运算（变换）

| 函数签名 | 说明 |
| :--- | :--- |
| `NanRect translated(T dx, T dy) const` | 平移矩形，返回新矩形。 |
| `NanRect& translate(T dx, T dy)` | 原地平移。 |
| `NanRect translated(const NanPoint& offset) const` | 使用点对象平移。 |
| `NanRect& translate(const NanPoint& offset)` | |
| `NanRect scaled(T sx, T sy) const` | 以左上角为锚点缩放尺寸。 |
| `NanRect& scale(T sx, T sy)` | |
| `NanRect expanded(T amount) const` | 向四周扩展 `amount`（边界加减）。 |
| `NanRect& expand(T amount)` | |
| `NanRect shrinked(T amount) const` | 向内部收缩（与 `expanded` 相反）。 |
| `NanRect& shrink(T amount)` | |

#### 4.2 几何关系判断

| 函数签名 | 说明 |
| :--- | :--- |
| `bool contains(const NanPoint& pt) const` | 点是否在矩形内（含边界）。 |
| `bool contains(const NanRect& other) const` | 矩形是否完全包含另一矩形。 |
| `bool intersects(const NanRect& other) const` | 两矩形是否相交。 |
| `NanRect intersected(const NanRect& other) const` | 返回相交部分（若无交集则返回空矩形）。 |
| `NanRect united(const NanRect& other) const` | 返回包含两矩形的最小边界矩形。 |
| `bool overlaps(const NanRect& other) const` | 同 `intersects`，别名。 |

#### 4.3 布局辅助（UI 特有）

| 函数签名 | 说明 |
| :--- | :--- |
| `NanRect alignedInside(const NanRect& container, Alignment align) const` | 将当前矩形按指定对齐方式放置在容器内（返回新矩形）。 |
| `NanRect withMargin(T margin) const` | 返回收缩了 `margin` 的矩形（用于内边距）。 |
| `NanRect withPadding(T padding) const` | 同 `withMargin`，语义不同。 |
| `NanRect inset(T dx, T dy) const` | 边缘缩进，四边独立调整。 |
| `NanRect centeredIn(const NanRect& outer) const` | 返回在 `outer` 中居中的矩形（尺寸不变）。 |
| `NanRect boundedTo(const NanRect& boundary) const` | 将矩形裁剪到边界内。 |

#### 4.4 类型转换与工具

| 函数签名 | 说明 |
| :--- | :--- |
| `template<typename U> NanRect<U> cast() const` | 转换为另一种数值类型的矩形。 |
| `std::string toString() const` | 返回形如 `"(l,t,r,b)"` 或 `"(x,y,w,h)"` 的字符串。 |
| `void swap(NanRect& other) noexcept` | 交换内容。 |

---

### 5. 非成员运算符与工具函数

| 函数签名 | 说明 |
| :--- | :--- |
| `NanRect operator+(const NanRect& rect, const NanPoint& offset)` | 平移。 |
| `NanRect operator-(const NanRect& rect, const NanPoint& offset)` | 反向平移。 |
| `NanRect operator&(const NanRect& lhs, const NanRect& rhs)` | 交集运算（重载按位与符号，风格类似 `QRect`）。 |
| `NanRect operator\|(const NanRect& lhs, const NanRect& rhs)` | 并集（最小包围矩形）。 |
| `std::ostream& operator<<(std::ostream& os, const NanRect& rect)` | 流输出。 |
| `template<> struct fmt::formatter<NanRect>` | 支持 `fmt::format`。 |
| `std::size_t hash_value(const NanRect& rect) noexcept` | 哈希支持。 |

---

### 6. 接口使用示例

```cpp
// 构造
auto rect1 = NanRect::fromXYWH(10, 20, 100, 80);
auto rect2 = NanRect(30, 40, 120, 90);

// 访问
int w = rect1.width();          // 100
NanPoint center = rect1.center(); // (60, 60)

// 判断
if (rect1.intersects(rect2)) {
    auto inter = rect1.intersected(rect2);
}

// 变换
auto moved = rect1.translated(5, 10);
auto grown = rect1.expanded(2);

// 布局
auto container = NanRect::fromXYWH(0, 0, 800, 600);
auto centered = rect1.centeredIn(container);
```

---

### 7. 命名与设计建议

1. **内部存储命名**：建议使用 `bounds_` 或 `edges_` 作为 `std::array<T, 4>` 的成员变量名，清晰表示四个边界值。
   ```cpp
   private:
       std::array<T, 4> bounds_{}; // [left, top, right, bottom]
   ```

2. **辅助类 `NanSize`**：强烈建议同时引入 `NanSize` 类，用于表示二维尺寸（宽度、高度）。它可以与 `NanPoint` 和 `NanRect` 形成完整的几何体系：
    - `NanPoint`：位置
    - `NanSize`：尺寸
    - `NanRect`：位置 + 尺寸（或边界）

   `NanSize` 可提供类似 `width()`, `height()`, `area()`, `isEmpty()` 等方法，并能与 `Point` 和 `Rect` 进行运算符重载（如 `NanRect = NanPoint + NanSize`）。

3. **对齐枚举**：定义 `enum class Alignment` 用于 `alignedInside` 等函数。
   ```cpp
   enum class Alignment {
       TopLeft, TopCenter, TopRight,
       CenterLeft, Center, CenterRight,
       BottomLeft, BottomCenter, BottomRight
   };
   ```

4. **模块导出**：
   ```cpp
   export module nandina.foundation.nan_rect;

   export namespace nandina {
       template<satisfied_point T>
       class BaseRect;

       class NanRect;

       // 非成员运算符
       template<typename T>
       bool operator==(const BaseRect<T>& lhs, const BaseRect<T>& rhs);
       // ...
   }
   ```

5. **与 `Point` 的一致性**：保持命名风格统一（如使用 `get`/`set` 前缀或直接属性函数），并考虑未来支持 `constexpr` 以提升编译期计算能力。

---

### 8. 可选扩展：模板化与概念约束

您可以将 `Rect` 也模板化，与 `BasePoint<T, N>` 类似：

```cpp
template<satisfied_point T>
class BaseRect {
public:
    // ...
private:
    std::array<T, 4> bounds_; // left, top, right, bottom
};

using NanRect = BaseRect<float>;
using IntRect = BaseRect<int>;
using DoubleRect = BaseRect<double>;
```