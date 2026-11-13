# 双平台验证：Windows + Linux 协作流程

> **状态（2026-11-10）**：双平台链路已打通并完成一次全量复验。Windows（本机，175% + 125%
> 双屏）负责交互式视觉、混合 DPI、DXIL；Linux（GitHub 仓库的正式支持平台）提供 GCC 16 /
> Clang 23 构建、Vulkan SDF、Linux-only 测试与 Linux 页面 golden。两边共享同一份
> `main`，视觉基线按 `platform` 隔离、各自审批。远程凭据不进入仓库。

## 1. 平台分工

| 项 | Windows | Linux |
| --- | --- | --- |
| 定位 | 实验性平台，交互/视觉验收 | 正式支持平台（X11/Wayland） |
| 构建矩阵 | MSYS2 UCRT64 GCC 16，vendored SDL3 | Arch：GCC 16 与 Clang 23，vendored SDL3 |
| 显示会话 | 2880×1800@175% + 2560×1440@125%，Radeon 660M/D3D12 | GNOME 46 Wayland + RADV RX 6750 GRE 12GB，Mesa 26.2.6 |
| 专属验收 | 混合 DPI 跨屏、IME/候选窗、DXIL 设备路径、Windows 页面 golden | Linux-only 测试（`sdk-vendored-consumer` 等）、Vulkan SDF、Linux 页面 golden、X11/Wayland 窗口 |
| 门禁 | 全量套件 + `release_check --ci` | 全量套件（含 Linux-only）+ 页面 golden compare |

## 2. 远程 Linux 操作（当前通道）

Linux 机器与 Windows **可能不在同一网段**，通过 SSH 以普通用户操作仓库。当前入口由维护者
临时提供（`server.cloudvl.cn:15922`，NAT/端口转发后的地址，可能随网络环境变化）；本机使用
`~/.ssh/nandina_linux` 专用密钥，2026-11-12 在新入口仍可直接密钥登录。地址、端口与密码只保存
在本机会话中，不写入仓库、日志或文档正文。约定：

- 远端默认 shell 为 **fish**；自动化脚本统一用 `ssh ... /usr/bin/bash -lc '...'` 或
  `ssh ... /usr/bin/bash -s < script.sh` 执行，不要假设 POSIX shell 语法；
- 只在 `/workspace/Cpp/NandinaUI` 与各 `buildDir*` 上工作；不改系统配置、不执行 `sudo`；
- 使用**专用密钥**（不入库、不贴日志）；密码仅用于首次安装公钥，之后全部密钥认证，建议定期
  轮换原密码；
- 需要显示/GPU 的验证必须注入图形会话环境（登录会话为 GNOME Wayland 时）：

```bash
export XDG_RUNTIME_DIR=/run/user/$(id -u)
export WAYLAND_DISPLAY=wayland-0        # 从 $XDG_RUNTIME_DIR 下实际的 wayland-* socket 取
export DISPLAY=:1                        # 仅 XWayland 客户端需要
export XDG_SESSION_TYPE=wayland
```

- `page_probe` 创建**隐藏窗口**再做离屏读回，因此不会在用户桌面弹窗；GNOME 会话活动时可直接
  采集。Windows 侧同理使用桌面会话采集 SDF golden。
- 用户负责最终视觉观察与 golden 审批；代理负责构建、采集、差异分析、审批工具写入与提交。

## 3. 一次完整验证的命令序列

### 3.1 Linux（代理经 SSH 执行；用户本地等价）

```bash
cd /workspace/Cpp/NandinaUI
git pull --ff-only
meson setup --reconfigure buildDir
meson compile -C buildDir
meson test -C buildDir --print-errorlogs        # 全量：unit + integration（129 项）
```

2026-11-10 复验：**129 OK / 0 FAIL**（Windows 为 125 OK，差异来自 Linux-only 用例）。
2026-11-12 经新 SSH 入口（端口转发）复验：同一台 `cvrain-workspace`（Arch、RX 6750 GRE、
`/workspace/Cpp/NandinaUI`），**129 OK / 0 FAIL**，8 条页面 golden 全部 0 像素差异。

### 3.2 Linux 页面 golden 采集与比较

```bash
meson setup buildDir-probe -Dsdl_page_probe=true --reconfigure
meson compile -C buildDir-probe
# approved 基线比较（当前 8 条：home/anchors/label/typography × sdf/cpu）
python3 tools/compare_backend_golden.py --page home --backend sdf --json /tmp/g.json
# 视觉变更后：先采集候选，再做差异图，人工审阅后 approve
python3 tools/capture_backend_golden.py --pages home,label,anchors --modes sdf,cpu
```

2026-11-11 中文版重批：8 条 Linux approved 基线全部 **0 像素差异**（含 typography 新页；
隐藏窗口 + RADV，与 2026-10-20 首批记录一致，说明 GNOME Wayland 会话不影响离屏读回结果）。
2026-11-12 经新 SSH 入口再次复验仍为 0 差异。

### 3.3 Windows（本机）

```powershell
meson setup --reconfigure buildDir
meson compile -C buildDir
meson test -C buildDir --print-errorlogs        # 125 项
python tools/release_check.py --ci --build-dir buildDir
```

## 4. Golden 变更流程（视觉改动时）

1. 在功能分支完成代码与单测；CI 覆盖 Linux 构建/单测/集成；
2. Windows 与 Linux 分别采集候选（平台各自 `platform` 键，禁止跨平台复制）；
3. 生成差异图/联系表，维护者人工审阅并给出结论；
4. `approve_backend_golden.py` 写入 `reviewed_by` / `reviewed_on`（必须带真实审阅信息）；
5. 两边 `compare_backend_golden.py` 全绿后合并；基线图与 manifest 同批提交。

## 5. 已暴露并修复的跨平台问题（样例）

- `sdl_platform_window.cpp` 的 `logical_size_ = {};`：GCC 16 接受，Clang 23 拒绝
  （`NanSize` 无初始化列表赋值）→ 改为 `foundation::NanSize::zero()`（`d782729`）。
  说明：**代理侧 Windows 构建通过不代表 Clang/Linux 通过**，Linux 全量套件是必需门禁。
- 双平台单元测试数量不同属预期（平台专属测试/跳过）；报告时必须写清平台与数量。

## 6. 长期机制（待逐步落地）

- CI 已覆盖 Linux GCC 16 / Clang 21 构建、全量测试、ASan/UBSan、physics、SDL system 路径、
  格式与资源工具；Windows 以本机桌面会话为门禁。
- 计划补一个**平台验证入口**（`tools/platform_verify.sh` / `.ps1`）：构建 → 单测 → 设备测试 →
  golden compare → 性能采样，统一输出平台/渲染器/合成器元数据，两个系统跑同一入口。
- **CPU 路径 golden 已进 CI（2026-11-12 实现，非阻断起步）**：新增
  `.github/workflows/visual-golden.yml` + `tools/visual_golden_ci.py`，在 ubuntu 容器里
  `SDL_VIDEODRIVER=dummy` + `--renderer sdl2d` 离屏采集 home/anchors/label/typography CPU 页，
  先断言两次采集逐字节一致，再与当前平台 approved CPU 基线比较；artifact 上传采集/差异图/
  统计。首次真实 Actions 跑绿后删除 workflow 里的 `continue-on-error` 即升级为阻断门禁；
  GPU/SDF 基线仍由真实 Linux 机器采集，CJK 回退字体以固定 sha256 钉住。
- 每批视觉验收在对应 reference 文档记录平台、驱动、合成器与协议，形成可追溯矩阵。

## 7. 安全与维护

- 远程凭据只存在于本机 SSH 配置与 authorized_keys；仓库、日志、文档中不得出现密码或私钥。
- 远程命令只读写仓库/构建目录；如需安装依赖或改系统配置，先向维护者确认。
- 每次远程操作结束前确认 `git status` 干净；代理不得在 Linux 上直接 commit（避免身份/网络
  分叉），提交统一在 Windows 侧完成后再 `git pull`。
