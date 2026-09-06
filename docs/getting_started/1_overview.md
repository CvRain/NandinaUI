# NandinaUI 项目介绍

**NandinaUI**（[github.com/CvRain/NandinaUI](https://github.com/CvRain/NandinaUI)）是一个用 **C++26** 编写、基于 **Meson** 构建的**原生桌面 UI 框架**，自我定位为「简单、自研、全链路」。它借鉴了 `Angular`、`Slint`、`Qt QML` 等现代声明式 UI 框架的思路，提供从响应式状态、声明式控件、渲染后端到主题、动画、资源打包的完整自研能力；当前处于 `0.1.0-alpha.1` 早期阶段，但分层设计与测试完备度已经相当可观。

## 核心特性

- **声明式 UI DSL** —— 用 `ui.column()`、`ui.center()`、`ui.make<widget::Button>()` 组合出可读的界面树，布局、对齐、间距一链式完成。
- **响应式状态** —— `signal` / `computed` / `effect` / `property` / `batch`，状态变化自动驱动界面更新，告别手动刷新。
- **丰富组件库** —— Button、Label、Checkbox、Slider、Switch、Radio、Select、TextField、Tabs、Card、Badge、Chip、Avatar、Dialog、Tooltip、ProgressBar、Image、List、Grid、ScrollView 等 20+ 控件。
- **动画系统** —— Tween、Spring、关键帧、缓动曲线与动画组，让过渡与动效顺滑自然。
- **现代文本引擎** —— FreeType + HarfBuzz + FriBidi + utf8proc 组成的字形管线，支持多字体、系统字体发现、复杂文字整形与双向文本。
- **主题与设计系统** —— 三层设计令牌（primitive → semantic → component）、明暗外观（Appearance）、内置主题与样式文档。
- **资源系统** —— 资源清单（manifest）+ 内置/目录/内存/SQLite 四类后端，配合 `nanres` 编译器与可移植打包流程。
- **应用运行时** —— 窗口、Router/Page 导航、视口缩放、异步作用域与统一的输入/剪贴板分发。
- **可选 2D 物理** —— 基于 Box2D 3.x 的轻量物理桥（默认关闭）。
- **无障碍语义** —— 控件语义树导出，为可访问性工具铺路。

## 架构分层

模块自底向上单向依赖，共 12 个模块（`foundation / reactive / resource / scene / semantics / render / theme / text / widget / animation / app / physics2d`）：

<html style="margin:0;padding:0;">
<div style="background-color:transparent;box-sizing:border-box;padding:4px 0;font-family:'Roboto','PingFang SC','Segoe UI',Arial,sans-serif;--accent:#A3D5E8;">
  <div style="font-size:15px;font-weight:600;color:#1A1B1C;">NandinaUI 架构分层（示意）</div>
  <div style="font-size:12px;color:#6B7280;margin-top:2px;">基于仓库 meson.build 与目录结构整理 · 自底向上依赖</div>

  <!-- 应用层 -->
  <div style="display:flex;flex-wrap:wrap;gap:8px;margin-top:12px;align-items:stretch;">
    <div style="flex:0 0 148px;min-width:0;background:rgba(163,213,232,0.22);border-radius:12px;padding:10px 12px;box-sizing:border-box;">
      <div style="font-size:14px;font-weight:600;color:#1A1B1C;">应用层</div>
      <div style="font-size:11px;color:#6B7280;">app</div>
      <div style="font-size:12px;color:#374151;margin-top:6px;line-height:1.5;">窗口与生命周期、路由、UI 调度、视口缩放</div>
    </div>
    <div style="flex:1 1 260px;min-width:0;background:#FFFFFF;border:1px solid rgba(0,0,0,0.08);border-radius:12px;padding:10px 12px;box-sizing:border-box;display:flex;flex-wrap:wrap;gap:6px;align-content:flex-start;">
      <span style="font-size:12px;background:rgba(163,213,232,0.18);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">NanApplication</span>
      <span style="font-size:12px;background:rgba(163,213,232,0.18);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">NanWindow</span>
      <span style="font-size:12px;background:rgba(163,213,232,0.18);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">NanRouter</span>
      <span style="font-size:12px;background:rgba(163,213,232,0.18);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">RootView</span>
      <span style="font-size:12px;background:rgba(163,213,232,0.18);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">UiDispatcher</span>
      <span style="font-size:12px;background:rgba(163,213,232,0.18);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">AsyncScope</span>
      <span style="font-size:12px;background:rgba(163,213,232,0.18);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">NanStore / NanPage</span>
    </div>
  </div>
  <div style="text-align:center;font-size:11px;color:#9CA3AF;margin:4px 0;">▾ 依赖</div>

  <!-- 控件层 -->
  <div style="display:flex;flex-wrap:wrap;gap:8px;align-items:stretch;">
    <div style="flex:0 0 148px;min-width:0;background:rgba(155,187,244,0.20);border-radius:12px;padding:10px 12px;box-sizing:border-box;">
      <div style="font-size:14px;font-weight:600;color:#1A1B1C;">控件层</div>
      <div style="font-size:11px;color:#6B7280;">widget + authoring</div>
      <div style="font-size:12px;color:#374151;margin-top:6px;line-height:1.5;">声明式组装、布局、交互与视觉原语</div>
    </div>
    <div style="flex:1 1 260px;min-width:0;background:#FFFFFF;border:1px solid rgba(0,0,0,0.08);border-radius:12px;padding:10px 12px;box-sizing:border-box;display:flex;flex-wrap:wrap;gap:6px;align-content:flex-start;">
      <span style="font-size:12px;background:rgba(155,187,244,0.16);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">Button / Checkbox / Switch</span>
      <span style="font-size:12px;background:rgba(155,187,244,0.16);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">Slider / Radio / Select</span>
      <span style="font-size:12px;background:rgba(155,187,244,0.16);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">TextField / Tabs / Dialog</span>
      <span style="font-size:12px;background:rgba(155,187,244,0.16);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">Card / Badge / Chip / Avatar</span>
      <span style="font-size:12px;background:rgba(155,187,244,0.16);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">Grid / ScrollView / ListView / Image</span>
      <span style="font-size:12px;background:rgba(155,187,244,0.16);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">Pressable / Ripple / FocusRing</span>
    </div>
  </div>
  <div style="text-align:center;font-size:11px;color:#9CA3AF;margin:4px 0;">▾ 依赖</div>

  <!-- 呈现层 -->
  <div style="display:flex;flex-wrap:wrap;gap:8px;align-items:stretch;">
    <div style="flex:0 0 148px;min-width:0;background:rgba(148,216,195,0.20);border-radius:12px;padding:10px 12px;box-sizing:border-box;">
      <div style="font-size:14px;font-weight:600;color:#1A1B1C;">呈现层</div>
      <div style="font-size:11px;color:#6B7280;">render · text · scene · animation · semantics</div>
      <div style="font-size:12px;color:#374151;margin-top:6px;line-height:1.5;">绘制、文本排版、场景树、动效与无障碍</div>
    </div>
    <div style="flex:1 1 260px;min-width:0;background:#FFFFFF;border:1px solid rgba(0,0,0,0.08);border-radius:12px;padding:10px 12px;box-sizing:border-box;display:flex;flex-wrap:wrap;gap:6px;align-content:flex-start;">
      <span style="font-size:12px;background:rgba(148,216,195,0.16);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">RenderDevice(raylib) + SDF</span>
      <span style="font-size:12px;background:rgba(148,216,195,0.16);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">Text: FreeType·HarfBuzz·FriBidi</span>
      <span style="font-size:12px;background:rgba(148,216,195,0.16);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">SceneTree / Node2D / Control</span>
      <span style="font-size:12px;background:rgba(148,216,195,0.16);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">Tween / Spring / Keyframes</span>
      <span style="font-size:12px;background:rgba(148,216,195,0.16);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">Semantics(无障碍)</span>
    </div>
  </div>
  <div style="text-align:center;font-size:11px;color:#9CA3AF;margin:4px 0;">▾ 依赖</div>

  <!-- 主题层 -->
  <div style="display:flex;flex-wrap:wrap;gap:8px;align-items:stretch;">
    <div style="flex:0 0 148px;min-width:0;background:rgba(225,185,143,0.18);border-radius:12px;padding:10px 12px;box-sizing:border-box;">
      <div style="font-size:14px;font-weight:600;color:#1A1B1C;">主题层</div>
      <div style="font-size:11px;color:#6B7280;">theme</div>
      <div style="font-size:12px;color:#374151;margin-top:6px;line-height:1.5;">设计令牌、内置主题、样式文档</div>
    </div>
    <div style="flex:1 1 260px;min-width:0;background:#FFFFFF;border:1px solid rgba(0,0,0,0.08);border-radius:12px;padding:10px 12px;box-sizing:border-box;display:flex;flex-wrap:wrap;gap:6px;align-content:flex-start;">
      <span style="font-size:12px;background:rgba(225,185,143,0.14);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">DesignTokens / DesignSystem</span>
      <span style="font-size:12px;background:rgba(225,185,143,0.14);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">ThemeManager / BuiltinThemes</span>
      <span style="font-size:12px;background:rgba(225,185,143,0.14);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">StyleDocument(TOML)</span>
      <span style="font-size:12px;background:rgba(225,185,143,0.14);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">VisualState / Motion</span>
    </div>
  </div>
  <div style="text-align:center;font-size:11px;color:#9CA3AF;margin:4px 0;">▾ 依赖</div>

  <!-- 基础层 -->
  <div style="display:flex;flex-wrap:wrap;gap:8px;align-items:stretch;">
    <div style="flex:0 0 148px;min-width:0;background:rgba(158,172,234,0.20);border-radius:12px;padding:10px 12px;box-sizing:border-box;">
      <div style="font-size:14px;font-weight:600;color:#1A1B1C;">基础层</div>
      <div style="font-size:11px;color:#6B7280;">foundation · reactive · resource · physics2d</div>
      <div style="font-size:12px;color:#374151;margin-top:6px;line-height:1.5;">响应式系统、资源管理、基础设施</div>
    </div>
    <div style="flex:1 1 260px;min-width:0;background:#FFFFFF;border:1px solid rgba(0,0,0,0.08);border-radius:12px;padding:10px 12px;box-sizing:border-box;display:flex;flex-wrap:wrap;gap:6px;align-content:flex-start;">
      <span style="font-size:12px;background:rgba(158,172,234,0.16);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">Signal / Computed / Effect</span>
      <span style="font-size:12px;background:rgba(158,172,234,0.16);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">ReactiveScope / Batch</span>
      <span style="font-size:12px;background:rgba(158,172,234,0.16);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">ResourceManager + 4 后端</span>
      <span style="font-size:12px;background:rgba(158,172,234,0.16);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">Color / Geometry / JSON / UTF-8</span>
      <span style="font-size:12px;background:rgba(158,172,234,0.16);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">PhysicsWorld2D(Box2D, 可选)</span>
    </div>
  </div>

  <!-- 依赖与工具 -->
  <div style="display:flex;flex-wrap:wrap;gap:8px;margin-top:12px;align-items:stretch;">
    <div style="flex:0 0 148px;min-width:0;background:rgba(107,114,128,0.08);border-radius:12px;padding:10px 12px;box-sizing:border-box;">
      <div style="font-size:14px;font-weight:600;color:#1A1B1C;">依赖与工具</div>
      <div style="font-size:11px;color:#6B7280;">third-party + tools</div>
    </div>
    <div style="flex:1 1 260px;min-width:0;background:#FFFFFF;border:1px solid rgba(0,0,0,0.08);border-radius:12px;padding:10px 12px;box-sizing:border-box;display:flex;flex-wrap:wrap;gap:6px;align-content:flex-start;">
      <span style="font-size:12px;background:rgba(107,114,128,0.08);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">raylib · spdlog · SQLite3</span>
      <span style="font-size:12px;background:rgba(107,114,128,0.08);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">toml++ · nlohmann/json · OpenSSL</span>
      <span style="font-size:12px;background:rgba(107,114,128,0.08);color:#1A1B1C;border-radius:8px;padding:3px 8px;white-space:nowrap;">nanres 打包器 · 项目 CLI · Catch2 测试</span>
    </div>
  </div>
</div>
</html>


## 外部依赖一览

| 类别 | 依赖 |
| --- | --- |
| 渲染 | raylib（GPU 后端，支持 JPG 等格式） |
| 文本 | FreeType、HarfBuzz、FriBidi、utf8proc |
| 数据 / 配置 | nlohmann/json、toml++、SQLite3 |
| 其他 | spdlog（日志）、OpenSSL（资源签名）、Box2D（可选物理）、Catch2（测试） |
