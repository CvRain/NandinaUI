# 快速开始

本文帮助你在本地构建并运行 NandinaUI。

## 环境要求

- **Zig 0.16.0**（项目当前固定版本）。
- 支持的桌面平台（Linux / macOS / Windows）。

确认 Zig 版本：

```sh
zig version
# 期望输出：0.16.0
```

## 获取代码

```sh
git clone https://github.com/CvRain/NandinaUI.git
cd NandinaUI
```

## 构建与运行

```sh
zig build run      # 构建并运行 showcase 烟雾程序
zig build test     # 运行全部分层单元测试
zig build          # 仅构建，产物在 zig-out/
```

`zig build run` 当前会打印库版本、已就位的分层模块，以及一个 foundation 几何能力的演示：

```
NandinaUI v0.0.0
layers: foundation, reactive, render, layout, theme, text, runtime, widgets, app
demo rect center = (50, 40)
```

## 在你自己的项目中使用

> 注意：NandinaUI 仍处于早期阶段，公共 API 尚未稳定。下面是依赖接入的形态预览。

在你的 `build.zig.zon` 中添加依赖，然后在 `build.zig` 里把 `NandinaUI` 模块接入你的可执行文件：

```zig
const nandina = b.dependency("NandinaUI", .{
    .target = target,
    .optimize = optimize,
});
exe.root_module.addImport("NandinaUI", nandina.module("NandinaUI"));
```

源码中即可导入：

```zig
const nandina = @import("NandinaUI");

const rect = nandina.foundation.Rect.fromXywh(0, 0, 100, 80);
const center = rect.center();
```

## 下一步

- 了解整体心智模型：[核心概念](core-concepts.md)
- 定制外观：[主题与设计令牌](theming.md)
- 想参与内核开发：[开发文档](../development/architecture.md)
