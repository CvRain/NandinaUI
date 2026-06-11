## Nandina Point 类功能接口文档

### 1. 设计概述
- **模板类 `BasePoint<T, N>`**：通用 N 维点，存储于 `std::array<T, N>`，提供维度无关的坐标访问与操作。
- **具体类 `NanPoint`**：特化为二维点，使用 `float` 坐标值，适用于窗口像素坐标、UI布局和几何计算，并继承 `BasePoint<float, 2>`。
- **目标**：提供直观、安全、高性能的坐标表示，支持常见几何运算，并与其他数学库互操作。
- **优势**：使用 float 避免 uint8_t 溢出的处理复杂性和精度损失，支持负坐标和连续空间变换，与图形API和动画系统无缝集成。

---

### 2. 核心接口

#### 2.1 构造与赋值
| 函数签名 | 说明 |
|---------|------|
| `NanPoint()` | 默认构造，坐标置零 `(0,0)`。 |
| `NanPoint(T x, T y)` | 通过两个坐标值构造。 |
| `NanPoint(const NanPoint&)` | 拷贝构造（编译器默认生成即可）。 |
| `NanPoint(NanPoint&&)` | 移动构造（默认生成）。 |
| `NanPoint& operator=(const NanPoint&)` | 拷贝赋值（默认生成）。 |
| `NanPoint& operator=(NanPoint&&)` | 移动赋值（默认生成）。 |
| `NanPoint(const BasePoint<T, N>& other)` | 从基类转换构造（当类型匹配时）。 |
| `template<typename U> explicit NanPoint(const BasePoint<U, 2>& other)` | 从其他数值类型的二维点显式转换。 |

#### 2.2 坐标访问
| 函数签名 | 说明 |
|---------|------|
| `constexpr T x() const` | 返回 x 坐标（const 版本）。 |
| `constexpr T y() const` | 返回 y 坐标（const 版本）。 |
| `void set_x(T value)` | 设置 x 坐标。 |
| `void set_y(T value)` | 设置 y 坐标。 |
| `void set(T x, T y)` | 同时设置两个坐标。 |
| `T& operator[](std::size_t index)` | 下标访问（非 const，用于修改）。 |
| `const T& operator[](std::size_t index) const` | 下标访问（const 版本）。 |
| `std::optional<T> get(std::size_t index) const` | 安全获取坐标值（越界返回 `nullopt`）。 |
| `void set(std::size_t index, T value)` | 安全设置坐标值（越界抛出异常）。 |

#### 2.3 比较运算符
| 函数签名 | 说明 |
|---------|------|
| `bool operator==(const NanPoint& rhs) const` | 逐分量相等比较。 |
| `bool operator!=(const NanPoint& rhs) const` | 不等比较。 |
| `bool operator<(const NanPoint& rhs) const` | 字典序比较（便于用作 `std::map` 键）。 |
| `bool operator<=(const NanPoint& rhs) const` | |
| `bool operator>(const NanPoint& rhs) const` | |
| `bool operator>=(const NanPoint& rhs) const` | |

---

### 3. 扩展接口（推荐实现）

#### 3.1 算术运算符
| 函数签名 | 说明 |
|---------|------|
| `NanPoint operator+(const NanPoint& rhs) const` | 向量加法。 |
| `NanPoint operator-(const NanPoint& rhs) const` | 向量减法。 |
| `NanPoint operator*(T scalar) const` | 标量乘法。 |
| `NanPoint operator/(T scalar) const` | 标量除法（除零需处理）。 |
| `NanPoint operator-() const` | 一元负号。 |

#### 3.2 复合赋值运算符
| 函数签名 | 说明 |
|---------|------|
| `NanPoint& operator+=(const NanPoint& rhs)` | |
| `NanPoint& operator-=(const NanPoint& rhs)` | |
| `NanPoint& operator*=(T scalar)` | |
| `NanPoint& operator/=(T scalar)` | |

#### 3.3 几何运算（仅适用于算术类型坐标）
> 对于 `NanPoint` 使用 `float`，所有几何运算直接基于 float 类型，无需额外溢出处理。

| 函数签名 | 说明 |
|---------|------|
| `float dot(const NanPoint& rhs) const` | 点积（内积）。 |
| `float cross(const NanPoint& rhs) const` | 二维叉积（标量值）。 |
| `float length() const` | 向量长度（模）。 |
| `float length_squared() const` | 长度平方（避免开方，用于比较）。 |
| `float distance_to(const NanPoint& other) const` | 两点欧氏距离。 |
| `float distance_squared_to(const NanPoint& other) const` | 距离平方。 |
| `NanPoint normalized() const` | 返回单位向量（若零向量则抛异常）。 |
| `NanPoint& normalize()` | 原地归一化。 |

#### 3.4 类型转换
| 函数签名 | 说明 |
|---------|------|
| `template<typename U> NanPoint cast() const` | 转换为另一种数值类型的点。 |
| `template<typename U> explicit operator NanPoint<U>() const` | 显式转换运算符。 |

#### 3.5 其他工具函数
| 函数签名 | 说明 |
|---------|------|
| `void swap(NanPoint& other) noexcept` | 与另一个点交换内容。 |
| `bool is_zero() const` | 检查是否为零向量。 |
| `std::string to_string() const` | 返回形如 `"(x, y)"` 的字符串。 |

---

### 4. 非成员运算符与工具函数

#### 4.1 标量与点的混合运算
| 函数签名 | 说明 |
|---------|------|
| `NanPoint operator*(T scalar, const NanPoint& pt)` | 标量左乘。 |
| `NanPoint operator/(T scalar, const NanPoint& pt)` | 标量左除（通常无意义，可不实现）。 |

#### 4.2 流输出与格式化
| 函数签名 | 说明 |
|---------|------|
| `std::ostream& operator<<(std::ostream& os, const NanPoint& pt)` | 流输出。 |
| `template<> struct fmt::formatter<NanPoint>` | 支持 `fmt::format`（已包含 `fmt` 头文件）。 |

#### 4.3 哈希支持
| 函数签名 | 说明 |
|---------|------|
| `std::size_t hash_value(const NanPoint& pt) noexcept` | 自定义哈希函数（或特化 `std::hash`）。 |
| `template<> struct std::hash<NanPoint>` | 标准库哈希特化，便于用于无序容器。 |

---

### 5. 接口使用示例（预期行为）
```cpp
NanPoint p1{10.0f, 20.0f};
NanPoint p2{30.0f, 40.0f};

auto p3 = p1 + p2;          // (40, 60) float 运算无溢出
auto dot = p1.dot(p2);      // 10*30 + 20*40 = 1100
auto dist = p1.distance_to(p2);
auto normalized = p1.normalized();

if (p1 < p2) { /* 字典序比较 */ }

std::cout << p1;            // 输出 "(10, 20)"
fmt::print("{}", p1);       // 同样输出
```

---

### 7. 模块导出清单

```cpp
export module nandina.foundation.nan_point;

export namespace nandina {
    // 概念
    template<typename T>
    concept satisfied_point = std::is_integral_v<T> || std::is_floating_point_v<T>;

    // 模板类
    template<satisfied_point T, std::size_t N>
    class BasePoint;

    // 具体类
    class NanPoint;

    // 非成员运算符
    template<typename T, std::size_t N>
    bool operator==(const BasePoint<T, N>& lhs, const BasePoint<T, N>& rhs);

    // ... 其他运算符

    // 流输出
    template<typename T, std::size_t N>
    std::ostream& operator<<(std::ostream& os, const BasePoint<T, N>& pt);
}

// fmt 格式化支持（在命名空间外特化）
template<typename T, std::size_t N>
struct fmt::formatter<nandina::BasePoint<T, N>>;