# showcase

NandinaUI 的组件 / 能力演示子项目。两个用途：

1. **展示库已落地的能力**（类似组件库 gallery）。
2. **开发时实际跑一下运行效果**，直观感受实现后的行为。

## 运行

```sh
zig build showcase                 # 运行全部 demo
zig build showcase -- list         # 列出全部 demo
zig build showcase -- <name>       # 只运行指定 demo，如 reactive-counter
```

## 结构

```
showcase/
├── main.zig            # CLI 运行器：解析参数、调度 demo
├── registry.zig        # demo 注册表 + DemoContext 抽象
└── demos/              # 每个 demo 一个文件，自描述并注册到 registry
    ├── foundation_geometry.zig
    ├── foundation_color.zig
    ├── reactive_counter.zig
    ├── reactive_derived.zig
    ├── reactive_batch.zig
    ├── render_scene.zig
    └── layout_box.zig
```

## 新增一个 demo

1. 在 `demos/` 下新建文件，实现 `fn run(ctx: *registry.DemoContext) anyerror!void`
   并导出一个 `pub const demo = registry.Demo{ ... }`。
2. 在 `registry.zig` 里 `@import` 该文件并加入 `demos` 数组。

`DemoContext` 提供 `allocator`、文本输出 `out` 和共享的 `reactive.Graph`。

## 现状与演进

当前 render / runtime / widgets 尚未落地，demo 以**文本输出**展示运行效果
（如 reactive 的依赖追踪、batch 合并次数）。

框架已为可视化画廊预留扩展点：等 render / runtime / widgets 落地后，
`DemoContext` 可扩展出 scene / 根节点字段，demo 的 `run` 返回一棵 widget 树，
本运行器即演进为真正的图形化组件画廊（对应 archive 旧 C++ showcase 的侧边栏 +
多页组件演示形态）——而 demo 的注册方式与目录结构保持不变。
