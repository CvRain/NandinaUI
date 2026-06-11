## NanInsets — 四边缩进类型设计文档

### 1. 设计概述

- **模块名**: `nandina.foundation.nan_insets`
- **命名空间**: `nandina::geometry`
- **目标**: 提供统一的四边缩进类型，用于表示 padding / margin / border 等 UI 布局内边距概念。
- **定位**: 属于 `foundation` 层，依赖 `nandina.foundation.nan_point` 和 `nandina.foundation.nan_rect` 模块。
- **对应 Issue**: Issue 006（Milestone M1 — Foundation 与 Runtime MVP）

#### 设计原则

1. **与几何体系一致**: 使用 `float` 类型，与 `NanPoint` / `NanSize` / `NanRect` 保持类型体系一致。
2. **四边独立**: 支持 left / top / right / bottom 四边独立设置，同时提供统一设置和静态工厂方法。
3. **与 Rect 交互**: 提供 `apply_to_rect()`（向内应用）和 `inflate_rect()`（向外扩张）两种与 `NanRect` 的交互方法。
4. **运算符重载**: 支持 `+`, `-`, `*`, `/`, `-`（一元负号）等算术运算，以及 `+` / `-` 与 `NanRect` 的混合运算。

### 2. 类设计

#### NanInsets

```
class NanInsets {
    // 构造函数
    NanInsets();                                         // 全零
    explicit NanInsets(float all);                       // 统一值
    NanInsets(float horizontal, float vertical);         // 水平/垂直对称
    NanInsets(float left, float top, float right, float bottom); // 四边独立

    // 静态工厂方法
    static all(float value)
    static symmetric_h(float value)
    static symmetric_v(float value)
    static from_all(float value)
    static from_left(float value)
    static from_top(float value)
    static from_right(float value)
    static from_bottom(float value)

    // 访问器
    float left() const
    float top() const
    float right() const
    float bottom() const
    float horizontal() const    // left + right
    float vertical() const      // top + bottom
    NanPoint top_left() const    // (left, top)
    NanPoint bottom_right() const // (right, bottom)

    // 设置器
    void set_left(float)
    void set_top(float)
    void set_right(float)
    void set_bottom(float)
    void set_all(float)
    void set(float left, float top, float right, float bottom)

    // 属性方法
    bool is_zero() const
    bool is_empty() const
    bool has_positive() const
    bool has_negative() const
    bool is_horizontal_symmetric() const
    bool is_vertical_symmetric() const
    bool is_uniform() const

    // Rect 交互
    NanRect apply_to_rect(const NanRect&) const    // 正值缩小
    NanRect inflate_rect(const NanRect&) const    // 正值扩大
    NanSize to_size() const                       // 水平+垂直总和

    // 算术运算符
    NanInsets operator+(const NanInsets&) const
    NanInsets operator-(const NanInsets&) const
    NanInsets operator*(float) const
    NanInsets operator/(float) const
    NanInsets operator-() const

    // 复合赋值
    NanInsets& operator+=(const NanInsets&)
    NanInsets& operator-=(const NanInsets&)
    NanInsets& operator*=(float)
    NanInsets& operator/=(float)

    // 比较
    bool operator==(const NanInsets&) const
    bool operator!=(const NanInsets&) const

    // 辅助
    string to_string() const
    void swap(NanInsets& other)
};
```

### 3. 使用示例

```cpp
import nandina.foundation.nan_insets;

// 统一缩进
auto padding = nandina::geometry::NanInsets{10.0f};

// 水平/垂直不同
auto margin = nandina::geometry::NanInsets{8.0f, 16.0f};

// 四边独立
auto border = nandina::geometry::NanInsets{1.0f, 2.0f, 1.0f, 2.0f};

// 应用到矩形
auto rect = nandina::geometry::NanRect{0.0f, 0.0f, 100.0f, 100.0f};
auto inner = padding.apply_to_rect(rect);  // (10, 10, 90, 90)
auto outer = margin.inflate_rect(rect);   // (-8, -16, 108, 116)

// 运算符
auto total = padding + margin;           // (18, 26, 18, 26)
auto shrunk = rect + padding;            // 同 apply_to_rect(rect)
auto expanded = rect - padding;          // 同 inflate_rect(rect)

// 静态工厂
auto only_left = nandina::geometry::NanInsets::from_left(5.0f);
```

### 4. 与 Issue 006 的对应关系

| 要求 | 实现 |
|------|------|
| Insets 类型 | ✅ NanInsets |
| 四边独立缩进 | ✅ left / top / right / bottom |
| contains / intersect / inflate / deflate | ✅ apply_to_rect / inflate_rect + intersects 通过 NanRect 代理 |
| 基本的辅助方法 | ✅ is_zero / is_empty / is_uniform / is_horizontal_symmetric 等 |
| 与 Rect 类型协作 | ✅ operator+(rect, insets) / operator-(rect, insets) |
| 算术运算 | ✅ + - * / 以及复合赋值 |

### 5. 边界情况

- **零缩进**: `is_zero()` 返回 true，`apply_to_rect` 返回原矩形。
- **负缩进**: 表示向外扩张，`has_negative()` 返回 true。
- **除零**: `operator/` 在除数为零时抛出 `std::domain_error`。
- **单边缩进**: 通过 `from_left()`/`from_top()` 等静态工厂创建。