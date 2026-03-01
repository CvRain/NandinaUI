# Nandina 设计系统架构文档

> 版本：v0.2-alpha · 最后更新：2026-02-28

---

## 1. 总览

Nandina 是一套面向 Qt Quick 的现代化组件库，其设计系统底座由 **颜色（Color）**、**图元（Primitives）**、**令牌（Tokens）** 和 **主题管理（Theme）** 四层组成。架构参考了前端 [Skeleton UI](https://www.skeleton.dev/) 的设计令牌体系，并将 OKLCH 色彩空间的 CSS 主题文件自动转换为类型安全的 C++ / QML 代码。

### 整体分层

```
┌──────────────────────────────────────────────────────────────┐
│                     QML 组件层 (Controls)                     │
│  NanButton / NanInput / NanSwitch / NanCheckbox / ...        │
├──────────────────────────────────────────────────────────────┤
│                    主题管理 (Theme)                            │
│  ThemeManager (QML_SINGLETON)                                │
│  ├── currentTheme: ThemePreset (枚举)                         │
│  ├── darkMode: bool                                          │
│  ├── colors: ColorSchema*                                    │
│  └── primitives: PrimitiveSchema*                            │
├──────────────────────────────────────────────────────────────┤
│                  预设令牌 (Tokens)                             │
│  theme_presets.hpp/.cpp (自动生成)                             │
│  6 套 Skeleton 主题 × (7 色族 × 24 色值 + 图元 + 排版)        │
├──────────────────────────────────────────────────────────────┤
│                    颜色层 (Color)                              │
│  ColorSchema ──┬── primary:   ColorPalette*                  │
│                ├── secondary: ColorPalette*                   │
│                ├── tertiary:  ColorPalette*                   │
│                ├── success:   ColorPalette*                   │
│                ├── warning:   ColorPalette*                   │
│                ├── error:     ColorPalette*                   │
│                └── surface:   ColorPalette*                   │
│                                                              │
│  ColorPalette ── std::array<QColor, 24>                      │
│  (shade50..950, contrastDark/Light, contrast50..950)         │
├──────────────────────────────────────────────────────────────┤
│                   图元层 (Primitives)                          │
│  PrimitiveSchema                                             │
│  ├── spacing / textScaling / radiusBase / radiusContainer    │
│  ├── borderWidth / divideWidth / ringWidth                   │
│  ├── bodyBackgroundColor / bodyBackgroundColorDark           │
│  ├── baseFont:    TypographySchema*                          │
│  ├── headingFont: TypographySchema*                          │
│  └── anchorFont:  TypographySchema*                          │
├──────────────────────────────────────────────────────────────┤
│                  类型定义 (Core)                               │
│  nandina_types.hpp                                           │
│  ├── ThemePreset   (Catppuccin, Cerberus, ...)               │
│  ├── ColorVariant  (Primary, Secondary, ...)                 │
│  └── ColorAccent   (Shade50..950, Contrast...)               │
└──────────────────────────────────────────────────────────────┘
```

---

## 2. 模块说明

### 2.1 Nandina.Core — `nandina_types.hpp`

定义了整个设计系统的核心枚举类型，通过 `Q_NAMESPACE` + `QML_NAMED_ELEMENT(NandinaType)` 暴露到 QML。

| 枚举 | 值 | 用途 |
|------|------|------|
| `ThemePreset` | `Catppuccin, Cerberus, Concord, Crimson, Fennec, Legacy` | 主题选择（类型安全，替代字符串） |
| `ColorVariant` | `Primary, Secondary, Tertiary, Success, Warning, Error, Surface` | 色族索引 |
| `ColorAccent` | `Shade50..Shade950, ContrastDark, ContrastLight, Contrast50..Contrast950` | 色阶索引（24 个值） |

附带辅助工具函数：
- `themePresetName(ThemePreset) → QString` — 枚举转小写名称
- `themePresetFromName(QString) → ThemePreset` — 名称转枚举（容错回退 Cerberus）
- `allThemePresetNames() → QStringList` — 全部主题名列表
- `AllThemePresets / AllColorVariants / ShadeAccents` — constexpr 数组供遍历

### 2.2 Nandina.Color — 颜色系统

#### ColorPalette

表示单个色族（如 primary）的完整色值集合。

**核心设计决策：**
- 内部使用 `std::array<QColor, 24>` 扁平存储，以 `ColorAccent` 枚举为索引
- 对外同时提供命名属性（QML 绑定）和索引方法（动态/泛型访问）
- 使用 `NAN_COLOR_IMPL` 宏将 24 对 getter/setter 压缩为单行声明，消除样板代码
- `copyFrom()` 方法通过一次数组拷贝实现批量更新

**QML 双重访问模式：**
```qml
// 1. 命名属性（静态绑定，IDE 补全友好）
color: ThemeManager.colors.primary.shade500

// 2. 枚举索引（动态场景，如循环遍历色阶）
color: ThemeManager.colors.color(NandinaType.Primary, NandinaType.Shade500)
```

#### ColorSchema

7 个 `ColorPalette*` 的组合容器，内部同样使用 `std::array` 存储。

| 属性 | 对应 CSS 色族 | 语义 |
|------|-------------|------|
| `primary` | `--color-primary-*` | 品牌主色 |
| `secondary` | `--color-secondary-*` | 辅助强调色 |
| `tertiary` | `--color-tertiary-*` | 第三强调色 |
| `success` | `--color-success-*` | 成功/正向状态 |
| `warning` | `--color-warning-*` | 警告/注意状态 |
| `error` | `--color-error-*` | 错误/危险状态 |
| `surface` | `--color-surface-*` | 背景/中性面 |

关键 `Q_INVOKABLE` 方法：
- `palette(ColorVariant) → ColorPalette*` — 按枚举获取色族
- `color(ColorVariant, ColorAccent) → QColor` — 一步到位获取具体颜色

### 2.3 Nandina.Primitives — 图元系统

#### TypographySchema

单个文字类别（base / heading / anchor）的排版配置：

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `fontFamily` | `QString` | `"system-ui"` | 字体族 |
| `fontWeight` | `int` | `400` | 字重（QFont::Weight 兼容） |
| `italic` | `bool` | `false` | 是否斜体 |
| `letterSpacing` | `qreal` | `0.0` | 字间距 |
| `fontColor` | `QColor` | `black` | 亮色模式字色 |
| `fontColorDark` | `QColor` | `white` | 暗色模式字色 |

#### PrimitiveSchema

全局设计图元，组合了布局令牌与排版子对象：

| 类别 | 属性 | 说明 |
|------|------|------|
| 布局 | `spacing`, `textScaling` | 基础间距（px）、文字缩放比例 |
| 圆角 | `radiusBase`, `radiusContainer` | 元素/容器圆角 |
| 边框 | `borderWidth`, `divideWidth`, `ringWidth` | 默认描边宽度 |
| 背景 | `bodyBackgroundColor`, `bodyBackgroundColorDark` | 页面背景色（亮/暗） |
| 排版 | `baseFont`, `headingFont`, `anchorFont` | 三个 TypographySchema* 子对象 |

### 2.4 Nandina.Tokens — 预设令牌（自动生成）

`theme_presets.hpp` / `theme_presets.cpp` 由 Python 脚本从 Skeleton CSS 主题文件自动生成。

核心函数签名：
```cpp
bool applyPreset(ThemePreset preset, ColorSchema *colors, PrimitiveSchema *primitives);
```

使用 `switch(ThemePreset)` 分发到 6 个 `apply{Name}()` 函数，每个函数通过枚举索引 API 批量设置色值：
```cpp
auto *p = colors->palette(ColorVariant::Primary);
p->setColor(ColorAccent::Shade500, QColor("#0770ef"));
```

### 2.5 Nandina.Theme — 主题管理器

`ThemeManager` 是 QML_SINGLETON，作为整个设计系统的全局入口。

#### 属性

| 属性 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `currentTheme` | `ThemePreset` | RW | 当前主题枚举 |
| `currentThemeName` | `QString` | RO | 当前主题名称（只读） |
| `darkMode` | `bool` | RW | 暗色模式开关 |
| `colors` | `ColorSchema*` | RO | 颜色 schema（CONSTANT 指针） |
| `primitives` | `PrimitiveSchema*` | RO | 图元 schema（CONSTANT 指针） |
| `availableThemes` | `QStringList` | RO | 所有可用主题名列表 |

#### 方法

| 方法 | 说明 |
|------|------|
| `setThemeByName(QString)` | 通过名称字符串切换主题（兼容便捷方法） |
| `themeName(ThemePreset)` | 获取枚举对应的显示名称 |
| `resolveBodyBackground()` | 根据 darkMode 返回对应背景色 |

#### 主题切换机制

```
setCurrentTheme(ThemePreset::Catppuccin)
  │
  ├── applyPreset(ThemePreset, colors, primitives)
  │   └── applyCatppuccin(colors, primitives)
  │       ├── colors->palette(Primary)->setColor(Shade50, ...) × 24 × 7 族
  │       ├── primitives->setSpacing(...) / setRadiusBase(...) / ...
  │       └── primitives->baseFont()->setFontFamily(...)
  │
  ├── emit themeApplied()
  └── emit currentThemeChanged()
        └── QML 绑定自动更新所有 UI 元素
```

**关键设计：** `ColorSchema` 和 `PrimitiveSchema` 的指针是 CONSTANT 的（对象不变），变化的是内部值。子对象的 `changed()` 信号向上冒泡，确保 QML 绑定能感知到任何层级的属性变化。

---

## 3. 色彩空间与转换

### OKLCH → sRGB 管线

Skeleton CSS 主题使用 [OKLCH 色彩空间](https://oklch.com/) 定义颜色：

```css
--color-primary-500: oklch(0.6 0.19 257.51);
```

由于 Qt 的 `QColor` 只支持 sRGB，项目通过 Python 脚本完成转换：

```
OKLCH → OKLab → Linear sRGB → Gamma sRGB → #RRGGBB
```

转换链中的关键步骤：
1. **OKLCH → OKLab**: 极坐标 (L, C, h°) → 直角坐标 (L, a, b)
2. **OKLab → Linear sRGB**: 通过 3×3 矩阵变换
3. **Linear → sRGB**: 应用 gamma 校正曲线
4. **Clamp + 量化**: 钳位到 [0,1] 后转换为 8-bit 整数

### 对比色解析

每个色阶（如 shade500）都有对应的对比色（contrast500），用于确定 **该色阶上的文字颜色**：

```css
--color-primary-contrast-500: var(--color-primary-contrast-light); /* 浅色文字 */
--color-primary-contrast-100: var(--color-primary-contrast-dark);  /* 深色文字 */
```

脚本通过 `var()` 引用解析，将 `contrastDark` 指向 `shade950`，`contrastLight` 指向 `shade50`。

---

## 4. 代码生成流水线

```
temp/global/*.css         scripts/generate_presets.py        Nandina/Core/Tokens/
  (6 个 Skeleton        ─────────────────────────>      theme_presets.hpp
   主题 CSS 文件)                                       theme_presets.cpp
                                                        (~1500 行自动生成代码)
```

### 执行方式

```bash
python3 scripts/generate_presets.py
```

### 生成内容

- **theme_presets.hpp**: 函数声明（`applyPreset`, `apply{Name}`）
- **theme_presets.cpp**: 6 套主题的完整色值 + 图元 + 排版数据

### 添加新主题

1. 将新的 Skeleton CSS 文件放入 `temp/global/`
2. 在 `nandina_types.hpp` 的 `ThemePreset` 枚举中添加新值
3. 在 `generate_presets.py` 的 `PRESET_NAMES` 字典中添加映射
4. 运行代码生成脚本
5. 重新编译

---

## 5. QML 模块结构

```
Nandina (顶层聚合模块)
├── Nandina.Core              nandina_types.hpp (NandinaType 枚举)
├── Nandina.Color             ColorPalette, ColorSchema
├── Nandina.Primitives        TypographySchema, PrimitiveSchema
├── Nandina.Tokens            theme_presets (自动生成)
├── Nandina.Theme             ThemeManager (QML_SINGLETON)
├── Nandina.Controls          (组件层, 开发中)
└── Nandina.Window            (窗口组件, 开发中)
```

### CMake 依赖关系

```
NandinaCore ←── NandinaColor ←── NandinaTokens ←── NandinaTheme
                NandinaPrimitives ←┘                      │
                                                          ↓
                                               NandinaExampleApp
```

每个子模块使用 `qt_add_qml_module()` 注册，通过 `target_link_libraries` 传递依赖。

---

## 6. 设计决策记录

### D1: 为什么用枚举而非字符串选择主题？

- **类型安全**：编译期捕获拼写错误，QML 侧有 IDE 补全
- **性能**：`switch(enum)` 比字符串比较更快
- **可扩展**：添加新主题只需增加枚举值，编译器会提醒未处理的 case
- **兼容**：保留 `setThemeByName(QString)` 便捷方法，向后兼容

### D2: 为什么用 array + 枚举索引取代独立成员变量？

- **消除样板**：24 个颜色从 ~240 行 getter/setter 压缩为 24 行宏 + 1 个 array
- **支持泛型访问**：`setColor(ColorAccent, QColor)` 允许循环批量设置
- **保持 QML 兼容**：宏展开后仍生成独立的 `Q_PROPERTY`，QML 绑定正常工作
- **批量拷贝**：`copyFrom()` 简化为一次数组赋值

### D3: 为什么同时保留命名属性和索引方法？

- **命名属性**（`ThemeManager.colors.primary.shade500`）：IDE 补全、静态分析友好
- **索引方法**（`colors.color(NandinaType.Primary, NandinaType.Shade500)`）：支持动态场景（如遍历所有色族显示调色板）

两种模式底层共享同一个 `std::array`，零冗余。

### D4: 为什么选择 CONSTANT 指针 + 内部信号冒泡？

如果 `colors` 属性每次切换主题都创建新对象，QML 的绑定链会断裂。使用 CONSTANT 指针 + 内部 `changed()` 信号冒泡的方案：
- QML 绑定到子属性（如 `primary.shade500`）时自动跟踪 `changed()` 信号
- 无需重建对象树，切换主题仅修改数值

### D5: 为什么从 legacy-archive 重写而非迁移？

旧版存在以下问题：
- 硬编码 Catppuccin 单一主题系统（4 个 palette 无法扩展）
- ~40 个语义色属性命名不透明（`color0..17`, `mark1..3`）
- QHash 运行时存储 + NanStyle attached property 父链遍历导致性能开销
- 设计令牌作为独立单例（NanSpacing/NanRadius/NanMotion/NanTypography），与主题切换脱钩

新版架构统一了颜色与令牌的生命周期管理，所有设计值通过 ThemeManager 统一分发。

---

## 7. 文件清单

| 文件 | 模块 | 类型 | 说明 |
|------|------|------|------|
| `Nandina/Core/nandina_types.hpp` | Core | 枚举定义 | ThemePreset, ColorVariant, ColorAccent |
| `Nandina/Core/Color/color_palette.hpp` | Color | 类 | 单色族 24 色值容器 |
| `Nandina/Core/Color/color_schema.hpp` | Color | 类 | 7 色族组合容器 |
| `Nandina/Core/Primitives/typography_schema.hpp` | Primitives | 类 | 排版配置 |
| `Nandina/Core/Primitives/primitive_schema.hpp` | Primitives | 类 | 设计图元容器 |
| `Nandina/Core/Tokens/theme_presets.hpp` | Tokens | 自动生成 | 预设函数声明 |
| `Nandina/Core/Tokens/theme_presets.cpp` | Tokens | 自动生成 | 预设数据 (~1500 行) |
| `Nandina/Theme/theme_manager.hpp` | Theme | 类 | 全局主题管理单例 |
| `Nandina/Theme/theme_manager.cpp` | Theme | 实现 | 主题切换逻辑 |
| `scripts/generate_presets.py` | 工具 | Python | CSS → C++ 代码生成器 |
| `temp/global/*.css` | 资源 | CSS | 6 套 Skeleton 主题源文件 |
