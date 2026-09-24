/**
 * theme/design_system — 不可变设计系统快照：共享视觉片段、组件配方与外观感知解析。
 *
 * 分层模型：
 *
 *   primitive tokens / semantic palette（light + dark）
 *                     │
 *                     ▼
 *   共享视觉片段      BoxStyle / TypeStyle / FocusRingStyle /
 *                    TrackStyle / ThumbStyle / ControlMetrics
 *                     │
 *                     ▼
 *   组件配方          ButtonRecipe / CheckboxRecipe / SliderRecipe /
 *                    TextFieldRecipe（+ selector 规则覆盖）
 *                     │
 *                     ▼
 *   解析结果          ResolvedButtonStyle / …（具体值）
 *                     │
 *                     ▼
 *   painter          BoxPainter / FocusRingPainter（非节点）
 *
 * 迁移状态：全部控件已切换到本模型；遗留 per-component 平铺解析器（button_style 等）
 * 已退休，配方书是唯一事实来源。遗留 NanStyle 规则作为选择器层仍可合并覆盖。
 * 详见 dev-docs-v3/phase6-theme-design-system.md。
 */

#ifndef NANDINA_EXPERIMENT_THEME_DESIGN_SYSTEM_HPP
#define NANDINA_EXPERIMENT_THEME_DESIGN_SYSTEM_HPP

#include "appearance.hpp"
#include "nan_style.hpp"
#include "theme.hpp"
#include "visual_state.hpp"

#include <optional>
#include <vector>

namespace nandina::theme
{
    // ─── 共享视觉片段（声明形态：token-or-literal） ───────────────────────────

    /**
     * 矩形视觉槽位的填充 / 描边 / 圆角（按钮主体、勾选框指示器、输入框容器等）。
     */
    using BoxStyle = struct BoxStyle {
        ThemeColor fill;
        ThemeColor border;
        ThemeScalar border_width;
        ThemeScalar radius;
    };

    /**
     * 文本槽位的排版。命名 `TypeStyle` 是因为 `primitives::TextStyle` 已被
     * widget 文本 primitive 占用。
     */
    using TypeStyle = struct TypeStyle {
        ThemeColor color;
        ThemeScalar font_size;
    };

    /** 控件聚焦时绘制的焦点环。 */
    using FocusRingStyle = struct FocusRingStyle {
        ThemeColor color;
        ThemeScalar width;
    };

    /** 线性轨道（slider / progress）。`box` 承载填充 / 描边 / 圆角。 */
    using TrackStyle = struct TrackStyle {
        BoxStyle box;
        ThemeScalar thickness;
    };

    /** 轨道上可拖拽的拇指。`box` 的 `radius` 仍是**圆角半径**语义；Slider 需要的拇指
     * 像素半径放在 `SliderRecipe::thumb_radius`，两者不要混用。 */
    using ThumbStyle = struct ThumbStyle {
        BoxStyle box;
    };

    /** 状态层（hover / pressed 半透明叠加色），独立绘制在基础容器之上。 */
    using StateLayerStyle = struct StateLayerStyle {
        ThemeColor hover;
        ThemeColor pressed;
    };

    /** Pointer impact feedback, clipped to the component container. */
    using RippleStyle = struct RippleStyle {
        ThemeColor color;
        ThemeScalar duration;
    };

    /** 软阴影（elevation）：颜色 + 偏移 + 软边衰减宽度，供 Card 等容器叠加。 */
    using ShadowStyle = struct ShadowStyle {
        ThemeColor color;   // 阴影颜色（带 alpha 控强度）
        ThemeScalar offset_x;
        ThemeScalar offset_y;
        ThemeScalar spread; // 软边衰减宽度（>0）
    };

    /**
     * 所有控件共享的尺寸 / 间距。组件只取用自己相关的槽位，其余忽略。
     */
    using ControlMetrics = struct ControlMetrics {
        ThemeScalar height;
        ThemeScalar padding_x;
        ThemeScalar gap;
        ThemeScalar min_height;
        ThemeScalar box_size;
        ThemeScalar preferred_width;
    };

    /** Switch 度量：轨道 / 拇指尺寸（组件专属片段）。 */
    using SwitchMetrics = struct SwitchMetrics {
        ThemeScalar track_width;   // 轨道宽度（含拇指行程）
        ThemeScalar track_height;  // 轨道高度（pill 直径）
        ThemeScalar thumb_size;    // 拇指直径
        ThemeScalar gap;           // 轨道与标签间距
        ThemeScalar min_height;    // 整控件最小高度
    };

    // ─── 解析后的片段（具体值） ───────────────────────────────────────────────

    /** 解析后的矩形槽位：所有字段都是具体颜色 / 数值。 */
    using ResolvedBoxStyle = struct ResolvedBoxStyle {
        NanColor fill;
        NanColor border;
        float border_width = 0.0F;
        float radius = 0.0F;
    };

    using ResolvedTypeStyle = struct ResolvedTypeStyle {
        NanColor color;
        float font_size = 0.0F;
    };

    using ResolvedFocusRing = struct ResolvedFocusRing {
        NanColor color;
        float width = 0.0F;
    };

    using ResolvedTrackStyle = struct ResolvedTrackStyle {
        ResolvedBoxStyle box;
        float thickness = 0.0F;
    };

    using ResolvedThumbStyle = struct ResolvedThumbStyle {
        ResolvedBoxStyle box;
        /** 拇指圆的**像素半径**（不是圆角半径）。绘制时按屏幕缩放后再夹紧到控件内。 */
        float radius = 0.0F;
    };

    using ResolvedControlMetrics = struct ResolvedControlMetrics {
        float height = 0.0F;
        float padding_x = 0.0F;
        float gap = 0.0F;
        float min_height = 0.0F;
        float box_size = 0.0F;
        float preferred_width = 0.0F;
    };

    using ResolvedShadowStyle = struct ResolvedShadowStyle {
        NanColor color;
        float offset_x = 0.0F;
        float offset_y = 0.0F;
        float spread = 0.0F;
    };

    /** 解析后的 Switch 度量。 */
    using ResolvedSwitchMetrics = struct ResolvedSwitchMetrics {
        float track_width = 0.0F;
        float track_height = 0.0F;
        float thumb_size = 0.0F;
        float gap = 0.0F;
        float min_height = 0.0F;
    };

    // ─── 组件配方（片段组合） ────────────────────────────────────────────────

    /** Button 配方：容器 + 文本 + 焦点环 + 状态层 + 度量。 */
    using ButtonRecipe = struct ButtonRecipe {
        BoxStyle container;
        TypeStyle label;
        FocusRingStyle focus;
        StateLayerStyle state_layer;
        RippleStyle ripple;
        ControlMetrics metrics;
    };

    /** Checkbox 配方：指示器（勾选框）+ 勾选标记 + 文本 + 焦点环 + 度量。 */
    using CheckboxRecipe = struct CheckboxRecipe {
        BoxStyle indicator;
        ThemeColor check; // 勾选标记（对勾）颜色
        TypeStyle label;
        FocusRingStyle focus;
        ControlMetrics metrics;
    };

    /** Slider 配方：活动 / 非活动轨道 + 拇指 + 焦点环 + 度量。 */
    using SliderRecipe = struct SliderRecipe {
        TrackStyle inactive_track;
        TrackStyle active_track;
        ThumbStyle thumb;
        /** 拇指圆的像素半径。刻意独立于 `thumb.box.radius`（后者是圆角半径语义），
         * 避免主题作者把 `radius_full` 之类的圆角 token 当像素半径填进来。 */
        ThemeScalar thumb_radius;
        FocusRingStyle focus;
        ControlMetrics metrics;
    };

    /** TextField 配方：容器 + 值 / 占位 / 选区文本 + 焦点环 + 度量。 */
    using TextFieldRecipe = struct TextFieldRecipe {
        BoxStyle container;
        TypeStyle value;
        TypeStyle placeholder;
        ThemeColor selection;
        FocusRingStyle focus;
        ControlMetrics metrics;
    };

    /**
     * TextArea 度量：可见行数 / 行高 / 内边距。
     *
     * 不复用共享的 `ControlMetrics`：后者没有 `padding_y`，而多行输入框的垂直内边距
     * 与默认高度都由行数 × 行高决定（同 AlertMetrics 的取舍）。
     */
    using TextAreaMetrics = struct TextAreaMetrics {
        ThemeScalar rows;        // 默认可见行数（set_rows 的实例覆盖优先）
        ThemeScalar line_height; // 默认高度用的每行逻辑高度
        ThemeScalar padding_x;   // 水平内边距
        ThemeScalar padding_y;   // 垂直内边距
    };

    /** TextArea 配方：容器 + 值 / 占位 / 选区文本 + 焦点环 + 多行度量。 */
    using TextAreaRecipe = struct TextAreaRecipe {
        BoxStyle container;
        TypeStyle value;
        TypeStyle placeholder;
        ThemeColor selection;
        FocusRingStyle focus;
        TextAreaMetrics metrics;
    };

    /** Switch 配方：轨道 + 拇指 + 文本 + 焦点环 + 度量。 */
    using SwitchRecipe = struct SwitchRecipe {
        BoxStyle track;
        ThumbStyle thumb;
        TypeStyle label;
        FocusRingStyle focus;
        SwitchMetrics metrics;
    };

    /** Badge 配方：pill 容器 + 文本 + 度量（纯展示，无状态无交互）。 */
    using BadgeRecipe = struct BadgeRecipe {
        BoxStyle container;
        TypeStyle label;
        ControlMetrics metrics;
    };

    /** Card 度量：水平/垂直内边距与最小高度（内容驱动尺寸）。 */
    using CardMetrics = struct CardMetrics {
        ThemeScalar padding_x;
        ThemeScalar padding_y;
        ThemeScalar min_height;
    };

    /** Card 配方：surface 容器 + 软阴影 + 度量（单子内容容器，无状态）。 */
    using CardRecipe = struct CardRecipe {
        BoxStyle container;
        ShadowStyle shadow;
        CardMetrics metrics;
    };

    /** ProgressBar 配方：轨道 + 填充 + 度量（确定性进度条，无交互）。 */
    using ProgressBarRecipe = struct ProgressBarRecipe {
        BoxStyle track;
        BoxStyle fill;
        ControlMetrics metrics;
    };

    /**
     * Spinner 度量：指示环直径 / 环厚 / 弧长 / 旋转速度。
     *
     * 组件专属片段（同 SwitchMetrics / CardMetrics）：`ControlMetrics` 没有弧长与
     * 角速度字段，硬塞进去会让所有组件都被动携带这两个无关字段。
     */
    using SpinnerMetrics = struct SpinnerMetrics {
        ThemeScalar diameter;       // 指示环外径
        ThemeScalar thickness;      // 环厚
        ThemeScalar arc_radians;    // 弧长（弧度，2π = 整环）
        ThemeScalar rotation_speed; // 角速度（弧度/秒）
    };

    /** Spinner 配方：指示环颜色 + 组件专属度量（不定量进度指示，无交互）。 */
    using SpinnerRecipe = struct SpinnerRecipe {
        ThemeColor indicator;
        SpinnerMetrics metrics;
    };

    /**
     * Skeleton 度量：占位条高度 / 行间距 / 末行宽度比例 / 首选宽度。
     *
     * 组件专属片段（同 CardMetrics / SwitchMetrics）：`ControlMetrics` 没有行间距与
     * 末行比例字段，硬塞进去会让所有组件都被动携带这两个无关字段。
     */
    using SkeletonMetrics = struct SkeletonMetrics {
        ThemeScalar height;            // 单行占位条高度
        ThemeScalar line_gap;          // 相邻占位条间距
        ThemeScalar last_line_ratio;   // 末行宽度占比 [0,1]
        ThemeScalar preferred_width;   // 无界约束下的首选宽度
    };

    /** Skeleton 配方：加载占位块（surface）+ 度量（纯展示，无交互）。 */
    using SkeletonRecipe = struct SkeletonRecipe {
        BoxStyle surface;
        SkeletonMetrics metrics;
    };

    /** EmptyState 度量：内容间距 / 内边距 / 最小高度 / 首选宽度。 */
    using EmptyStateMetrics = struct EmptyStateMetrics {
        ThemeScalar gap;             // 相邻内容块间距
        ThemeScalar padding_x;       // 水平内边距
        ThemeScalar padding_y;       // 垂直内边距
        ThemeScalar min_height;      // 整控件最小高度
        ThemeScalar preferred_width; // 无界约束下的首选宽度
    };

    /** EmptyState 配方：容器 + 标题 / 描述排版 + 度量（空状态展示，无交互）。 */
    using EmptyStateRecipe = struct EmptyStateRecipe {
        BoxStyle container;
        TypeStyle title;
        TypeStyle description;
        EmptyStateMetrics metrics;
    };

    /**
     * Alert 度量：列间距 / 内边距 / 最小高度 / dismiss 边长 / 首选宽度。
     *
     * 不复用共享的 `ControlMetrics`：后者没有 `padding_y`，而 Alert 的上下内边距是
     * 真实存在的造型字段；把 `height` 当 padding 用属于语义错配（本项目已因单位 /
     * 语义错配出过一次事故，见 `SliderRecipe::thumb_radius` 的注释）。这与
     * `EmptyStateMetrics` 的选择一致。
     */
    using AlertMetrics = struct AlertMetrics {
        ThemeScalar gap;             // 相邻列间距（icon / 文本列 / action / dismiss）
        ThemeScalar padding_x;       // 水平内边距
        ThemeScalar padding_y;       // 垂直内边距
        ThemeScalar min_height;      // 整控件最小高度
        ThemeScalar box_size;        // dismiss 小按钮的方形边长
        ThemeScalar preferred_width; // 无界约束下的首选宽度
    };

    /**
     * Alert 配方：容器 + 标题 / 描述排版 + 图标色 + 度量（内联消息条，无交互）。
     *
     * tone 不是配方字段而是解析入参：每档 tone 的填充 / 边框 / 图标色由默认设计系统里
     * 的 tone 规则给出（见 `default_alert_recipe()` 与 `DesignSystem.components.alert.rules`）。
     */
    using AlertRecipe = struct AlertRecipe {
        BoxStyle container;
        TypeStyle title;
        TypeStyle description;
        ThemeColor icon;
        AlertMetrics metrics;
    };

    /** RadioButton 配方：圆形指示器 + 选中点 + 文本 + 焦点环 + 度量。 */
    using RadioButtonRecipe = struct RadioButtonRecipe {
        BoxStyle indicator;
        ThemeColor dot; // 选中内点颜色
        TypeStyle label;
        FocusRingStyle focus;
        ControlMetrics metrics;
    };

    /**
     * Toggle 配方：容器 + 文本 + 焦点环 + 状态层 + 度量。
     *
     * 两态按钮（外观是按钮、语义是 checkbox）：`checked` 由解析入参决定容器填充
     * （未选中走 treatment 规则，选中统一换成 tone 强调色），hover / pressed 走独立的
     * 状态层片段（与 Button 同款），因此不需要 treatment × checked × state 的组合规则。
     */
    using ToggleRecipe = struct ToggleRecipe {
        BoxStyle container;
        TypeStyle label;
        FocusRingStyle focus;
        StateLayerStyle state_layer;
        ControlMetrics metrics; // height、padding_x、min_height
    };

    /**
     * ToggleGroup 配方：容器 + 度量（gap = 成员间距、padding_x = 组内边距）。
     *
     * 组是协调对象而不是场景节点（同 RadioGroup），容器造型由宿主读取
     * `resolved_style()` 后自行绘制，因此默认容器完全透明。
     */
    using ToggleGroupRecipe = struct ToggleGroupRecipe {
        BoxStyle container;
        ControlMetrics metrics; // gap、padding_x、min_height
    };

    /**
     * ButtonGroup 配方：容器 + 度量（gap = 成员间距、padding_x = 组内边距）。
     *
     * 组是真实场景节点（子按钮经 `add_button()` 挂载），容器默认完全透明：成员之间的
     * 节奏由 gap 表达。刻意没有 "attached"（共边）字段——当前配方模型无法表达按子项
     * 位置覆盖圆角 / 抑制相邻边框，理由见 `widget/button_group.hpp` 的类注释。
     */
    using ButtonGroupRecipe = struct ButtonGroupRecipe {
        BoxStyle container;
        ControlMetrics metrics; // gap、padding_x、min_height
    };

    /** Breadcrumb 配方：容器 + 链接 / 当前页排版 + 分隔符 + 链接焦点环 + 度量。 */
    using BreadcrumbRecipe = struct BreadcrumbRecipe {
        BoxStyle container;
        TypeStyle link;            // 可点击条目
        ThemeColor link_hover;     // hover / pressed 文字色
        TypeStyle current;         // 当前页 / 无回调条目
        ThemeColor separator;      // 分隔符字形颜色
        FocusRingStyle link_focus; // 链接条目焦点环
        ControlMetrics metrics;    // gap（条目与分隔符间距）、padding_x、min_height
    };

    /** Pagination 配方：容器 + 槽位面（普通 / hover 叠加 / 当前页）+ 文本 + 焦点环 + 度量。 */
    using PaginationRecipe = struct PaginationRecipe {
        BoxStyle container;     // 整条容器（透明默认）
        BoxStyle item;          // 普通页码槽位面（透明默认）
        ThemeColor item_hover;  // hover 叠加色（按 alpha 覆盖在槽位面上）
        BoxStyle item_active;   // 当前页槽位面
        TypeStyle label;        // 普通页码 / 省略号 / 上一页·下一页文本
        TypeStyle label_active; // 当前页文本
        ThemeColor ellipsis;    // 省略号颜色
        FocusRingStyle focus;   // 聚焦槽位的焦点环
        ControlMetrics metrics; // box_size（槽位边长）、gap、padding_x、min_height
    };

    /** Tabs 配方：容器（背景/边框）+ 选中 pill + 下划线 + 标签 + 焦点环 + 度量。 */
    using TabsRecipe = struct TabsRecipe {
        BoxStyle container;           // 列表容器背景/边框/圆角（透明默认 = 无背景边框）
        BoxStyle selected_background; // 选中标签 pill 背景（透明默认 = 无 pill）
        TypeStyle label;              // 未选中标签
        TypeStyle label_selected;     // 选中标签
        ThemeColor indicator;         // 下划线颜色（透明默认 = 无下划线）
        ThemeScalar indicator_thickness;
        FocusRingStyle focus;
        ControlMetrics metrics;       // gap（标签间距）、padding_x（容器内边距）、min_height
    };

    /** Tooltip 配方：气泡容器 + 文本 + 度量（纯展示浮层，无交互状态）。 */
    using TooltipRecipe = struct TooltipRecipe {
        BoxStyle container;
        TypeStyle label;
        ControlMetrics metrics; // padding_x（气泡内边距）、gap（气泡与目标间距）、min_height
    };

    /** Select 配方：触发字段 + 弹出列表 + 值/选项文本 + 焦点环 + 度量。 */
    using SelectRecipe = struct SelectRecipe {
        BoxStyle container;         // 触发字段（关闭态）
        BoxStyle popup;             // 弹出列表容器
        TypeStyle value;            // 当前选中值文本
        TypeStyle option;           // 选项文本（未选中）
        TypeStyle option_selected;  // 选中选项文本
        FocusRingStyle focus;
        ControlMetrics metrics;     // height（字段高）、padding_x、gap（字段与弹窗间距）、min_height（选项行高）
    };

    /** Divider 配方：分隔线颜色/厚度/首选长度（纯展示）。 */
    using DividerRecipe = struct DividerRecipe {
        ThemeColor color;
        ThemeScalar thickness;
        ThemeScalar preferred_length;
    };

    /** Avatar 配方：圆形容器 + 首字母文本 + 度量（纯展示）。 */
    using AvatarRecipe = struct AvatarRecipe {
        BoxStyle container;
        TypeStyle label;
        ControlMetrics metrics; // box_size = 直径
    };

    /** Chip 配方：pill 容器 + 文本 + 移除标记 + 度量。 */
    using ChipRecipe = struct ChipRecipe {
        BoxStyle container;
        TypeStyle label;
        ThemeColor remove_color;
        FocusRingStyle focus;
        ControlMetrics metrics; // height、padding_x、gap（文本与移除标记间距）
    };

    /** Dialog 度量：面板尺寸 / 内边距 / 标题与内容间距 / 最小高度。 */
    using DialogMetrics = struct DialogMetrics {
        ThemeScalar panel_width;
        ThemeScalar padding_x;
        ThemeScalar padding_y;
        ThemeScalar gap;
        ThemeScalar min_height;
    };

    /** Popover 度量：面板内边距 / 面板与锚点间距 / 最小高度。 */
    using PopoverMetrics = struct PopoverMetrics {
        ThemeScalar padding_x;
        ThemeScalar padding_y;
        ThemeScalar gap;
        ThemeScalar min_height;
    };

    /** Popover 配方：锚定浮层面板 + 度量（非模态容器，无交互状态）。 */
    using PopoverRecipe = struct PopoverRecipe {
        BoxStyle panel; // 浮层面板容器
        PopoverMetrics metrics;
    };

    /**
     * DropdownMenu 度量：条目行高 / 面板内边距 / 分隔线上下间距 / 条目圆角 /
     * 分隔线厚度 / 最小宽度。
     *
     * 外层面板（填充 / 边框 / 圆角 / 面板内边距）由 `PopoverRecipe` 负责：DropdownMenu
     * 组合一个 Popover，配方只覆盖面板**内部**的条目列表。
     */
    using DropdownMenuMetrics = struct DropdownMenuMetrics {
        ThemeScalar item_height;         // 单个条目行高
        ThemeScalar padding_x;           // 条目列表的水平内边距
        ThemeScalar padding_y;           // 条目列表的垂直内边距
        ThemeScalar gap;                 // 分隔线上下留白
        ThemeScalar item_radius;         // 高亮条目填充圆角
        ThemeScalar separator_thickness; // 分隔线厚度
        ThemeScalar min_width;           // 面板内容最小宽度（含 padding_x）
    };

    /**
     * DropdownMenu 配方：条目列表的排版 / 状态色 + 度量（浮层面板由 Popover 负责）。
     *
     * 没有组件级交互状态选择器（同 Popover / Tooltip）：hover / focused 是**逐条目**
     * 的状态，由视图读取 `hover_fill` / `focus_fill` 直接绘制，不参与配方规则选择。
     */
    using DropdownMenuRecipe = struct DropdownMenuRecipe {
        TypeStyle item_label;      // 常规条目文本
        TypeStyle item_shortcut;   // 快捷键提示文本（仅展示）
        TypeStyle group_label;     // 分组标题（kind == label）
        ThemeColor disabled_label; // 禁用条目文本色
        ThemeColor hover_fill;     // 指针悬停条目填充
        ThemeColor focus_fill;     // 键盘高亮条目填充
        ThemeColor checked_indicator; // 勾选指示（对勾 / 圆点）颜色
        ThemeColor separator;         // 分隔线颜色
        DropdownMenuMetrics metrics;
    };

    /**
     * Combobox 度量：输入框高度 / 内边距 / 首选宽度、浮层与输入框间距、
     * 选项行高 / 列表内边距 / 行圆角 / 列表最小宽度。
     *
     * 外层面板（填充 / 边框 / 圆角 / 面板内边距）由 `PopoverRecipe` 负责：Combobox
     * 组合一个 Popover，配方只覆盖输入框外壳与面板**内部**的选项列表。
     */
    using ComboboxMetrics = struct ComboboxMetrics {
        ThemeScalar height;          // 输入框高度
        ThemeScalar padding_x;       // 输入文本水平内边距
        ThemeScalar preferred_width; // 无内容时的首选宽度
        ThemeScalar gap;             // 输入框与浮层的间距
        ThemeScalar item_height;     // 单个选项行高
        ThemeScalar list_padding_x;  // 选项列表的水平内边距
        ThemeScalar list_padding_y;  // 选项列表的垂直内边距
        ThemeScalar item_radius;     // 高亮选项填充圆角
        ThemeScalar min_width;       // 列表内容最小宽度（含 list_padding_x）
    };

    /**
     * Combobox 配方：输入框外壳 + 输入 / 占位文本 + 焦点环 + 选项列表排版与状态色。
     *
     * 没有浮层面板字段（面板由 Popover 携带），也没有组件级状态选择器之外的造型：
     * hover / focused 是逐条目的状态，视图直接读取 `hover_fill` / `focus_fill`。
     */
    using ComboboxRecipe = struct ComboboxRecipe {
        BoxStyle input;            // 输入框外壳（填充 / 边框 / 圆角）
        TypeStyle value;           // 输入文本（也是选中标签的显示）
        TypeStyle placeholder;     // 占位文本
        ThemeColor selection;      // 输入选区的填充色
        FocusRingStyle focus;      // 输入框焦点环
        TypeStyle option;          // 未高亮选项文本
        ThemeColor disabled_label; // 禁用选项文本色
        ThemeColor hover_fill;     // 指针悬停选项填充
        ThemeColor focus_fill;     // 键盘高亮选项填充
        ComboboxMetrics metrics;
    };

    /** Dialog 配方：半透明遮罩 + 居中面板 + 标题文本 + 度量。 */
    using DialogRecipe = struct DialogRecipe {
        ThemeColor scrim;    // 遮罩（半透明，覆盖全屏）
        BoxStyle panel;      // 居中面板容器
        TypeStyle title;     // 标题文本
        DialogMetrics metrics;
    };

    // ─── 配方规则覆盖（selector 增量） ────────────────────────────────────────
    //
    // 配方书 = `base`（完全指定）+ 有序规则列表。
    // 解析：从 `base` 出发，按顺序应用所有匹配规则（后匹配者胜），再把
    // ThemeValue 按当前外观解析为具体值。
    // 跨切面状态变换（disabled 透明度）由解析器在规则循环之后应用。Button 状态层
    // 保留为独立解析片段，由 widget 根据当前交互状态绘制，不再改写基础容器。

    /** Button 状态规则：按 tone / treatment / size / state 选择，覆盖容器 / 文本 / 焦点 / 度量字段。 */
    using ButtonRecipeRule = struct ButtonRecipeRule {
        ButtonRuleSelector selector; // tone / treatment / size / state
        std::optional<ThemeColor> container_fill;
        std::optional<ThemeColor> container_border;
        std::optional<ThemeScalar> container_border_width;
        std::optional<ThemeScalar> container_radius;
        std::optional<ThemeColor> label_color;
        std::optional<ThemeScalar> label_font_size;
        std::optional<ThemeColor> focus_ring_color;
        std::optional<ThemeScalar> focus_ring_width;
        std::optional<ThemeScalar> metrics_height;
        std::optional<ThemeScalar> metrics_padding_x;
        std::optional<ThemeColor> state_layer_hover;
        std::optional<ThemeColor> state_layer_pressed;
        std::optional<ThemeColor> ripple_color;
        std::optional<ThemeScalar> ripple_duration;
    };

    /** Checkbox 规则：支持 checked 布尔选择器（未勾选 outline / 勾选 filled）。 */
    using CheckboxRecipeRule = struct CheckboxRecipeRule {
        std::optional<bool> checked; // nullopt = 任意
        std::optional<CheckboxVisualState> state;
        std::optional<ThemeColor> indicator_fill;
        std::optional<ThemeColor> indicator_border;
        std::optional<ThemeScalar> indicator_border_width;
        std::optional<ThemeScalar> indicator_radius;
        std::optional<ThemeColor> label_color;
        std::optional<ThemeScalar> label_font_size;
        std::optional<ThemeColor> focus_ring_color;
        std::optional<ThemeScalar> focus_ring_width;
        std::optional<ThemeScalar> metrics_gap;
        std::optional<ThemeScalar> metrics_box_size;
    };

    /** Slider 规则：按状态覆盖轨道 / 拇指 / 焦点环字段。 */
    using SliderRecipeRule = struct SliderRecipeRule {
        std::optional<SliderVisualState> state;
        std::optional<ThemeColor> track_inactive_fill;
        std::optional<ThemeColor> track_active_fill;
        std::optional<ThemeScalar> track_thickness;
        std::optional<ThemeColor> thumb_fill;
        /** 拇指圆的像素半径（不是圆角半径）；见 `SliderRecipe::thumb_radius`。 */
        std::optional<ThemeScalar> thumb_radius;
        std::optional<ThemeColor> thumb_border;
        std::optional<ThemeScalar> thumb_border_width;
        std::optional<ThemeColor> focus_ring_color;
        std::optional<ThemeScalar> focus_ring_width;
    };

    /** TextField 规则：按位掩码状态覆盖容器 / 文本 / 选区 / 焦点环字段。 */
    using TextFieldRecipeRule = struct TextFieldRecipeRule {
        std::optional<TextFieldVisualState> state;
        std::optional<ThemeColor> container_fill;
        std::optional<ThemeColor> container_border;
        std::optional<ThemeScalar> container_border_width;
        std::optional<ThemeScalar> container_radius;
        std::optional<ThemeColor> value_color;
        std::optional<ThemeColor> placeholder_color;
        std::optional<ThemeColor> selection_color;
        std::optional<ThemeColor> focus_ring_color;
        std::optional<ThemeScalar> focus_ring_width;
        std::optional<ThemeScalar> font_size;
        std::optional<ThemeScalar> metrics_height;
        std::optional<ThemeScalar> metrics_padding_x;
    };

    /**
     * TextArea 规则：状态位掩码 + read_only 选择器，覆盖容器 / 文本 / 选区 / 焦点环 / 度量。
     *
     * `read_only` 与 `state` 正交：`nullopt` 表示任意读写状态，`true` 只命中只读实例。
     */
    using TextAreaRecipeRule = struct TextAreaRecipeRule {
        std::optional<TextAreaVisualState> state;
        std::optional<bool> read_only;
        std::optional<ThemeColor> container_fill;
        std::optional<ThemeColor> container_border;
        std::optional<ThemeScalar> container_border_width;
        std::optional<ThemeScalar> container_radius;
        std::optional<ThemeColor> value_color;
        std::optional<ThemeColor> placeholder_color;
        std::optional<ThemeColor> selection_color;
        std::optional<ThemeColor> focus_ring_color;
        std::optional<ThemeScalar> focus_ring_width;
        std::optional<ThemeScalar> font_size;
        std::optional<ThemeScalar> metrics_rows;
        std::optional<ThemeScalar> metrics_line_height;
        std::optional<ThemeScalar> metrics_padding_x;
        std::optional<ThemeScalar> metrics_padding_y;
    };

    /** Switch 规则：支持 checked 布尔选择器 + 状态选择器，覆盖轨道 / 拇指 / 文本 / 焦点环 / 度量。 */
    using SwitchRecipeRule = struct SwitchRecipeRule {
        std::optional<bool> checked; // nullopt = 任意
        std::optional<SwitchVisualState> state;
        std::optional<ThemeColor> track_fill;
        std::optional<ThemeColor> track_border;
        std::optional<ThemeScalar> track_border_width;
        std::optional<ThemeScalar> track_radius;
        std::optional<ThemeColor> thumb_fill;
        std::optional<ThemeScalar> thumb_radius;
        std::optional<ThemeColor> label_color;
        std::optional<ThemeScalar> label_font_size;
        std::optional<ThemeColor> focus_ring_color;
        std::optional<ThemeScalar> focus_ring_width;
        std::optional<ThemeScalar> metrics_track_width;
        std::optional<ThemeScalar> metrics_track_height;
        std::optional<ThemeScalar> metrics_thumb_size;
        std::optional<ThemeScalar> metrics_gap;
    };

    /** Badge 规则：无选择器（纯展示），覆盖容器 / 文本 / 度量字段。 */
    using BadgeRecipeRule = struct BadgeRecipeRule {
        std::optional<ThemeColor> container_fill;
        std::optional<ThemeColor> container_border;
        std::optional<ThemeScalar> container_border_width;
        std::optional<ThemeScalar> container_radius;
        std::optional<ThemeColor> label_color;
        std::optional<ThemeScalar> label_font_size;
        std::optional<ThemeScalar> metrics_height;
        std::optional<ThemeScalar> metrics_padding_x;
    };

    /** Card 规则：无选择器（纯容器），覆盖容器 / 阴影 / 度量字段。 */
    using CardRecipeRule = struct CardRecipeRule {
        std::optional<ThemeColor> container_fill;
        std::optional<ThemeColor> container_border;
        std::optional<ThemeScalar> container_border_width;
        std::optional<ThemeScalar> container_radius;
        std::optional<ThemeColor> shadow_color;
        std::optional<ThemeScalar> shadow_offset_x;
        std::optional<ThemeScalar> shadow_offset_y;
        std::optional<ThemeScalar> shadow_spread;
        std::optional<ThemeScalar> metrics_padding_x;
        std::optional<ThemeScalar> metrics_padding_y;
        std::optional<ThemeScalar> metrics_min_height;
    };

    /** ProgressBar 规则：支持 disabled 选择器，覆盖轨道 / 填充 / 度量字段。 */
    using ProgressBarRecipeRule = struct ProgressBarRecipeRule {
        std::optional<ProgressBarVisualState> state; // nullopt = 任意
        std::optional<ThemeColor> track_fill;
        std::optional<ThemeColor> track_border;
        std::optional<ThemeScalar> track_border_width;
        std::optional<ThemeScalar> track_radius;
        std::optional<ThemeColor> fill_fill;
        std::optional<ThemeColor> fill_border;
        std::optional<ThemeScalar> fill_radius;
        std::optional<ThemeScalar> metrics_height;
        std::optional<ThemeScalar> metrics_min_height;
        std::optional<ThemeScalar> metrics_preferred_width;
    };

    /** Spinner 规则：支持 disabled 选择器，覆盖指示环颜色 / 组件专属度量字段。 */
    using SpinnerRecipeRule = struct SpinnerRecipeRule {
        std::optional<SpinnerVisualState> state; // nullopt = 任意
        std::optional<ThemeColor> indicator;
        std::optional<ThemeScalar> metrics_diameter;
        std::optional<ThemeScalar> metrics_thickness;
        std::optional<ThemeScalar> metrics_arc_radians;
        std::optional<ThemeScalar> metrics_rotation_speed;
    };

    /** Skeleton 规则：支持状态选择器，覆盖占位块 / 度量字段。 */
    using SkeletonRecipeRule = struct SkeletonRecipeRule {
        std::optional<SkeletonVisualState> state; // nullopt = 任意
        std::optional<ThemeColor> surface_fill;
        std::optional<ThemeColor> surface_border;
        std::optional<ThemeScalar> surface_border_width;
        std::optional<ThemeScalar> surface_radius;
        std::optional<ThemeScalar> metrics_height;
        std::optional<ThemeScalar> metrics_line_gap;
        std::optional<ThemeScalar> metrics_last_line_ratio;
        std::optional<ThemeScalar> metrics_preferred_width;
    };

    /** EmptyState 规则：支持状态选择器，覆盖容器 / 标题 / 描述 / 度量字段。 */
    using EmptyStateRecipeRule = struct EmptyStateRecipeRule {
        std::optional<EmptyStateVisualState> state; // nullopt = 任意
        std::optional<ThemeColor> container_fill;
        std::optional<ThemeColor> container_border;
        std::optional<ThemeScalar> container_border_width;
        std::optional<ThemeScalar> container_radius;
        std::optional<ThemeColor> title_color;
        std::optional<ThemeScalar> title_font_size;
        std::optional<ThemeColor> description_color;
        std::optional<ThemeScalar> description_font_size;
        std::optional<ThemeScalar> metrics_gap;
        std::optional<ThemeScalar> metrics_padding_x;
        std::optional<ThemeScalar> metrics_padding_y;
        std::optional<ThemeScalar> metrics_min_height;
        std::optional<ThemeScalar> metrics_preferred_width;
    };

    /** Alert 规则：支持 tone 选择器（+ 状态选择器），覆盖容器 / 标题 / 描述 / 图标 / 度量字段。 */
    using AlertRecipeRule = struct AlertRecipeRule {
        std::optional<AlertTone> tone;           // nullopt = 任意 tone
        std::optional<AlertVisualState> state;   // nullopt = 任意状态
        std::optional<ThemeColor> container_fill;
        std::optional<ThemeColor> container_border;
        std::optional<ThemeScalar> container_border_width;
        std::optional<ThemeScalar> container_radius;
        std::optional<ThemeColor> title_color;
        std::optional<ThemeScalar> title_font_size;
        std::optional<ThemeColor> description_color;
        std::optional<ThemeScalar> description_font_size;
        std::optional<ThemeColor> icon_color;
        std::optional<ThemeScalar> metrics_gap;
        std::optional<ThemeScalar> metrics_padding_x;
        std::optional<ThemeScalar> metrics_padding_y;
        std::optional<ThemeScalar> metrics_min_height;
        std::optional<ThemeScalar> metrics_box_size;
        std::optional<ThemeScalar> metrics_preferred_width;
    };

    /** RadioButton 规则：支持 checked 布尔选择器 + 状态选择器。 */
    using RadioButtonRecipeRule = struct RadioButtonRecipeRule {
        std::optional<bool> checked; // nullopt = 任意
        std::optional<RadioButtonVisualState> state;
        std::optional<ThemeColor> indicator_fill;
        std::optional<ThemeColor> indicator_border;
        std::optional<ThemeScalar> indicator_border_width;
        std::optional<ThemeScalar> indicator_radius;
        std::optional<ThemeColor> dot_color;
        std::optional<ThemeColor> label_color;
        std::optional<ThemeScalar> label_font_size;
        std::optional<ThemeColor> focus_ring_color;
        std::optional<ThemeScalar> focus_ring_width;
        std::optional<ThemeScalar> metrics_gap;
        std::optional<ThemeScalar> metrics_box_size;
    };

    /**
     * Toggle 规则：支持 tone / treatment / checked / state 选择器。
     *
     * treatment 只描述**未选中**的容器外观；选中态由 `checked = true` 的规则统一换成
     * tone 强调色，保证任何 tone / treatment 组合下 checked 与 unchecked 都能区分。
     */
    using ToggleRecipeRule = struct ToggleRecipeRule {
        std::optional<ButtonTone> tone;              // nullopt = 任意 tone
        std::optional<ButtonTreatment> treatment;    // nullopt = 任意 treatment
        std::optional<bool> checked;                 // nullopt = 任意
        std::optional<ToggleVisualState> state;      // nullopt = 任意状态
        std::optional<ThemeColor> container_fill;
        std::optional<ThemeColor> container_border;
        std::optional<ThemeScalar> container_border_width;
        std::optional<ThemeScalar> container_radius;
        std::optional<ThemeColor> label_color;
        std::optional<ThemeScalar> label_font_size;
        std::optional<ThemeColor> focus_ring_color;
        std::optional<ThemeScalar> focus_ring_width;
        std::optional<ThemeColor> state_layer_hover;
        std::optional<ThemeColor> state_layer_pressed;
        std::optional<ThemeScalar> metrics_height;
        std::optional<ThemeScalar> metrics_padding_x;
        std::optional<ThemeScalar> metrics_min_height;
    };

    /** ToggleGroup 规则：支持状态选择器，覆盖容器 / 度量字段。 */
    using ToggleGroupRecipeRule = struct ToggleGroupRecipeRule {
        std::optional<ToggleGroupVisualState> state; // nullopt = 任意状态
        std::optional<ThemeColor> container_fill;
        std::optional<ThemeColor> container_border;
        std::optional<ThemeScalar> container_border_width;
        std::optional<ThemeScalar> container_radius;
        std::optional<ThemeScalar> metrics_gap;
        std::optional<ThemeScalar> metrics_padding_x;
        std::optional<ThemeScalar> metrics_min_height;
    };

    /** ButtonGroup 规则：支持状态选择器，覆盖容器 / 度量字段。 */
    using ButtonGroupRecipeRule = struct ButtonGroupRecipeRule {
        std::optional<ButtonGroupVisualState> state; // nullopt = 任意状态
        std::optional<ThemeColor> container_fill;
        std::optional<ThemeColor> container_border;
        std::optional<ThemeScalar> container_border_width;
        std::optional<ThemeScalar> container_radius;
        std::optional<ThemeScalar> metrics_gap;
        std::optional<ThemeScalar> metrics_padding_x;
        std::optional<ThemeScalar> metrics_min_height;
    };

    /** Breadcrumb 规则：支持状态选择器，覆盖容器 / 链接 / 当前页 / 分隔符 / 焦点环 / 度量字段。 */
    using BreadcrumbRecipeRule = struct BreadcrumbRecipeRule {
        std::optional<BreadcrumbVisualState> state; // nullopt = 任意状态
        std::optional<ThemeColor> container_fill;
        std::optional<ThemeColor> container_border;
        std::optional<ThemeScalar> container_border_width;
        std::optional<ThemeScalar> container_radius;
        std::optional<ThemeColor> link_color;
        std::optional<ThemeScalar> link_font_size;
        std::optional<ThemeColor> link_hover_color;
        std::optional<ThemeColor> current_color;
        std::optional<ThemeScalar> current_font_size;
        std::optional<ThemeColor> separator_color;
        std::optional<ThemeColor> link_focus_ring_color;
        std::optional<ThemeScalar> link_focus_ring_width;
        std::optional<ThemeScalar> metrics_gap;
        std::optional<ThemeScalar> metrics_padding_x;
        std::optional<ThemeScalar> metrics_min_height;
    };

    /** Pagination 规则：支持状态选择器，覆盖容器 / 槽位面 / 文本 / 省略号 / 焦点环 / 度量字段。 */
    using PaginationRecipeRule = struct PaginationRecipeRule {
        std::optional<PaginationVisualState> state; // nullopt = 任意状态
        std::optional<ThemeColor> container_fill;
        std::optional<ThemeColor> container_border;
        std::optional<ThemeScalar> container_border_width;
        std::optional<ThemeScalar> container_radius;
        std::optional<ThemeColor> item_fill;
        std::optional<ThemeColor> item_border;
        std::optional<ThemeScalar> item_border_width;
        std::optional<ThemeScalar> item_radius;
        std::optional<ThemeColor> item_hover;
        std::optional<ThemeColor> item_active_fill;
        std::optional<ThemeColor> item_active_border;
        std::optional<ThemeScalar> item_active_border_width;
        std::optional<ThemeScalar> item_active_radius;
        std::optional<ThemeColor> label_color;
        std::optional<ThemeScalar> label_font_size;
        std::optional<ThemeColor> label_active_color;
        std::optional<ThemeScalar> label_active_font_size;
        std::optional<ThemeColor> ellipsis_color;
        std::optional<ThemeColor> focus_ring_color;
        std::optional<ThemeScalar> focus_ring_width;
        std::optional<ThemeScalar> metrics_box_size;
        std::optional<ThemeScalar> metrics_gap;
        std::optional<ThemeScalar> metrics_padding_x;
        std::optional<ThemeScalar> metrics_min_height;
    };

    /** Tabs 规则：支持状态选择器，覆盖容器/选中 pill/标签/指示条/焦点环/度量字段。 */
    using TabsRecipeRule = struct TabsRecipeRule {
        std::optional<TabsVisualState> state; // nullopt = 任意
        std::optional<ThemeColor> container_fill;
        std::optional<ThemeColor> container_border;
        std::optional<ThemeScalar> container_border_width;
        std::optional<ThemeScalar> container_radius;
        std::optional<ThemeColor> selected_background_fill;
        std::optional<ThemeScalar> selected_background_radius;
        std::optional<ThemeColor> label_color;
        std::optional<ThemeScalar> label_font_size;
        std::optional<ThemeColor> label_selected_color;
        std::optional<ThemeScalar> label_selected_font_size;
        std::optional<ThemeColor> indicator_color;
        std::optional<ThemeScalar> indicator_thickness;
        std::optional<ThemeColor> focus_ring_color;
        std::optional<ThemeScalar> focus_ring_width;
        std::optional<ThemeScalar> metrics_gap;
        std::optional<ThemeScalar> metrics_padding_x;
        std::optional<ThemeScalar> metrics_min_height;
    };

    /** Tooltip 规则：无选择器（纯展示），覆盖气泡 / 文本 / 度量字段。 */
    using TooltipRecipeRule = struct TooltipRecipeRule {
        std::optional<ThemeColor> container_fill;
        std::optional<ThemeColor> container_border;
        std::optional<ThemeScalar> container_border_width;
        std::optional<ThemeScalar> container_radius;
        std::optional<ThemeColor> label_color;
        std::optional<ThemeScalar> label_font_size;
        std::optional<ThemeScalar> metrics_padding_x;
        std::optional<ThemeScalar> metrics_gap;
        std::optional<ThemeScalar> metrics_min_height;
    };

    /** Select 规则：支持状态选择器，覆盖字段 / 弹窗 / 文本 / 焦点环 / 度量字段。 */
    using SelectRecipeRule = struct SelectRecipeRule {
        std::optional<SelectVisualState> state; // nullopt = 任意
        std::optional<ThemeColor> container_fill;
        std::optional<ThemeColor> container_border;
        std::optional<ThemeScalar> container_border_width;
        std::optional<ThemeScalar> container_radius;
        std::optional<ThemeColor> popup_fill;
        std::optional<ThemeColor> popup_border;
        std::optional<ThemeScalar> popup_border_width;
        std::optional<ThemeScalar> popup_radius;
        std::optional<ThemeColor> value_color;
        std::optional<ThemeColor> option_color;
        std::optional<ThemeColor> option_selected_color;
        std::optional<ThemeScalar> option_font_size;
        std::optional<ThemeColor> focus_ring_color;
        std::optional<ThemeScalar> focus_ring_width;
        std::optional<ThemeScalar> metrics_height;
        std::optional<ThemeScalar> metrics_padding_x;
        std::optional<ThemeScalar> metrics_gap;
        std::optional<ThemeScalar> metrics_min_height;
        std::optional<ThemeScalar> metrics_preferred_width;
    };

    /** Divider 规则：无选择器，覆盖颜色/厚度/首选长度。 */
    using DividerRecipeRule = struct DividerRecipeRule {
        std::optional<ThemeColor> color;
        std::optional<ThemeScalar> thickness;
        std::optional<ThemeScalar> preferred_length;
    };

    /** Avatar 规则：无选择器，覆盖容器/文本/度量字段。 */
    using AvatarRecipeRule = struct AvatarRecipeRule {
        std::optional<ThemeColor> container_fill;
        std::optional<ThemeColor> container_border;
        std::optional<ThemeScalar> container_border_width;
        std::optional<ThemeScalar> container_radius;
        std::optional<ThemeColor> label_color;
        std::optional<ThemeScalar> label_font_size;
        std::optional<ThemeScalar> metrics_box_size;
    };

    /** Chip 规则：无选择器，覆盖容器/文本/移除标记/度量字段。 */
    using ChipRecipeRule = struct ChipRecipeRule {
        std::optional<ThemeColor> container_fill;
        std::optional<ThemeColor> container_border;
        std::optional<ThemeScalar> container_border_width;
        std::optional<ThemeScalar> container_radius;
        std::optional<ThemeColor> label_color;
        std::optional<ThemeScalar> label_font_size;
        std::optional<ThemeColor> remove_color;
        std::optional<ThemeColor> focus_ring_color;
        std::optional<ThemeScalar> focus_ring_width;
        std::optional<ThemeScalar> metrics_height;
        std::optional<ThemeScalar> metrics_padding_x;
        std::optional<ThemeScalar> metrics_gap;
    };

    /** Popover 规则：无选择器（非模态容器），覆盖面板 / 度量字段。 */
    using PopoverRecipeRule = struct PopoverRecipeRule {
        std::optional<ThemeColor> panel_fill;
        std::optional<ThemeColor> panel_border;
        std::optional<ThemeScalar> panel_border_width;
        std::optional<ThemeScalar> panel_radius;
        std::optional<ThemeScalar> metrics_padding_x;
        std::optional<ThemeScalar> metrics_padding_y;
        std::optional<ThemeScalar> metrics_gap;
        std::optional<ThemeScalar> metrics_min_height;
    };

    /** DropdownMenu 规则：无选择器（逐条目状态在绘制时读取），覆盖条目排版 / 状态色 / 度量。 */
    using DropdownMenuRecipeRule = struct DropdownMenuRecipeRule {
        std::optional<ThemeColor> item_label_color;
        std::optional<ThemeScalar> item_label_font_size;
        std::optional<ThemeColor> item_shortcut_color;
        std::optional<ThemeScalar> item_shortcut_font_size;
        std::optional<ThemeColor> group_label_color;
        std::optional<ThemeScalar> group_label_font_size;
        std::optional<ThemeColor> disabled_label;
        std::optional<ThemeColor> hover_fill;
        std::optional<ThemeColor> focus_fill;
        std::optional<ThemeColor> checked_indicator;
        std::optional<ThemeColor> separator;
        std::optional<ThemeScalar> metrics_item_height;
        std::optional<ThemeScalar> metrics_padding_x;
        std::optional<ThemeScalar> metrics_padding_y;
        std::optional<ThemeScalar> metrics_gap;
        std::optional<ThemeScalar> metrics_item_radius;
        std::optional<ThemeScalar> metrics_separator_thickness;
        std::optional<ThemeScalar> metrics_min_width;
    };

    /**
     * Combobox 规则：状态选择器覆盖输入框外壳 / 文本 / 焦点环，并覆盖选项列表的
     * 排版 / 状态色 / 度量（逐条目 hover / focus 在绘制时读取，不进选择器）。
     */
    using ComboboxRecipeRule = struct ComboboxRecipeRule {
        std::optional<ComboboxVisualState> state; // nullopt = 任意
        std::optional<ThemeColor> input_fill;
        std::optional<ThemeColor> input_border;
        std::optional<ThemeScalar> input_border_width;
        std::optional<ThemeScalar> input_radius;
        std::optional<ThemeColor> value_color;
        std::optional<ThemeScalar> value_font_size;
        std::optional<ThemeColor> placeholder_color;
        std::optional<ThemeScalar> placeholder_font_size;
        std::optional<ThemeColor> selection_color;
        std::optional<ThemeColor> focus_ring_color;
        std::optional<ThemeScalar> focus_ring_width;
        std::optional<ThemeColor> option_color;
        std::optional<ThemeScalar> option_font_size;
        std::optional<ThemeColor> disabled_label;
        std::optional<ThemeColor> hover_fill;
        std::optional<ThemeColor> focus_fill;
        std::optional<ThemeScalar> metrics_height;
        std::optional<ThemeScalar> metrics_padding_x;
        std::optional<ThemeScalar> metrics_preferred_width;
        std::optional<ThemeScalar> metrics_gap;
        std::optional<ThemeScalar> metrics_item_height;
        std::optional<ThemeScalar> metrics_list_padding_x;
        std::optional<ThemeScalar> metrics_list_padding_y;
        std::optional<ThemeScalar> metrics_item_radius;
        std::optional<ThemeScalar> metrics_min_width;
    };

    /** Dialog 规则：无选择器，覆盖遮罩/面板/标题/度量字段。 */
    using DialogRecipeRule = struct DialogRecipeRule {
        std::optional<ThemeColor> scrim;
        std::optional<ThemeColor> panel_fill;
        std::optional<ThemeColor> panel_border;
        std::optional<ThemeScalar> panel_border_width;
        std::optional<ThemeScalar> panel_radius;
        std::optional<ThemeColor> title_color;
        std::optional<ThemeScalar> title_font_size;
        std::optional<ThemeScalar> metrics_panel_width;
        std::optional<ThemeScalar> metrics_padding_x;
        std::optional<ThemeScalar> metrics_padding_y;
        std::optional<ThemeScalar> metrics_gap;
        std::optional<ThemeScalar> metrics_min_height;
    };

    // ─── 解析后的配方（控件绘制时消费） ──────────────────────────────────────

    /** 解析后的状态层（具体叠加色；hover/focused 用 hover，pressed 用 pressed）。 */
    using ResolvedStateLayer = struct ResolvedStateLayer {
        NanColor hover;
        NanColor pressed;
    };

    using ResolvedRippleStyle = struct ResolvedRippleStyle {
        NanColor color;
        float duration = 0.0F;
    };

    using ResolvedButtonStyle = struct ResolvedButtonStyle {
        ResolvedBoxStyle container;
        ResolvedTypeStyle label;
        ResolvedFocusRing focus;
        ResolvedStateLayer state_layer;
        ResolvedRippleStyle ripple;
        ResolvedControlMetrics metrics;
    };

    using ResolvedCheckboxStyle = struct ResolvedCheckboxStyle {
        ResolvedBoxStyle indicator;
        /** 勾选标记（对勾）颜色。 */
        NanColor check;
        ResolvedTypeStyle label;
        ResolvedFocusRing focus;
        ResolvedControlMetrics metrics;
    };

    using ResolvedSliderStyle = struct ResolvedSliderStyle {
        ResolvedTrackStyle inactive_track;
        ResolvedTrackStyle active_track;
        ResolvedThumbStyle thumb;
        ResolvedFocusRing focus;
        ResolvedControlMetrics metrics;
    };

    using ResolvedTextFieldStyle = struct ResolvedTextFieldStyle {
        ResolvedBoxStyle container;
        ResolvedTypeStyle value;
        ResolvedTypeStyle placeholder;
        NanColor selection;
        ResolvedFocusRing focus;
        ResolvedControlMetrics metrics;
    };

    /** 解析后的 TextArea 度量。 */
    using ResolvedTextAreaMetrics = struct ResolvedTextAreaMetrics {
        float rows = 0.0F;
        float line_height = 0.0F;
        float padding_x = 0.0F;
        float padding_y = 0.0F;
    };

    using ResolvedTextAreaStyle = struct ResolvedTextAreaStyle {
        ResolvedBoxStyle container;
        ResolvedTypeStyle value;
        ResolvedTypeStyle placeholder;
        NanColor selection;
        ResolvedFocusRing focus;
        ResolvedTextAreaMetrics metrics;
    };

    using ResolvedSwitchStyle = struct ResolvedSwitchStyle {
        ResolvedBoxStyle track;
        ResolvedBoxStyle thumb;
        ResolvedTypeStyle label;
        ResolvedFocusRing focus;
        ResolvedSwitchMetrics metrics;
    };

    using ResolvedBadgeStyle = struct ResolvedBadgeStyle {
        ResolvedBoxStyle container;
        ResolvedTypeStyle label;
        ResolvedControlMetrics metrics;
    };

    using ResolvedCardMetrics = struct ResolvedCardMetrics {
        float padding_x = 0.0F;
        float padding_y = 0.0F;
        float min_height = 0.0F;
    };

    using ResolvedCardStyle = struct ResolvedCardStyle {
        ResolvedBoxStyle container;
        ResolvedShadowStyle shadow;
        ResolvedCardMetrics metrics;
    };

    using ResolvedProgressBarStyle = struct ResolvedProgressBarStyle {
        ResolvedBoxStyle track;
        ResolvedBoxStyle fill;
        ResolvedControlMetrics metrics;
    };

    using ResolvedSpinnerMetrics = struct ResolvedSpinnerMetrics {
        float diameter = 0.0F;
        float thickness = 0.0F;
        float arc_radians = 0.0F;
        float rotation_speed = 0.0F;
    };

    using ResolvedSpinnerStyle = struct ResolvedSpinnerStyle {
        NanColor indicator;
        ResolvedSpinnerMetrics metrics;
    };

    using ResolvedSkeletonMetrics = struct ResolvedSkeletonMetrics {
        float height = 0.0F;
        float line_gap = 0.0F;
        float last_line_ratio = 1.0F;
        float preferred_width = 0.0F;
    };

    using ResolvedSkeletonStyle = struct ResolvedSkeletonStyle {
        ResolvedBoxStyle surface;
        ResolvedSkeletonMetrics metrics;
    };

    using ResolvedEmptyStateMetrics = struct ResolvedEmptyStateMetrics {
        float gap = 0.0F;
        float padding_x = 0.0F;
        float padding_y = 0.0F;
        float min_height = 0.0F;
        float preferred_width = 0.0F;
    };

    using ResolvedEmptyStateStyle = struct ResolvedEmptyStateStyle {
        ResolvedBoxStyle container;
        ResolvedTypeStyle title;
        ResolvedTypeStyle description;
        ResolvedEmptyStateMetrics metrics;
    };

    using ResolvedAlertMetrics = struct ResolvedAlertMetrics {
        float gap = 0.0F;
        float padding_x = 0.0F;
        float padding_y = 0.0F;
        float min_height = 0.0F;
        float box_size = 0.0F;
        float preferred_width = 0.0F;
    };

    using ResolvedAlertStyle = struct ResolvedAlertStyle {
        ResolvedBoxStyle container;
        ResolvedTypeStyle title;
        ResolvedTypeStyle description;
        NanColor icon;
        ResolvedAlertMetrics metrics;
    };

    using ResolvedRadioButtonStyle = struct ResolvedRadioButtonStyle {
        ResolvedBoxStyle indicator;
        NanColor dot;
        ResolvedTypeStyle label;
        ResolvedFocusRing focus;
        ResolvedControlMetrics metrics;
    };

    using ResolvedToggleStyle = struct ResolvedToggleStyle {
        ResolvedBoxStyle container;
        ResolvedTypeStyle label;
        ResolvedFocusRing focus;
        ResolvedStateLayer state_layer;
        ResolvedControlMetrics metrics;
    };

    using ResolvedToggleGroupStyle = struct ResolvedToggleGroupStyle {
        ResolvedBoxStyle container;
        ResolvedControlMetrics metrics;
    };

    using ResolvedButtonGroupStyle = struct ResolvedButtonGroupStyle {
        ResolvedBoxStyle container;
        ResolvedControlMetrics metrics;
    };

    using ResolvedBreadcrumbStyle = struct ResolvedBreadcrumbStyle {
        ResolvedBoxStyle container;
        ResolvedTypeStyle link;
        NanColor link_hover;
        ResolvedTypeStyle current;
        NanColor separator;
        ResolvedFocusRing link_focus;
        ResolvedControlMetrics metrics;
    };

    using ResolvedPaginationStyle = struct ResolvedPaginationStyle {
        ResolvedBoxStyle container;
        ResolvedBoxStyle item;
        NanColor item_hover;
        ResolvedBoxStyle item_active;
        ResolvedTypeStyle label;
        ResolvedTypeStyle label_active;
        NanColor ellipsis;
        ResolvedFocusRing focus;
        ResolvedControlMetrics metrics;
    };

    using ResolvedTabsStyle = struct ResolvedTabsStyle {
        ResolvedBoxStyle container;
        ResolvedBoxStyle selected_background;
        ResolvedTypeStyle label;
        ResolvedTypeStyle label_selected;
        NanColor indicator;
        float indicator_thickness = 0.0F;
        ResolvedFocusRing focus;
        ResolvedControlMetrics metrics;
    };

    using ResolvedTooltipStyle = struct ResolvedTooltipStyle {
        ResolvedBoxStyle container;
        ResolvedTypeStyle label;
        ResolvedControlMetrics metrics;
    };

    using ResolvedSelectStyle = struct ResolvedSelectStyle {
        ResolvedBoxStyle container;
        ResolvedBoxStyle popup;
        ResolvedTypeStyle value;
        ResolvedTypeStyle option;
        ResolvedTypeStyle option_selected;
        ResolvedFocusRing focus;
        ResolvedControlMetrics metrics;
    };

    using ResolvedDividerStyle = struct ResolvedDividerStyle {
        NanColor color;
        float thickness = 0.0F;
        float preferred_length = 0.0F;
    };

    using ResolvedAvatarStyle = struct ResolvedAvatarStyle {
        ResolvedBoxStyle container;
        ResolvedTypeStyle label;
        ResolvedControlMetrics metrics;
    };

    using ResolvedChipStyle = struct ResolvedChipStyle {
        ResolvedBoxStyle container;
        ResolvedTypeStyle label;
        NanColor remove_color;
        ResolvedFocusRing focus;
        ResolvedControlMetrics metrics;
    };

    using ResolvedPopoverMetrics = struct ResolvedPopoverMetrics {
        float padding_x = 0.0F;
        float padding_y = 0.0F;
        float gap = 0.0F;
        float min_height = 0.0F;
    };

    using ResolvedPopoverStyle = struct ResolvedPopoverStyle {
        ResolvedBoxStyle panel;
        ResolvedPopoverMetrics metrics;
    };

    using ResolvedDropdownMenuMetrics = struct ResolvedDropdownMenuMetrics {
        float item_height = 0.0F;
        float padding_x = 0.0F;
        float padding_y = 0.0F;
        float gap = 0.0F;
        float item_radius = 0.0F;
        float separator_thickness = 0.0F;
        float min_width = 0.0F;
    };

    using ResolvedDropdownMenuStyle = struct ResolvedDropdownMenuStyle {
        ResolvedTypeStyle item_label;
        ResolvedTypeStyle item_shortcut;
        ResolvedTypeStyle group_label;
        NanColor disabled_label;
        NanColor hover_fill;
        NanColor focus_fill;
        NanColor checked_indicator;
        NanColor separator;
        ResolvedDropdownMenuMetrics metrics;
    };

    using ResolvedComboboxMetrics = struct ResolvedComboboxMetrics {
        float height = 0.0F;
        float padding_x = 0.0F;
        float preferred_width = 0.0F;
        float gap = 0.0F;
        float item_height = 0.0F;
        float list_padding_x = 0.0F;
        float list_padding_y = 0.0F;
        float item_radius = 0.0F;
        float min_width = 0.0F;
    };

    using ResolvedComboboxStyle = struct ResolvedComboboxStyle {
        /** 输入框外壳（由组合的 TextField 按本样式绘制）。 */
        ResolvedBoxStyle input;
        ResolvedTypeStyle value;
        ResolvedTypeStyle placeholder;
        NanColor selection;
        ResolvedFocusRing focus;
        ResolvedTypeStyle option;
        NanColor disabled_label;
        NanColor hover_fill;
        NanColor focus_fill;
        ResolvedComboboxMetrics metrics;
    };

    using ResolvedDialogMetrics = struct ResolvedDialogMetrics {
        float panel_width = 0.0F;
        float padding_x = 0.0F;
        float padding_y = 0.0F;
        float gap = 0.0F;
        float min_height = 0.0F;
    };

    using ResolvedDialogStyle = struct ResolvedDialogStyle {
        NanColor scrim;
        ResolvedBoxStyle panel;
        ResolvedTypeStyle title;
        ResolvedDialogMetrics metrics;
    };

    // ─── Typography 角色 ──────────────────────────────────────────────────────

    /** 命名排版角色；配方内的文本片段可引用这些角色或直接覆盖。 */
    using TypographyRoles = struct TypographyRoles {
        TypeStyle label_sm;
        TypeStyle label_md;
        TypeStyle label_lg;
    };

    // ─── 配方书与 DesignSystem ────────────────────────────────────────────────

    using ButtonRecipes = struct ButtonRecipes {
        ButtonRecipe base;
        std::vector<ButtonRecipeRule> rules;
    };

    using CheckboxRecipes = struct CheckboxRecipes {
        CheckboxRecipe base;
        std::vector<CheckboxRecipeRule> rules;
    };

    using SliderRecipes = struct SliderRecipes {
        SliderRecipe base;
        std::vector<SliderRecipeRule> rules;
    };

    using TextFieldRecipes = struct TextFieldRecipes {
        TextFieldRecipe base;
        std::vector<TextFieldRecipeRule> rules;
    };

    using TextAreaRecipes = struct TextAreaRecipes {
        TextAreaRecipe base;
        std::vector<TextAreaRecipeRule> rules;
    };

    using SwitchRecipes = struct SwitchRecipes {
        SwitchRecipe base;
        std::vector<SwitchRecipeRule> rules;
    };

    using BadgeRecipes = struct BadgeRecipes {
        BadgeRecipe base;
        std::vector<BadgeRecipeRule> rules;
    };

    using CardRecipes = struct CardRecipes {
        CardRecipe base;
        std::vector<CardRecipeRule> rules;
    };

    using ProgressBarRecipes = struct ProgressBarRecipes {
        ProgressBarRecipe base;
        std::vector<ProgressBarRecipeRule> rules;
    };

    using SpinnerRecipes = struct SpinnerRecipes {
        SpinnerRecipe base;
        std::vector<SpinnerRecipeRule> rules;
    };

    using SkeletonRecipes = struct SkeletonRecipes {
        SkeletonRecipe base;
        std::vector<SkeletonRecipeRule> rules;
    };

    using EmptyStateRecipes = struct EmptyStateRecipes {
        EmptyStateRecipe base;
        std::vector<EmptyStateRecipeRule> rules;
    };

    using AlertRecipes = struct AlertRecipes {
        AlertRecipe base;
        std::vector<AlertRecipeRule> rules;
    };

    using RadioButtonRecipes = struct RadioButtonRecipes {
        RadioButtonRecipe base;
        std::vector<RadioButtonRecipeRule> rules;
    };

    using ToggleRecipes = struct ToggleRecipes {
        ToggleRecipe base;
        std::vector<ToggleRecipeRule> rules;
    };

    using ToggleGroupRecipes = struct ToggleGroupRecipes {
        ToggleGroupRecipe base;
        std::vector<ToggleGroupRecipeRule> rules;
    };

    using ButtonGroupRecipes = struct ButtonGroupRecipes {
        ButtonGroupRecipe base;
        std::vector<ButtonGroupRecipeRule> rules;
    };

    using BreadcrumbRecipes = struct BreadcrumbRecipes {
        BreadcrumbRecipe base;
        std::vector<BreadcrumbRecipeRule> rules;
    };

    using PaginationRecipes = struct PaginationRecipes {
        PaginationRecipe base;
        std::vector<PaginationRecipeRule> rules;
    };

    using TabsRecipes = struct TabsRecipes {
        TabsRecipe base;
        std::vector<TabsRecipeRule> rules;
    };

    using TooltipRecipes = struct TooltipRecipes {
        TooltipRecipe base;
        std::vector<TooltipRecipeRule> rules;
    };

    using SelectRecipes = struct SelectRecipes {
        SelectRecipe base;
        std::vector<SelectRecipeRule> rules;
    };

    using DividerRecipes = struct DividerRecipes {
        DividerRecipe base;
        std::vector<DividerRecipeRule> rules;
    };

    using AvatarRecipes = struct AvatarRecipes {
        AvatarRecipe base;
        std::vector<AvatarRecipeRule> rules;
    };

    using ChipRecipes = struct ChipRecipes {
        ChipRecipe base;
        std::vector<ChipRecipeRule> rules;
    };

    using DialogRecipes = struct DialogRecipes {
        DialogRecipe base;
        std::vector<DialogRecipeRule> rules;
    };

    using PopoverRecipes = struct PopoverRecipes {
        PopoverRecipe base;
        std::vector<PopoverRecipeRule> rules;
    };

    using DropdownMenuRecipes = struct DropdownMenuRecipes {
        DropdownMenuRecipe base;
        std::vector<DropdownMenuRecipeRule> rules;
    };

    using ComboboxRecipes = struct ComboboxRecipes {
        ComboboxRecipe base;
        std::vector<ComboboxRecipeRule> rules;
    };

    using ComponentRecipes = struct ComponentRecipes {
        ButtonRecipes button;
        CheckboxRecipes checkbox;
        SliderRecipes slider;
        TextFieldRecipes text_field;
        TextAreaRecipes text_area;
        SwitchRecipes switch_component;
        BadgeRecipes badge;
        CardRecipes card;
        ProgressBarRecipes progress_bar;
        SpinnerRecipes spinner;
        SkeletonRecipes skeleton;
        EmptyStateRecipes empty_state;
        AlertRecipes alert;
        RadioButtonRecipes radio_button;
        ToggleRecipes toggle;
        ToggleGroupRecipes toggle_group;
        ButtonGroupRecipes button_group;
        BreadcrumbRecipes breadcrumb;
        PaginationRecipes pagination;
        TabsRecipes tabs;
        TooltipRecipes tooltip;
        SelectRecipes select;
        DividerRecipes divider;
        AvatarRecipes avatar;
        ChipRecipes chip;
        DialogRecipes dialog;
        PopoverRecipes popover;
        DropdownMenuRecipes dropdown_menu;
        ComboboxRecipes combobox;
    };

    /**
     * 不可变、与外观无关的设计系统快照。调用方修改一份拷贝后交给
     * ThemeManager::apply() 原子替换并发布一次 revision。
     */
    using DesignSystem = struct DesignSystem {
        NanTokens tokens;
        NanColorScheme light;
        NanColorScheme dark;
        TypographyRoles typography;
        ComponentRecipes components;

        /** 按外观选择语义调色板变体。 */
        [[nodiscard]] auto palette(const ColorAppearance appearance) const noexcept
            -> const NanColorScheme& {
            return appearance == ColorAppearance::dark ? dark : light;
        }
    };

    // ─── 解析 ──────────────────────────────────────────────────────────────────

    /**
     * 将 ThemeColor 解析为具体颜色（按当前外观选择 light/dark palette）。
     *
     * @param system     目标设计系统快照
     * @param appearance 当前外观
     * @param value      token-or-literal 颜色值
     * @param tone       当前 Button tone（accent / on_accent 引用依赖它；缺省按 primary）
     * @return 解析后的 NanColor
     */
    [[nodiscard]] inline auto resolve_color(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const ThemeColor& value,
        const std::optional<ButtonTone> tone = std::nullopt
    ) -> NanColor {
        return resolve_theme_color(NanTheme {system.tokens, system.palette(appearance)}, value, tone);
    }

    /**
     * 将 ThemeScalar 解析为具体数值。
     *
     * @param system     目标设计系统快照
     * @param appearance 当前外观
     * @param value      token-or-literal 标量值
     * @return 解析后的 float
     */
    [[nodiscard]] inline auto
    resolve_scalar(const DesignSystem& system, const ColorAppearance appearance, const ThemeScalar& value)
        -> float {
        return resolve_theme_scalar(NanTheme {system.tokens, system.palette(appearance)}, value);
    }

    /** 解析矩形槽位片段为具体值。 */
    [[nodiscard]] inline auto resolve(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const BoxStyle& box
    ) -> ResolvedBoxStyle {
        return {
            .fill = resolve_color(system, appearance, box.fill),
            .border = resolve_color(system, appearance, box.border),
            .border_width = resolve_scalar(system, appearance, box.border_width),
            .radius = resolve_scalar(system, appearance, box.radius),
        };
    }

    /** 解析排版片段为具体值。 */
    [[nodiscard]] inline auto resolve(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const TypeStyle& type
    ) -> ResolvedTypeStyle {
        return {
            .color = resolve_color(system, appearance, type.color),
            .font_size = resolve_scalar(system, appearance, type.font_size),
        };
    }

    /** 解析焦点环片段为具体值。 */
    [[nodiscard]] inline auto resolve(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const FocusRingStyle& ring
    ) -> ResolvedFocusRing {
        return {
            .color = resolve_color(system, appearance, ring.color),
            .width = resolve_scalar(system, appearance, ring.width),
        };
    }

    /** 解析轨道片段为具体值。 */
    [[nodiscard]] inline auto resolve(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const TrackStyle& track
    ) -> ResolvedTrackStyle {
        return {
            .box = resolve(system, appearance, track.box),
            .thickness = resolve_scalar(system, appearance, track.thickness),
        };
    }

    /** 解析拇指片段为具体值。 */
    [[nodiscard]] inline auto resolve(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const ThumbStyle& thumb
    ) -> ResolvedThumbStyle {
        return {.box = resolve(system, appearance, thumb.box)};
    }

    /** 解析度量片段为具体值。 */
    [[nodiscard]] inline auto resolve(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const ControlMetrics& metrics
    ) -> ResolvedControlMetrics {
        return {
            .height = resolve_scalar(system, appearance, metrics.height),
            .padding_x = resolve_scalar(system, appearance, metrics.padding_x),
            .gap = resolve_scalar(system, appearance, metrics.gap),
            .min_height = resolve_scalar(system, appearance, metrics.min_height),
            .box_size = resolve_scalar(system, appearance, metrics.box_size),
            .preferred_width = resolve_scalar(system, appearance, metrics.preferred_width),
        };
    }

    /** 解析 Dialog 度量片段为具体值。 */
    [[nodiscard]] inline auto resolve(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const DialogMetrics& metrics
    ) -> ResolvedDialogMetrics {
        return {
            .panel_width = resolve_scalar(system, appearance, metrics.panel_width),
            .padding_x = resolve_scalar(system, appearance, metrics.padding_x),
            .padding_y = resolve_scalar(system, appearance, metrics.padding_y),
            .gap = resolve_scalar(system, appearance, metrics.gap),
            .min_height = resolve_scalar(system, appearance, metrics.min_height),
        };
    }

    /** 解析软阴影片段为具体值。 */
    [[nodiscard]] inline auto resolve(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const ShadowStyle& shadow
    ) -> ResolvedShadowStyle {
        return {
            .color = resolve_color(system, appearance, shadow.color),
            .offset_x = resolve_scalar(system, appearance, shadow.offset_x),
            .offset_y = resolve_scalar(system, appearance, shadow.offset_y),
            .spread = resolve_scalar(system, appearance, shadow.spread),
        };
    }

    /** 解析 Switch 度量片段为具体值。 */
    [[nodiscard]] inline auto resolve(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const SwitchMetrics& metrics
    ) -> ResolvedSwitchMetrics {
        return {
            .track_width = resolve_scalar(system, appearance, metrics.track_width),
            .track_height = resolve_scalar(system, appearance, metrics.track_height),
            .thumb_size = resolve_scalar(system, appearance, metrics.thumb_size),
            .gap = resolve_scalar(system, appearance, metrics.gap),
            .min_height = resolve_scalar(system, appearance, metrics.min_height),
        };
    }

    /** 解析 Spinner 度量片段为具体值。 */
    [[nodiscard]] inline auto resolve(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const SpinnerMetrics& metrics
    ) -> ResolvedSpinnerMetrics {
        return {
            .diameter = resolve_scalar(system, appearance, metrics.diameter),
            .thickness = resolve_scalar(system, appearance, metrics.thickness),
            .arc_radians = resolve_scalar(system, appearance, metrics.arc_radians),
            .rotation_speed = resolve_scalar(system, appearance, metrics.rotation_speed),
        };
    }

    /** 解析 Alert 度量片段为具体值。 */
    [[nodiscard]] inline auto resolve(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const AlertMetrics& metrics
    ) -> ResolvedAlertMetrics {
        return {
            .gap = resolve_scalar(system, appearance, metrics.gap),
            .padding_x = resolve_scalar(system, appearance, metrics.padding_x),
            .padding_y = resolve_scalar(system, appearance, metrics.padding_y),
            .min_height = resolve_scalar(system, appearance, metrics.min_height),
            .box_size = resolve_scalar(system, appearance, metrics.box_size),
            .preferred_width = resolve_scalar(system, appearance, metrics.preferred_width),
        };
    }

    // 组件级解析（定义见 design_system.cpp）：
    //   遗留平铺解析器给出 base 语义（tone/treatment/size/state）→
    //   应用 DesignSystem 的规则覆盖 → 组装为片段组合的解析结果。

    [[nodiscard]] auto resolve_button(
        const DesignSystem& system,
        ColorAppearance appearance,
        ButtonTone tone,
        ButtonTreatment treatment,
        ButtonSize size,
        ButtonVisualState state
    ) -> ResolvedButtonStyle;

    [[nodiscard]] auto resolve_checkbox(
        const DesignSystem& system,
        ColorAppearance appearance,
        bool checked,
        CheckboxVisualState state
    ) -> ResolvedCheckboxStyle;

    [[nodiscard]] auto resolve_slider(
        const DesignSystem& system,
        ColorAppearance appearance,
        SliderVisualState state
    ) -> ResolvedSliderStyle;

    [[nodiscard]] auto resolve_text_field(
        const DesignSystem& system,
        ColorAppearance appearance,
        TextFieldVisualState state
    ) -> ResolvedTextFieldStyle;

    [[nodiscard]] auto resolve_text_area(
        const DesignSystem& system,
        ColorAppearance appearance,
        TextAreaVisualState state,
        bool read_only
    ) -> ResolvedTextAreaStyle;

    [[nodiscard]] auto resolve_switch(
        const DesignSystem& system,
        ColorAppearance appearance,
        bool checked,
        SwitchVisualState state
    ) -> ResolvedSwitchStyle;

    [[nodiscard]] auto resolve_badge(
        const DesignSystem& system,
        ColorAppearance appearance
    ) -> ResolvedBadgeStyle;

    [[nodiscard]] auto resolve_card(
        const DesignSystem& system,
        ColorAppearance appearance
    ) -> ResolvedCardStyle;

    [[nodiscard]] auto resolve_progress_bar(
        const DesignSystem& system,
        ColorAppearance appearance,
        ProgressBarVisualState state
    ) -> ResolvedProgressBarStyle;

    [[nodiscard]] auto resolve_spinner(
        const DesignSystem& system,
        ColorAppearance appearance,
        SpinnerVisualState state
    ) -> ResolvedSpinnerStyle;

    [[nodiscard]] auto resolve_skeleton(
        const DesignSystem& system,
        ColorAppearance appearance,
        SkeletonVisualState state
    ) -> ResolvedSkeletonStyle;

    [[nodiscard]] auto resolve_empty_state(
        const DesignSystem& system,
        ColorAppearance appearance,
        EmptyStateVisualState state
    ) -> ResolvedEmptyStateStyle;

    [[nodiscard]] auto resolve_alert(
        const DesignSystem& system,
        ColorAppearance appearance,
        AlertTone tone,
        AlertVisualState state
    ) -> ResolvedAlertStyle;

    [[nodiscard]] auto resolve_radio_button(
        const DesignSystem& system,
        ColorAppearance appearance,
        bool checked,
        RadioButtonVisualState state
    ) -> ResolvedRadioButtonStyle;

    [[nodiscard]] auto resolve_toggle(
        const DesignSystem& system,
        ColorAppearance appearance,
        ButtonTone tone,
        ButtonTreatment treatment,
        bool checked,
        ToggleVisualState state
    ) -> ResolvedToggleStyle;

    [[nodiscard]] auto resolve_toggle_group(
        const DesignSystem& system,
        ColorAppearance appearance,
        ToggleGroupVisualState state
    ) -> ResolvedToggleGroupStyle;

    [[nodiscard]] auto resolve_button_group(
        const DesignSystem& system,
        ColorAppearance appearance,
        ButtonGroupVisualState state
    ) -> ResolvedButtonGroupStyle;

    [[nodiscard]] auto resolve_breadcrumb(
        const DesignSystem& system,
        ColorAppearance appearance,
        BreadcrumbVisualState state
    ) -> ResolvedBreadcrumbStyle;

    [[nodiscard]] auto resolve_pagination(
        const DesignSystem& system,
        ColorAppearance appearance,
        PaginationVisualState state
    ) -> ResolvedPaginationStyle;

    [[nodiscard]] auto resolve_tabs(
        const DesignSystem& system,
        ColorAppearance appearance,
        TabsVisualState state
    ) -> ResolvedTabsStyle;

    [[nodiscard]] auto resolve_tooltip(
        const DesignSystem& system,
        ColorAppearance appearance
    ) -> ResolvedTooltipStyle;

    [[nodiscard]] auto resolve_select(
        const DesignSystem& system,
        ColorAppearance appearance,
        SelectVisualState state
    ) -> ResolvedSelectStyle;

    [[nodiscard]] auto resolve_divider(
        const DesignSystem& system,
        ColorAppearance appearance
    ) -> ResolvedDividerStyle;

    [[nodiscard]] auto resolve_avatar(
        const DesignSystem& system,
        ColorAppearance appearance
    ) -> ResolvedAvatarStyle;

    [[nodiscard]] auto resolve_chip(
        const DesignSystem& system,
        ColorAppearance appearance
    ) -> ResolvedChipStyle;

    [[nodiscard]] auto resolve_dialog(
        const DesignSystem& system,
        ColorAppearance appearance
    ) -> ResolvedDialogStyle;

    [[nodiscard]] auto resolve_popover(
        const DesignSystem& system,
        ColorAppearance appearance
    ) -> ResolvedPopoverStyle;

    [[nodiscard]] auto resolve_dropdown_menu(
        const DesignSystem& system,
        ColorAppearance appearance
    ) -> ResolvedDropdownMenuStyle;

    [[nodiscard]] auto resolve_combobox(
        const DesignSystem& system,
        ColorAppearance appearance,
        ComboboxVisualState state
    ) -> ResolvedComboboxStyle;

    // 规则覆盖：把配方规则应用到已解析的配方。解析器与 widget 的 set_override 共用同一路径。

    /** @param tone 当前 Button tone（accent / on_accent 引用依赖它）。 */
    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedButtonStyle& style,
        const ButtonRecipeRule& rule,
        ButtonTone tone
    );

    /** @return 当前交互状态对应的独立叠加色；normal / disabled 返回透明色。 */
    [[nodiscard]] auto button_state_layer_color(
        const ResolvedButtonStyle& style,
        ButtonVisualState state
    ) -> NanColor;

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedCheckboxStyle& style,
        const CheckboxRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedSliderStyle& style,
        const SliderRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedTextFieldStyle& style,
        const TextFieldRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedTextAreaStyle& style,
        const TextAreaRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedSwitchStyle& style,
        const SwitchRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedBadgeStyle& style,
        const BadgeRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedCardStyle& style,
        const CardRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedProgressBarStyle& style,
        const ProgressBarRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedSpinnerStyle& style,
        const SpinnerRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedSkeletonStyle& style,
        const SkeletonRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedEmptyStateStyle& style,
        const EmptyStateRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedAlertStyle& style,
        const AlertRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedRadioButtonStyle& style,
        const RadioButtonRecipeRule& rule
    );

    /** @param tone 当前 Button tone（accent / on_accent 引用依赖它）。 */
    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedToggleStyle& style,
        const ToggleRecipeRule& rule,
        ButtonTone tone
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedToggleGroupStyle& style,
        const ToggleGroupRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedButtonGroupStyle& style,
        const ButtonGroupRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedBreadcrumbStyle& style,
        const BreadcrumbRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedPaginationStyle& style,
        const PaginationRecipeRule& rule
    );

    /**
     * @return Toggle 当前交互状态对应的独立叠加色；normal / disabled 返回透明色。
     * 与 `button_state_layer_color` 同款：状态反馈不写回基础容器填充。
     */
    [[nodiscard]] auto toggle_state_layer_color(
        const ResolvedToggleStyle& style,
        ToggleVisualState state
    ) -> NanColor;

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedTabsStyle& style,
        const TabsRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedTooltipStyle& style,
        const TooltipRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedSelectStyle& style,
        const SelectRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedDividerStyle& style,
        const DividerRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedAvatarStyle& style,
        const AvatarRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedChipStyle& style,
        const ChipRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedDialogStyle& style,
        const DialogRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedPopoverStyle& style,
        const PopoverRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedDropdownMenuStyle& style,
        const DropdownMenuRecipeRule& rule
    );

    void apply_rule(
        const DesignSystem& system,
        ColorAppearance appearance,
        ResolvedComboboxStyle& style,
        const ComboboxRecipeRule& rule
    );

    // ─── 框架默认值（定义见 design_system.cpp） ──────────────────────────────

    [[nodiscard]] auto default_button_recipe() -> ButtonRecipe;
    [[nodiscard]] auto default_checkbox_recipe() -> CheckboxRecipe;
    [[nodiscard]] auto default_slider_recipe() -> SliderRecipe;
    [[nodiscard]] auto default_text_field_recipe() -> TextFieldRecipe;
    [[nodiscard]] auto default_text_area_recipe() -> TextAreaRecipe;
    [[nodiscard]] auto default_switch_recipe() -> SwitchRecipe;
    [[nodiscard]] auto default_badge_recipe() -> BadgeRecipe;
    [[nodiscard]] auto default_card_recipe() -> CardRecipe;
    [[nodiscard]] auto default_progress_bar_recipe() -> ProgressBarRecipe;
    [[nodiscard]] auto default_spinner_recipe() -> SpinnerRecipe;
    [[nodiscard]] auto default_skeleton_recipe() -> SkeletonRecipe;
    [[nodiscard]] auto default_empty_state_recipe() -> EmptyStateRecipe;
    [[nodiscard]] auto default_alert_recipe() -> AlertRecipe;
    [[nodiscard]] auto default_radio_button_recipe() -> RadioButtonRecipe;
    [[nodiscard]] auto default_toggle_recipe() -> ToggleRecipe;
    [[nodiscard]] auto default_toggle_group_recipe() -> ToggleGroupRecipe;
    [[nodiscard]] auto default_button_group_recipe() -> ButtonGroupRecipe;
    [[nodiscard]] auto default_breadcrumb_recipe() -> BreadcrumbRecipe;
    [[nodiscard]] auto default_pagination_recipe() -> PaginationRecipe;
    [[nodiscard]] auto default_tabs_recipe() -> TabsRecipe;
    [[nodiscard]] auto default_tooltip_recipe() -> TooltipRecipe;
    [[nodiscard]] auto default_select_recipe() -> SelectRecipe;
    [[nodiscard]] auto default_divider_recipe() -> DividerRecipe;
    [[nodiscard]] auto default_avatar_recipe() -> AvatarRecipe;
    [[nodiscard]] auto default_chip_recipe() -> ChipRecipe;
    [[nodiscard]] auto default_dialog_recipe() -> DialogRecipe;
    [[nodiscard]] auto default_popover_recipe() -> PopoverRecipe;
    [[nodiscard]] auto default_dropdown_menu_recipe() -> DropdownMenuRecipe;
    [[nodiscard]] auto default_combobox_recipe() -> ComboboxRecipe;

    /**
     * 框架默认设计系统。品牌主题从本函数的拷贝开始修改字段，再通过
     * ThemeManager::apply() 原子提交。
     */
    [[nodiscard]] auto default_design_system() -> DesignSystem;

    /**
     * 从遗留 NanTheme（tokens + 单调色板）构建 DesignSystem 快照，配方沿用框架默认。
     * 供 set_theme(NanTheme) 兼容路径与 widget 回退快照使用。
     */
    [[nodiscard]] auto design_system_from_theme(const NanTheme& theme) -> DesignSystem;

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_DESIGN_SYSTEM_HPP
