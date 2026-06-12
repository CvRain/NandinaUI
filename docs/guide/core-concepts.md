# 核心概念

读完本文，你会建立起使用 NandinaUI 的基本心智模型。NandinaUI 把现代前端范式带到原生 UI：
Flutter 式声明组合、Angular 式 signal 响应式与 page/router、shadcn 式设计体系。

> 当前状态：`foundation` 已可用，其余层为骨架。本文描述目标 API 形态，部分示例为方向预览。

## 1. 分层

NandinaUI 按单向依赖分层，使用时你主要接触最上面几层：

```
foundation → {reactive, render, layout, theme, text} → runtime → widgets → app
```

- 写界面 → `widgets` + `layout`
- 管理状态 → `reactive`
- 组织页面与导航 → `app`（page / router）
- 定制外观 → `theme`

## 2. 响应式：signal / computed / effect

NandinaUI 的响应式对齐 Angular signal 命名：

- **signal**：可读写的状态单元。
- **computed**：从其它 signal 派生的只读值，依赖变化时自动重算。
- **effect**：依赖变化时自动重跑的副作用（例如标记界面需要重绘）。

```zig
const count = signal(i32, 0);
const doubled = computed(i32, struct {
    fn f() i32 { return count.get() * 2; }
}.f);

effect(struct {
    fn f() void {
        std.log.info("count={d}, doubled={d}", .{ count.get(), doubled.get() });
    }
}.f);

count.set(5); // effect 自动重跑，doubled 自动变为 10
```

把多个更新合并为一次刷新用 `batch`：

```zig
batch(struct {
    fn f() void {
        first_name.set("Ada");
        last_name.set("Lovelace");
    }
}.f); // 只触发一次 effect / 重绘
```

详见开发文档 [响应式策略](../development/reactive-strategy.md)。

## 3. 组件组合：声明结构，而非坐标

你描述「组件树长什么样」，布局由框架完成——不需要手写 x/y 坐标：

```zig
const page = column(.{
    label("Overview"),
    row(.{
        button("Run"),
        spacer(),
        label("Ready"),
    }).gap(12),
}).padding(16);
```

- `row` / `column` / `stack`：基础布局容器。
- `.gap()` / `.padding()` / `.width()` / `.height()`：链式调整。
- `spacer()`：弹性占位。

组件分两类：
- **primitives**：底层积木（Surface / Pressable / Text / FocusRing）。
- **controls**：面向你的真实控件（Button / Label / Card / Panel ...），由 primitives 组合而成。

详见 [组件 Primitives](../development/widget-primitives.md)。

## 4. 挂载与引用

根组件通过单一入口挂载；挂载后如需访问某个组件，用 `Ref` 而不是保留局部变量：

```zig
var run_button: Ref(Button) = .{};

window.setRoot(
    column(.{
        button("Run").bind(&run_button),
    }),
);

run_button.get().setText("Running...");
```

详见 [Authoring 与挂载](../development/authoring-and-mounting.md)。

## 5. 页面与路由

多页面应用用 page / router 组织（吸收 Angular 思路）：

- **Page**：页面描述对象，提供 `routeKey()` / `title()` 与 `build()` 工厂。
- **Router**：注册页面、维护当前路由、`navigateTo(key)`。
- **PageHost**：承载当前页面内容区，导航时重建页面。

详见 [Page / Router 合约](../development/page-and-router.md)。

## 6. 主题

外观由 design token 驱动，而不是在组件里写死颜色尺寸。切换主题（如 light/dark）通过统一的 palette 生效。
详见 [主题与设计令牌](theming.md)。
