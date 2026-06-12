# Authoring 与挂载

> 状态：方向文档
> 来源：改写自旧 C++ `archive/docs/component-authoring-and-mounting.md`，按 Zig + Flutter 式声明语法重述。

## 目的

明确面向使用者的组件组合、挂载与引用模型，让业务开发者**描述组件树与布局意图**，
而不是手写坐标计算、手工遍历子节点、手工管理所有权。

## 核心目标

### 1. 不让使用者手写坐标
业务层应表达「这是一个 Column」「这里有 gap」「这里 Expanded」「这里 Padding」，
而不是 `x + 16`、`y + header_h + gap`、`for child in children`。几何只属于 layout 层内部。

### 2. 不让使用者显式处理所有权
框架内部可继续用显式所有权（owning 指针 / arena），但不应让业务层高频接触所有权转移。
使用者写的是「组件组合」，而不是「所有权编排」。

### 3. 挂载入口单一明确
根组件挂载应是单一入口能力，统一收口 root 创建、所有权接管、尺寸同步、生命周期接入：

```zig
window.setRoot(
    column(.{
        label("Dashboard"),
        button("Run"),
    }),
);
```

长期理想可进一步收敛为 `app.mount(DashboardPage())`。

### 4. 挂载与后续访问解耦
使用者保留局部组件变量，本质多是为了「后续还要访问它」。框架应提供独立引用机制
（`Ref` / `Handle` / `Key`），而不是依赖变量生命周期间接维持访问能力。

## 三层模型

### 第一层：Authoring API（使用者直接写）
声明式结构表达，接近 Flutter 组合：

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

### 第二层：运行时 Node 树（框架内部）
框架内部维护真实的 Node 树、子节点所有权、runtime/event/layout 集成。这是实现细节，不直接暴露给业务。

### 第三层：引用与交互句柄
挂载后仍需访问的组件通过显式句柄获取：

```zig
var add_button: Ref(Button) = .{};

const header = row(.{
    button("+").bind(&add_button),
});

add_button.get().setText("Save");
```

关键：组件挂载负责构树，`Ref` 负责后续访问，两者不再通过所有权变量耦合。

## 对公共 API 的约束

1. 业务层禁止手写 `cursor_x / cursor_y`。
2. 业务层禁止遍历 child 决定布局。
3. 业务层禁止直接依赖绝对坐标分发布局。
4. 业务层优先用布局原语表达结构。
5. 公共组合 API 不要求业务层显式管理所有权转移。
6. 子组件后续访问通过 `Ref / Handle / Key`，而不是保留所有权变量。

## 非目标（当前阶段）

- 不要求立刻实现完整的 diff / reconcile 框架——当前关键是「不再手写坐标 / 不再手工管所有权 / 不再手工挂 child」。
- 长期若引入 `Element / Spec` 描述层（使用者写描述、框架在 build/mount 时实例化 Node 树），可自然演化出更完整的更新机制，但不要求一次到位。
