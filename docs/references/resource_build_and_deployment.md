# D5：资源构建、部署与运行时定位

> **状态：D5 范围与目标契约已冻结；P2.1a/P2.1b/P2.2a–P2.2d/P2.3a–P2.4c 已实施。** 2026-10-11 定案。
> 本文汇总 nanres 模块边界、资源自动构建、SQLite 可选依赖以及跨部署布局的定位设计。
> “目标契约”是实施依据，不表示当前代码已经满足；具体类型名、构建选项名与算法按批次定形。
> 示例为伪代码，不是现有公开接口，也不表示项目已经支持 CMake、Windows/macOS 安装或 Web 资源加载。
> 实施遵守 [开发流程](development_loop.md)；现有支持范围见
> [构建系统与 Modules 范围](build_system_scope.md)。

定案摘要：解耦、运行时作用域、身份生成分批交付；解耦先行，身份设计可并行，新的声明式构建
在相应前置完成后汇合。资源语义统一由
`resources.toml` 声明。默认保持工具开/运行时 SQLite 开/vendored，关闭组合必须进入 CI。
作者持久引用倾向 ResourceKey，但不直接把现有 ResourceId 改成纯内容摘要；重复内容、包内记录
与缓存身份的区别见 §6。作用域在管理器查询边界补齐，后端提供与结果一致的发布代次；八个
查询入口、弱域令牌、歧义和流生命周期见 §6.2。§13 只列分批实施定形项，不重新开放方向选择。
本轮只修改文档；被排除的方案与证据纠正见 [story 013](../../stories/013-resource-scope-before-identity.md)。

## 1. 目标：开发者声明资源，然后正常构建运行

nanres 是服务应用的资源构建工具，不是 UI 组件。它可以独立构建、测试和调用，但正常作者流程
不应要求先编完整框架、找到 nanres，再手动 scan/pack，最后重新编译应用。

目标流程是：

1. 创建应用并引入 Nandina。
2. 声明资源文件或目录、逻辑身份与部署布局。
3. 正常配置、构建、运行或安装应用。

构建系统负责在正确时机取得并运行工具，运行时负责消费已定位的资源。这里的
“configure → build → install”描述阶段，不要求采用某一种命令，也不新增 Autotools 支持承诺。
借鉴 Qt 和 Web 工具链的声明式依赖组织，不照搬它们全部代码生成、文件系统或加载假设。

**D5 的核心不是将 nanres 从 tools 移到另一个目录，而是让资源成为应用构建图与部署流程中的正式产物。**

### 已接受的范围

- 资源核心、可选运行时后端、资源构建核心、CLI/构建适配分开；不要求立即拆成独立发布项目。
- 应用目标与其资源建立真实依赖，不仅把资源设为默认构建目标。
- 资源声明、包格式、部署布局分开；安装规则与运行时定位来自同一份部署声明。
- 不使用 nanres 的应用仍可使用 ResourceManager 和自定义后端。
- SQLite 工具侧与运行时侧用途分别控制，源码来源策略另行选择。
- 稳定身份记录与可删除的构建缓存分开，普通构建不偷偷修改版本控制中的身份数据。
- 原生布局先收口，为 D4 的跨平台/Web 留出接口，不提前宣称这些平台已经可用。

## 2. 当前实现：已有雏形与明确缺口

| 当前事实 | 证据入口 | 含义 |
| --- | --- | --- |
| `IResourceBackend` 已有按 key/ID 查找与流接口，ResourceManager 可挂载/卸载后端 | [resource_backend.hpp](../../nandina/resource/resource_backend.hpp)、[resource_manager.hpp](../../nandina/resource/resource_manager.hpp) | 自定义资源系统不是从零加接口；优先复用现有扩展点 |
| 内存、内置、目录和 SQLite 后端已经存在 | [resource/backends](../../nandina/resource/backends) | 不用 nanres 不等于不能使用框架资源协议 |
| SQLite 打包器写包，SQLiteBackend 运行时读包 | [resource_packager.cpp](../../tools/resource_packager.cpp)、[sqlite_backend.cpp](../../nandina/resource/backends/sqlite_backend.cpp) | 关闭 CLI 不足以去掉 SQLite |
| SQLite 被无条件构建并加入库依赖；nanres 链接完整 `nandina_dep` | [meson.build](../../meson.build)、[nandina/meson.build](../../nandina/meson.build) | 打包工具和可选后端还没有独立构建边界 |
| manifest/packager 的公开声明在 resource，部分实现位于 tools | [resource_manifest.hpp](../../nandina/resource/resource_manifest.hpp)、[resource_manifest.cpp](../../tools/resource_manifest.cpp) | 声明、实现归属与构建目标需要一起梳理，不只是移动 main |
| 模板与项目脚手架已通过 custom target 调用 nanres | [nandina_resources.build](../../meson/nandina_resources.build)；脚手架模板由独立仓库 NandinaCLI 消费 | 并非所有路径都必须手工打包；应完善并统一现有自动化 |
| 脚手架资源目标是 `build_by_default`，应用 executable 未显式依赖它 | 脚手架模板（NandinaCLI 消费），主仓库只保留 `nandina_resources.build` | 定向构建应用不能仅凭默认目标保证资源就绪 |
| 模板设置 `build_always_stale: true` | [nandina_resources.build](../../meson/nandina_resources.build) | 每次运行 helper 是当前发现变化的方式，不是精确增量依赖模型 |
| helper 在 validate 失败后 scan，并生成开发定位元数据 | [nanres_build_helper.py](../../tools/nanres_build_helper.py) | 缺少过期与真正错误的结构化区分，自动行为还会影响 lock |
| scan 写策略目录下的 lock，新身份由 `package_id + key` 派生（旧 lock 条目暂留随机 ID，P2.3b 迁移） | [nanres_main.cpp](../../tools/nanres_main.cpp)、[resource_manifest.cpp](../../tools/resource_manifest.cpp) | 不能不分析身份语义就将 lock 移进可随时删除的 build 目录 |
| 开发元数据包含绝对 package_root，portable stage 只复制包与外置资源 | [build_location.hpp](../../nandina/resource/build_location.hpp)、[portable_stage.py](../../tools/portable_stage.py) | 开发定位与可重定位部署已有区分，可继续复用 |
| 应用配置构造路径会自动寻找并打开 SQLite 包 | [nan_application.cpp](../../nandina/app/nan_application.cpp) | 可选后端需要同时调整启动策略，不能只删库依赖 |
| 当前 locator 校验 HOME/XDG 并列举可执行文件旁、用户和系统目录 | [platform_resource_locator.cpp](../../nandina/resource/platform_resource_locator.cpp) | 这是现有 Linux 风格策略，不是通用跨平台安装协议 |
| 默认进程路径发现仅实现 Linux，其他平台抛出不支持 | [application_config.cpp](../../nandina/app/application_config.cpp) | Windows/macOS 的原生路径与 bundle 支持仍是待实现能力 |

现有 [构建工作流测试](../../tests/nanres_build_workflow_test.py) 已覆盖 helper 自动更新部分输入，
不能据此宣布定向构建、安装重定位、可选依赖或跨平台工作流已经完成。
SQLite 已是共享的独立 static_library：当前同一构建配置只编译一次 amalgamation，不是每个
消费 target 各编一遍。只有所有本配置中的 SQLite 消费者均关闭，才能完全省去该翻译单元；
只关运行时后端但保留 SQLite 打包工具，仍需工具侧引擎。

### 格式与 vendoring 背景

本轮讨论发生在统一代码排版、固定 clang-format 23.1.1 和 SQLite vendoring 之后。
格式版本拒绝与独立门禁是现有工程约束，D5 不重新排版或改动 vendored 源码。
目录级 `DisableFormat` 保护使用仓库配置的格式化操作，不等于无法被显式 style 覆盖；
若验证第三方原样副本，须靠来源校验或文件摘要，而不是格式门禁。

固定工具的取得方式已经写在 [CI format 任务](../../.github/workflows/ci.yml) 和
[编码规范](coding_conventions.md)：安装 `clang-format==23.1.1`，而不是依赖镜像的默认版本。
本地可在隔离环境中安装该版本，显式指定可执行文件；CI 镜像/缓存也应记录取得来源，不静默
换版。同一版本号的不同发行构建不自动等于同一二进制；版本检测是门禁条件，不是来源摘要证明。
新增 vendor 目录的操作约定见 [third_party/README](../../third_party/README.md)。

SQLite amalgamation 是当前共享源码策略。支持未来 CMake 不意味着 wrap 本身错误，
也不意味着必须永远禁止系统库；真正需要统一的是来源策略、版本/能力要求与编译行为。
现有 [SQLite 说明](../../third_party/sqlite3/README.md) 中的 vendored-only 规则描述当前实现；
若后续批准 system 选项，应同步更新，不由本文提前改变构建行为。

## 3. 四个职责与依赖方向

| 职责 | 内容 | 不应承担 |
| --- | --- | --- |
| 资源核心 | key/ID、句柄、流、错误、后端协议、挂载和查找 | SQLite 引擎、安装器、UI 和打包流程 |
| 运行时后端 | 内存、内置、目录、SQLite、自定义来源 | 扫描作者目录和生成包 |
| 资源构建核心 | 策略、扫描、稳定身份处理、摘要、校验、打包 | 窗口、渲染、组件和完整应用运行时 |
| CLI 与构建/部署适配 | 命令、输入输出依赖、工具调度、安装和定位描述生成 | 再写一套扫描或打包算法 |

依赖方向：构建核心与运行时后端可以复用资源核心；CLI 调用构建核心；应用挂载后端。
共享的包格式常量、身份规则不能分别定义在读写两侧，具体归属在拆分时定形。

nanres 不再链接完整 UI 库。仅移动目录而保留完整 `nandina_dep` 不能算完成。
此处“模块”是职责与构建目标，不指 C++ Modules，也不要求立即独立仓库、独立 ABI 或插件系统。

### 解耦的机械门禁

解耦完成必须有 CI 可运行的目标依赖关卡，不只靠源码搜索 include 或文档承诺：

- 检查 nanres 的目标依赖闭包、链接输入和归档/object 来源，拒绝完整 UI 库以及
  render/text/widget/scene/app 等越界产物；允许项按正式模块契约维护。
- `ninja -t commands <nanres-target>` 可作为 Ninja 构建的诊断证据，但不能只 grep 上述目录名：
  单体 `libnandina`、对象库、传递依赖或 response file 都可能藏住实际输入。
  优先结合构建系统目标元数据解析闭包，并解析实际链接输入；检查器不能在查询失败、目标缺失
  或输入清单为空时返回成功。具体实现随目标拆分定形。
- 还需在未提供 UI/窗口依赖的环境中配置并构建纯 nanres 路径；链接干净而配置期仍强制查找
  SDL3/字体等依赖，不能算完整解耦。
- 故障注入至少包含：重新链入完整库、经中间目标引入越界库、元数据查询失败。不能仅因链接器
  没有从静态库抽取 UI 符号，就认为不再依赖该库。

这项检查不等待 ResourceId 新语义；拆分时保持现有格式、身份行为和读写测试。

自定义后端仍通过 `IResourceBackend` 和 `ResourceManager::mount()` 接入；新增工作是启动时的
定位/挂载策略注入、缺失后端诊断，以及保证不用 SQLite 时相关路径不会被自动调用。
具体注入时机需保证应用第一次字体、图片等资源消费之前，必需后端已就绪。

## 4. SQLite：用途、来源和成本分别决策

### 4.1 三个独立维度

| 配置维度 | 控制内容 | 建议方向 |
| --- | --- | --- |
| 构建 nanres | 是否产生本机扫描/校验/打包工具 | 可关闭；不影响资源核心 |
| 运行时 SQLite 后端 | 应用是否读取 SQLite 资源包 | 可关闭；仍能用其他后端 |
| SQLite 来源 | vendored / system，必要时再增加 auto | 当前保持 vendored；新增路径必须先有验证 |

首批默认保持现状：**工具开、运行时 SQLite 后端开、vendored**。选项首先提供减法能力，
零配置在现有支持环境和前置依赖满足时继续可用；不承诺未适配平台也可零配置运行。
具体选项名及 feature/boolean 形状待定。不同组合的含义须明确：

| 工具 | 运行时 SQLite 后端 | 预期场景 |
| --- | --- | --- |
| 开 | 开 | 本地生成并读取 SQLite 包 |
| 关 | 开 | 读取预制包或由外部原生 nanres 生成的包 |
| 开 | 关 | 单独使用打包工具；应用使用其他后端。工具写 SQLite 时仍需工具侧 SQLite |
| 关 | 关 | 目录/内存/内置/自定义后端，无其他消费者时不构建、不链接 SQLite |

工具关闭却声明需要自动打包的应用目标时，必须取得显式外部工具，否则配置阶段报错；
不能生成一个必定在构建中失败的空命令。声明 SQLite 包但禁用其运行时后端也不能静默忽略。
关闭工具应停止查找仅由打包器需要的依赖（例如当前摘要实现使用的 OpenSSL），不是只取消安装 CLI。
测试、示例及启动入口须随能力选择构建；不能通过跳过所有资源测试伪造“禁用配置通过”。

**CI 必须实际配置、编译并运行关闭组合**：工具关 + 运行时 SQLite 后端关，使用正式
ResourceManager 完成目录/内存资源读取，并检查依赖图不包含 SQLite 与打包专用依赖。
默认组合保留现有读写测试；每个正式公开的组合都应有对应构建和消费者验证，可利用已有任务
复用环境，不要求为每种编译器做全笛卡尔积。未验证组合不能因为选项能配置就宣称支持。

### 4.2 来源策略

- `vendored`：使用仓库内固定源码和编译选项，不需要额外下载。
- `system`（未来可选）：要求版本及使用到的功能满足契约，否则诊断失败；不暗中回退 vendored。
- `auto`：只在明确需要且有测试时引入，报告最终选择，不作为默认增加的未经验证分支。

源码模式、系统模式的线程能力、扩展加载限制和必要 API 要核对；版本号不是全部能力证明。
系统库选择时也应避免意外混用 vendored 头文件与系统二进制。工具侧与目标应用侧在交叉编译时
是两套机器环境，不应错误复用目标架构库来链接本机工具。

system 的正当收益是复用发行版库、依赖去重、跟随发行版维护的安全更新与打包政策；不是
“解决禁网”，因为当前 vendored 已支持离线。是否实际受益还取决于发行方式及静态/动态链接。
代价是能力验证和两条依赖解析路径，进入正式支持时必须增加 system 配置测试；这里不是
再维护一套资源读写业务实现。

### 4.3 是否替换 SQLite

暂不以“更轻”为由立即更换包格式。先测冷构建时间、最终产物大小、读取行为和启动成本，
再评估替代方案。换格式需要重新承担索引、流读取、外置资源、格式版本、损坏诊断和一致性。
首先解除可选功能的依赖耦合，未来才有低成本替换后端的空间。

## 5. 正式作者入口与构建时机

### 5.1 声明一次，自动构建

拟议入口的语义如下，不承诺对应语法已经存在：

```text
为 my_app 声明资源：
  包身份：org.example.my_app
  目录：assets/
  单文件：styles.toml
  包格式：sqlite
  部署布局：portable
```

**资源语义的唯一作者声明文件是 `resources.toml`。** Meson/CMake 只引用它并建立目标关系，
不再提供另一个承载资源列表、key 映射、排除、别名或存储规则的 DSL。
简便命令限定为生成/编辑该文件，且是作者明确执行的操作；普通构建只读它。
多个前端并非必然产生多个事实来源，但本项目此阶段主动收敛为一个可跨构建系统复用的文件。
资源命名空间如何与现有 ResourceKey/包身份衔接仍需明确，不把上例当作已支持包限定查找。

构建文件仍可指定应用 target、清单位置、构建输出根和 prefix/DESTDIR 等构建部署参数；
这些是适配输入，不是第二份资源语义。共同的部署布局解析规则由共享层定义，仍遵守 §7 的
安装/定位同源契约。现有 `package_directory` 的退出或默认值角色需明确，不留两个竞争的输出事实来源。

### 5.2 三阶段职责

```text
配置：声明应用、输入、输出、工具来源与部署布局
构建：构建本机 nanres → 生成资源产物 → 应用运行所需产物就绪
安装：按同一部署布局安置应用、包及外置资源
```

- 配置时不要求 nanres 已经存在，也不应先执行尚未构建的工具。
- 从源码引入框架时，将本机工具加入构建图；SDK 消费场景可提供兼容的预构建工具。
- 只构建应用目标也必须使其运行所需资源就绪，不能仅依靠 `build_by_default`。
- 普通外置包可与 C++ 编译并行；只有生成头文件/源文件等真实编译输入才建立编译前依赖。
- 安装入口须构建缺失产物，或在不负责构建的模式下明确检查并失败，不能复制旧包冒充成功。

CLI 保留为诊断、检查包、受控身份更新、独立生成与 CI 复用的高级入口，不是日常必需步骤。
Meson 与未来 CMake 适配调用同一构建核心，不各写一套业务规则。本阶段先完善 Meson，不借
伪代码提前扩大 [CMake 支持承诺](build_system_scope.md)。

### 5.3 增量、依赖与发布

必须分别处理已有文件修改、目录新增/删除、策略/别名改变、工具或包 schema 改变、外置资源
副产物及过期文件清理。仅把已知文件写进 depfile 并不能发现后来新增的文件，目录成员发现
机制必须单独定义。可先用轻量扫描守住正确性，再优化；不能将“避免每次扫描”置于正确性之前。

无变化时不重写包、定位描述或无故改变时间戳。区分“扫描命令运行了”和“昂贵打包重做了”。
不同构建配置、并行构建与多应用不得同时写同一中间文件或输出目录。

成功产物应完整发布，失败保留上一份有效输出但不得当作本次成功。数据库、外置资源和定位
描述须属于同一包代次，不能仅原子替换数据库而遗漏外置文件的一致性。清理范围限于本目标
拥有的输出，不删除作者目录或无关文件。

## 6. 稳定资源身份与构建缓存：自动化前置，不阻塞解耦

现有 lock 同时包含稳定 ID、资源 key、源文件位置、内容摘要等。helper 会在 validate 失败后
scan；scan 可能写回源码侧 lock。P2.3a 起新 key 的 ID 由 `package_id + key` 版本化派生
（§8.4），不再随机分配；已有 lock 中的旧随机 ID 按同 key 保留，直到 P2.3b 的显式迁移命令
重写。因此“所有自动生成内容搬进 build”不能机械执行：旧 lock 的迁移顺序、重命名后的身份
继承与碰撞策略仍是 P2.3b 的范围。需要先确定消费者依赖 ResourceKey 还是跨构建稳定 ResourceId。

| 数据 | 性质 | 目标职责 |
| --- | --- | --- |
| 作者 key 与显式路径映射 | 可能被持久数据或跨版本引用 | 保存在可审阅的 resources.toml 中，变更按作者接口处理 |
| 内容摘要、扫描与打包缓存 | 可重建优化数据 | 放 build 目录，删除后可正确重建 |
| 包 schema 与兼容信息 | 读写协议 | 由工具与运行时一致校验 |

### 6.1 作者身份使用 key，生成数据不成为持久引用

作者侧契约已接受，映射语法与生成算法在身份批定形：

1. `ResourceKey` 是作者声明的逻辑身份。由路径派生的 key 随重命名改变；显式声明并保持相同
   key 的资源，即使源文件重命名也保持作者引用。跨包引用仍须确定命名空间/包作用域。
2. 源码树只保存人可审阅的资源声明和显式 key→路径映射；摘要、内部 ID 和扫描缓存生成到
   build 目录，不回写源码树。现有 overrides 尚不等于已支持任意 key→路径重映射，入口需补齐。
   P2.3a 已补：`[[resources]]` 可同时声明 `key` 与相对 `path`；显式映射优先于路径派生 key，
   文件不必位于扫描根内；同一路径多个 key、缺失文件、与派生 key 冲突均报错。示例：

   ```toml
   [[resources]]
   key = "app/config"
   path = "assets/config-v2.toml"
   media_type = "application/toml"
   ```
3. 用户配置、存档和业务数据库不持久化生成 ID；使用明确作用域的逻辑 key。修改或删除显式 key
   本身属于作者接口变更，需要明确别名/迁移，而不是靠内部 ID 自动恢复。

该方向不要求另设随机 UUID 的作者登记系统；但不能未经说明改变既有 ID 的公开含义。
当前头文件称其稳定身份、ResourceManager 支持 ID 查找，已有调用点与测试须一起迁移，旧包
若不兼容则通过 schema 诊断，不能静默解释为新语义。解耦阶段先保留这些行为。

### 6.2 已定：保留包记录格式，补运行时作用域

不直接将纯字节摘要替换现有 ResourceId，原因来自现有代码而非兼容偏好：

- [包 schema](../../tools/resource_packager.cpp) 用 ID 作主键，每条记录拥有自己的 key 和
  media_type；[MemoryBackend](../../nandina/resource/backends/memory_backend.cpp) 同样要求 ID 唯一。
  两个不同 key 可以对应完全相同的字节，直接取内容摘要会让它们争用一个记录 ID，即使没有哈希碰撞。
- ID 目前是 16 字节，内容摘要是 SHA-256 字符串；是否截断、如何检测碰撞、包含哪些输入，
  都不能由“内容寻址”四个字隐含决定。
- [FontLoader](../../nandina/text/font_loader.cpp) 使用资源 ID + face index 缓存。只按包内 ID
  定义身份却在跨挂载缓存中全局使用，会产生别名风险；改语义必须审计后端、别名、流与缓存消费者。

本轮采纳“**不为解决缓存串包而拆包格式，先修正运行时作用域**”的收敛方向。
保留每条资源记录自己的 ID/key/media_type，不将 ID 换成纯字节摘要，不将记录索引/blob 拆表
列为本轮方案。区分逻辑 key、内容摘要与记录/缓存身份；将来跨包去重是独立能力，不能倒逼
现在把不同 key 的记录合并。

不过，“包内唯一的 ID”只有在跨包操作都使用正确作用域后才充分，当前句柄还没有这个能力：

- [ResourceHandle](../../nandina/resource/resource.hpp) 只是 `shared_ptr<const Resource>`，
  Resource 中没有 mount 身份或内容代次；`FontLoader::load(ResourceHandle)` 最终仍只取
  `resource->id()` 构造缓存键。调用该重载本身并没有消除串缓存。
- [ResourceManager](../../nandina/resource/resource_manager.cpp) 的裸 ID 查找按挂载优先级
  返回第一个命中。缓存即使改对，查找时也可能已选到另一个包；流查找同样需要审计。
- MountId 在每个 manager 中分别生成，不是跨 manager 的全局身份。同包多次挂载、卸载重挂、
  不同 manager 与同一挂载中资源替换，不能仅凭“包名 + ID”或不带归属的 MountId 判断相等。

#### 6.2.1 管理器包装作用域，覆盖八个公开入口

选择 A：在 **ResourceManager 的公开解析边界**返回带作用域结果。当前
`ResourceLookup` 同时供 `IResourceBackend::find()` 使用，不能直接要求该别名的所有生产者
填 MountId。同一 backend 可挂载多次、被多个 manager 使用；后端只提供本地结果与发布代次，
manager 为命中补上域身份和 MountId。后端协议与管理器结果类型分开，不提供两套竞争的作者 API。
挂载字段不进入包格式。

下表是 P2.2b 已实现的入口语义（§8.14）；`ResolvedResource` / `ResolvedStream` 自该批起是可用 API。
两者都携带作用域、发布代次及记录 ID；资源结果另持有不可变资源，流结果另持有流及其一致性类别。

| 当前入口 | 目标成功结果 | 查询规则 |
| --- | --- | --- |
| `find(key)` | `optional<ResolvedResource>` | key 按挂载优先级覆盖；别名沿同一路径解析 |
| `find(id)` | `optional<ResolvedResource>` | 无命中为空，单一命中成功，多重命中报歧义 |
| `require(key)` | `ResolvedResource` | 与 find(key) 同源，未找到转为错误 |
| `require(id)` | `ResolvedResource` | 与 find(id) 同源，保留歧义错误 |
| `open_stream(key)` | `optional<ResolvedStream>` | 与 key 资源查询使用同一作用域规则 |
| `open_stream(id)` | `optional<ResolvedStream>` | 单一命中成功，多重命中报歧义 |
| `require_stream(key)` | `ResolvedStream` | 与 open_stream(key) 同源，未找到转为错误 |
| `require_stream(id)` | `ResolvedStream` | 与 open_stream(id) 同源，保留歧义错误 |

所有入口保留 expected 风格的错误通道；不能由 require 包装再次剥掉作用域。资源与流的签名
迁移一起完成，不靠隐式转换回裸句柄维持旧缓存写法。编译错误只暴露解构点，正确缓存仍须冲突测试。
纯字节消费者可显式取出 payload；需要缓存身份或重新解析时必须保留完整解析引用。

priority 只裁决 key 查询，不能用于裸 ID 的平手裁决。同一 backend 挂两次也属于两个挂载命中，
不能按指针相同去重成“唯一”。查询错误不能被忽略后宣称唯一命中；诊断应能指出冲突挂载。
单一命中的 ID 查询继续可用，ID 查询没有被整体废弃。已解析引用的后续重新查询/打开只使用
原作用域，不回退到全树 key/ID 搜索；过期必须失败。迁移清单包含八个管理器入口、
`FontLoader::load(ResourceId)` / `invalidate(ResourceId)`，以及字体、图片、流与别名消费者。
直接加载未挂载的 ResourceHandle 不得伪造挂载身份；选择独立拥有内容的身份或绕过共享缓存，
具体入口在作用域批定形，不允许重新按裸 ID 混入挂载缓存。

#### 6.2.2 弱域令牌、挂载身份与缓存回收

采用 manager 内部持有强引用、解析引用/缓存持有弱引用的域令牌。比较的是**控制块的所有权
身份**，不是 `lock().get()` 的地址，也不是把所有 expired 令牌视为同一个空值。实现可用所有权
比较器；若选择哈希容器，必须提供同一语义的哈希与相等判断，不能退回裸地址。
旧弱引用还存在时其控制块身份仍可区分；弱令牌不意味着控制块永久不释放或占用自动回收。

域令牌只由活着的 manager 实现拥有强引用，缓存不保活 manager。现有 manager 可移动，移动
必须转移同一逻辑域，不能因对象地址改变而换域。manager 销毁后，旧引用的重新解析失败；已取得
内容按 §6.2.4 继续使用。MountId 在域内不复用，卸载重挂分配新 ID；溢出须诊断，不回绕复用。

字体缓存身份形状为 `(域所有权身份, MountId, 发布代次, ResourceId, face_index)`（P2.2c 已实施）。
失效使用与 lookup 同源的作用域；广域操作单独命名，并明确只作用于接收该操作的缓存实例。
失效移除关联，不销毁使用者仍持有的字体或资源。

回收分两类：清扫 expired 域令牌可删除已销毁 manager 的缓存关联；**仍存活 manager 的旧挂载、
旧代次不会因此消失**，还需卸载/发布后的定向清理或缓存自身的淘汰机制。P2.2d 定为：TextureCache
在帧边界与保留列表裁剪时清扫 expired 域，FontLoader 在缓存访问时清扫；改变缓存键只保证不错误
复用，不等于自动回收。清理不得要求本已销毁的 manager 回调。

#### 6.2.3 后端发布代次与同快照义务

当前 [MemoryBackend](../../nandina/resource/backends/memory_backend.hpp) 可直接 insert/erase/add_alias，
目录文件也可能在管理器之外改变，因此只加 manager 计数器不能捕捉发布。
可变内容的发布控制点在后端侧，并对管理器可见。后端必须将本地查询结果与它的发布代次作为
一致结果交付；单独增加 `revision()`，由 manager 在 find 前后随便读取一次，不满足契约。
MemoryBackend 可在同一锁内取得资源/别名结果和代次；第三方后端也必须遵守并有契约测试。
快照租约等具体类型在作用域批定形，后端仍不认识 manager 的 MountId。

成功发布影响可观察结果的修改（insert/replace、erase、别名重定向等）时推进代次；失败不发布
半份状态。同一 backend 的多次挂载应观察同一次发布的后端代次，各自保留独立 MountId。
运行期原 ID 可以保持而代次变化；代次不持久化为作者身份，不能回绕后与旧缓存身份相等。

目录文件的任意外部写入不会自动被 manager 计数器捕捉。首版不承诺文件监听或任意外部写入的
实时一致性：可复用缓存的快照来源须通过显式发布/不可变代次约束内容；开放外部修改的文件流
按非快照处理，不允许仅凭代次将它冒充不可变内容缓存。同大小改写也不能靠 declared_size 检出。

并发查询的挂载选择与后端快照须有明确生效点：与卸载/替换重叠的操作可取得被固定的旧结果，
卸载完成后新发起的重新解析不得成功。不能返回旧内容配新代次，也不承诺多个后端同时发布的全局
事务快照。不在 manager 的全局锁内无条件执行任意后端 I/O/回调；具体同步方案随 API 一起验收。

#### 6.2.4 已取得内容、重新解析与流的生命周期

| 对象或操作 | 目标契约 |
| --- | --- |
| 旧作用域引用重新查询/打开 | 域已销毁、挂载已卸载或代次已过期时显式失败，绝不静默指向新内容 |
| 已物化不可变资源及由其加载的字体/图片 | 可继续使用所持原快照；不因卸载而强制失效 |
| 已打开的内存流 | 持有原字节，卸载不打断读取 |
| 已打开的文件流 | 持有所需文件句柄，卸载本身不取消读取；不承诺免受外部改写、截断或 I/O 失败影响 |
| 只保留路径并稍后重新打开的实现 | 每次重新打开是解析操作，须重验作用域；不能宣称保留了原内容快照 |

**现有实现的证据纠正**：[file_resource_stream.hpp](../../nandina/resource/file_resource_stream.hpp)
中的 path/declared_size 是打开参数；[file_resource_stream.cpp](../../nandina/resource/file_resource_stream.cpp)
里的 FileResourceStream 实际持有 `std::ifstream stream_`，read/seek 使用该流，并非反复按路径
打开。它仍是非快照文件来源：保持打开不阻止外部原地修改。当前的尺寸核验也不证明内容一致。

流结果须能明确表达其一致性类别，不能仅根据“来自哪个后端”推断；SQLite 内嵌字节与外置文件
就具有不同边界。允许已打开流继续使用，需要流拥有实际字节/句柄/必要租约；mount 接受
shared_ptr 本身不是保活证据。后续 API 不得把非快照流用于声称同代次内容稳定的缓存。

因此，“包内唯一”不自动推出“ID 随内容变化”：原记录 ID 可以保持，而运行时内容代次改变。
后续将生成 ID 移入 build 时仍需定义生成算法、包内碰撞处理和重建契约；作用域解决跨挂载
别名，不替代这些决定，但无需重新引入 blob 拆表分支。阶段 1 解耦不夹带缓存/API 行为变更。

现有 lock 已有逐资源 SHA-256，但当前包的 resources 表没有逐资源摘要字段，包指纹也不能
代替它。未来如需运行时跨包去重，可复用已有摘要计算，再明确摘要的传递/存储方式与可能的
schema 变更，不在本轮提前加入；内容摘要本身不等于包真实性签名。

### 6.3 作者在 C++ 中如何引用

增加 D5.9，与 D5.1 同阶段定形，但不阻塞物理解耦：

| 入口 | 能保证什么 | 不能保证什么 |
| --- | --- | --- |
| 运行时字符串 key | 动态主题/用户资源可引用，现有路径可继续用 | 拼写或删除资源不自动导致编译失败 |
| 构建期生成常量头 | 从同一 resources.toml 生成具名 key；删除对应声明并重生成后，直接使用该符号的代码可编译失败 | 不验证任意手写字符串，也不证明安装产物存在 |
| 类型化访问器 | 可进一步限制声明的资源类别与访问方式 | 需要独立类型/schema 规则，不能凭文件后缀推定运行时一定解码成功 |

倾向先保留运行时 key，为静态资源提供可选生成常量头，不立即扩大成完整类型化加载框架。
常量内容是逻辑 key，不暴露内部 ID 或安装绝对路径。显式 key 的源文件重命名不应无故改变符号；
符号名规则、冲突诊断、删除后的再生成依赖与生成器可用性须明确，防止旧头文件让测试假绿。
选择生成头时它是真实编译输入，应在消费者编译前就绪；只使用运行时 key 的应用不被迫使用代码生成。
P2.3c 已按该最小形态实现：`nanres keys --header` 生成逻辑 key 常量，符号由 key 稳定派生，
碰撞显式失败，删除声明后重生成会使消费者编译失败；详见 §8.20。

### 6.4 错误与可复现性

validate 应返回可区分的结果。清单过期/缓存缺失可按声明策略更新；策略错误、身份冲突、权限
错误、包损坏不能统一进入“再 scan 一次”。失败诊断应保留原始原因。
可复现性测试应明确比较逻辑资源/身份还是整个包字节；未定义确定性输入与引擎版本前不承诺
跨机器、跨 SQLite 版本的逐字节相等。

## 7. 资源声明、包格式与部署布局分离

| 概念 | 决定什么 | 不决定什么 |
| --- | --- | --- |
| 资源声明 | 输入、逻辑 key、包身份、规则 | 安装器实现 |
| 包格式 | 目录、SQLite、内嵌等交付形式 | 必须安装到哪个目录 |
| 部署布局 | 应用与资源的最终位置及定位方式 | 组件如何改写资源 key |

同一个包可以部署到应用目录或共享数据目录；变更安装位置不改变资源引用。
“绿色版”在本契约中指可整体移动的应用目录，不默认等于单个可执行文件；单文件交付需要
明确选择并实现内嵌应用资源，现有内置字体后端不自动等于任意应用资源内嵌支持。

### 7.1 一份声明生成两个结果

**框架不预测所有安装器；安装规则与运行时定位必须来自同一份部署布局。**

```text
部署布局声明
    ├─→ 安装规则：把产物放到哪里
    └─→ 定位描述：运行时从哪个已知入口找到它
```

示例为伪布局：

```text
executable_dir = bin/
resource_dir   = share/my_app/
location      = executable_relative

安装：<prefix>/bin/my_app
      <prefix>/share/my_app/resources.db
运行：实际可执行文件目录 / ../share/my_app/resources.db
```

相对关系从声明生成，不由运行时代码猜测或硬编码为所有应用通用的 `../share`。
若两个位置无法建立可重定位的关系，必须选择固定路径或显式定位器，不假装任意移动仍可用。

### 7.2 少量明确策略，而非按操作系统猜布局

| 策略 | 定位契约 | 移动边界 |
| --- | --- | --- |
| Portable | 可执行文件相对资源位置 | 整体移动且内部关系保持时可用 |
| Prefix | 声明 bindir/datadir，选择相对关系或固定安装根 | 相对布局可整体移动；固定根不承诺任意移动 |
| Platform bundle | 平台适配取得 bundle 资源根 | 以平台 bundle 规则为准 |
| Custom | 应用/部署适配显式提供位置或直接挂载后端 | 由提供者声明 |

策略不绑定操作系统：Linux 也可 portable，Windows 也可选择安装布局。Windows 的原生路径、
macOS 的 bundle 入口、沙箱等具体适配须分平台验证；不继续在非 Linux 环境强制要求 HOME/XDG。
资源核心不引入操作系统 SDK 类型；平台发现和应用启动适配在相应上层完成。

### 7.3 定位描述的启动入口

生成 JSON 不足以解决问题，还必须知道从哪里读取 JSON。允许的方向：

- 稳定相对关系生成应用使用的定位配置。
- 安装时可调整的布局，在约定的可执行文件相对位置或平台 bundle 元数据入口提供描述。
- Custom 模式由应用显式注入位置/后端，不强制再找一份描述文件。

不能循环依赖“先找到资源目录，才能读取告诉我们资源目录在哪里的文件”。描述与包应声明
预期身份和格式，避免混入另一应用或不兼容包；具体兼容字段在实施前确定。
获取实际可执行文件位置不依赖当前工作目录，符号链接/启动器与 bundle 路径语义须定义并测试。

### 7.4 安装前缀、暂存根与安装时覆盖

```text
最终 prefix：/usr
暂存根：/tmp/package-root
安装写入：/tmp/package-root/usr/share/my_app/
运行定位：最终布局中的资源位置，不含 /tmp/package-root
```

定位描述来自最终布局，DESTDIR 等暂存根只参与安装写入，不写入运行时定位。
若允许安装时覆盖 prefix/bindir/datadir，安装适配必须同步更新受影响描述；编译进去的固定
路径无法同步时就拒绝该覆盖并要求重新配置，不允许安装成功却仍指向旧路径。

源文件、开发 build 路径与发布描述分开。开发期绝对路径可用于明确的 dev 模式，但发布产物
不得携带或优先消费它；删除开发元数据后安装版仍须独立运行。

## 8. 运行时定位、挂载与信任边界

建议先解析选定布局，再打开预期包，最后挂载到 ResourceManager。显式定位来源、生成的布局
与可选平台回退之间的优先级需一次定形，不让每种启动入口自行选择。

| 来源 | 用途 | 默认边界 |
| --- | --- | --- |
| 应用基础资源 | 随应用部署的必需内容 | 布局确定，通常只读 |
| 用户数据与缓存 | 用户生成或运行时可写内容 | 独立可写目录，不往安装资源目录写 |
| 主题/插件/资源覆盖 | 用户显式扩展 | 显式启用，定义冲突、优先级和可覆盖范围 |

当前 locator 列举用户/系统目录的行为不能自动成为通用发布默认。环境变量和用户目录不是
无条件可信的基础包替代来源；应用是否允许覆盖必须是策略而非搜索碰巧命中。

必需包缺失、损坏、身份或 schema 不兼容，应报告所选布局、解析路径、预期身份和具体原因，
不能静默退回另一处同名旧包。可选包缺失可以跳过，但损坏是否阻断启动要独立声明。
包内外置路径、别名和文件访问不能逃出其允许根；已有后端校验在重构时继续保留。
自定义后端也遵守“未找到”与“后端错误”的区分，不把 I/O 错误伪装成资源不存在。

## 9. 构建机器与目标机器分离

nanres 在构建机器执行，应用在目标机器执行。交叉编译时不得将目标架构 nanres 直接当成本机
构建命令。应支持原生工具目标或显式提供的已有工具，并校验其资源格式/能力兼容性。

工具缺失、不匹配和无法执行应可诊断，不自动绕过打包。工具与应用的 SQLite 引擎版本不必
无条件相同，但读写使用的 schema、SQL 能力和格式兼容须通过测试，不仅比较工具版本字符串。
不要求整个 UI 框架先为构建机器编一遍，正是解耦资源构建核心的实际价值。

## 10. D4 / Web 边界

逻辑资源 key 不应因开发目录、安装目录、bundle 或未来 Web URL 改变。部署适配负责把逻辑
引用映射到可消费的来源，但不要求所有平台使用同一种容器格式，更不要求 Web 为了统一而用 SQLite。

现有后端查找是同步接口，网络资源可能尚未就绪。Web 阶段必须选择启动前预加载后挂载，或
明确增加异步加载/就绪协议；不能在同步 find 内隐藏阻塞下载，不能把“尚未下载”当成 not_found。
这属于 D4/D5 的交界，不阻塞原生构建部署收口，也不在本轮承诺实现。

## 11. 已接受的分批计划

D5 范围已冻结。以下为 D5 内部实施顺序；项目总排序仍见 [项目进度](project_status.md)，
不因文档定案自动开始运行时代码改动或调整 D1–D4 的优先级。

1. **解耦先行**：只定该批模块所有权与目标边界，拆资源构建核心、去掉 nanres 对完整 UI 的依赖；
   增加工具/后端开关和机械依赖门禁，默认配置不变，禁用组合进 CI。保持当前 ID/包 schema/
   扫描行为，不等待 D5.1 的新身份决策；也不在这一批承诺所有交叉编译平台已支持。
2. **作用域批**：保持包格式和 ID 生成算法，按 §6.2 迁移管理器八个查询入口、后端发布协议、
   字体等缓存与失效；落实弱域令牌、歧义、快照/流生命周期及回收。允许有意改变跨挂载裸 ID
   歧义行为，不能用解耦批的无冲突等价测试替代这批验收。
3. **身份批**：落实 key 作者契约、内部记录/内容摘要区分、resources.toml 映射、生成 ID 算法
   与 C++ 引用形态。设计可以与解耦并行，改变生成行为的实现独立交付；依赖作用域的包内 ID
   契约须先完成第 2 步。不等待这一批即可交付解耦。
4. **声明式构建（汇合点）**：在解耦与相关身份前置完成后统一模板/脚手架，补应用目标依赖、
   增量/目录变化、输出一致性和本机工具；生成头若启用则作为真实编译输入。
5. **部署与定位**：先实现并验证 portable 与当前支持平台的 prefix 布局；同步安装描述，覆盖
   暂存、重定位、非 cwd 启动与错误诊断，再逐平台增加 bundle/custom 适配。
6. **作者验收与归档**：用独立下游应用验证从干净检出到安装运行，更新教程与示例；按需增加
   system SQLite 路径。CMake 适配和替代包格式依支持计划、实测收益另行安排。

每阶段区分机制单测、真实下游构建、安装运行和平台验证；阶段完成不代表所有部署形式已支持。
这里的“并行”描述设计依赖，不要求一次提交混入两条工作线。首批可独立验收交付，不等待 §13
全部项目拍板；仍需完成该批实际目标拆分与依赖选项的最小 API 定形。

## 12. 验收矩阵与故障注入

**以下是未来验收要求，不是本次已执行结果。** 每条落地后记录改坏位置、失败输出与恢复结果。

| 场景 | 必须证明 | 改坏何处应失败 |
| --- | --- | --- |
| 解耦前后逻辑等价（单挂载 / 无 ID 冲突） | 同一清单、源文件与既有 lock，两输出根下前后产物的 key/media_type/内容、既有 ID、别名、存储语义一致；新增资源 ID 值不参与比较 | 拆分时改了 key 派生、media_type 推断、ID 继承、别名或存储语义 |
| 干净检出构建 | 无手工 scan/pack 或预编 nanres，正常构建可运行 | 配置期执行未生成工具或删除工具依赖 |
| 定向构建应用 | 只选应用目标也得到最新必需资源 | 仅保留 build_by_default，移除应用资源依赖 |
| 新增/修改/删除 | 文件内容与目录成员变化均反映到包，删除旧副产物 | 只登记已存在文件，漏目录成员或清理 |
| 无变化增量 | 不重写包及描述，不改变无关输出时间戳 | 无条件重新发布全部产物 |
| 稳定身份（P2.3a 已实施） | 显式 key 跨源文件重命名保持，派生 key 按规则变化，用户引用不依赖内部 ID | 用源路径重写显式 key，或把持久数据仍指向内容派生 ID |
| 重复内容（身份批） | 相同字节的不同 key 可共存且各有记录身份 | 直接把纯字节摘要作为唯一资源记录主键 |
| 跨挂载 ID 冲突（作用域批） | 带作用域引用各自解析正确；裸 ID 单一命中成功、多重命中报歧义；key 优先级覆盖保持 | ID 查询用 priority 裁决，或缓存跨挂载复用 |
| 八入口与别名（作用域批） | find/require/open_stream/require_stream 的 key/ID 变体均保留作用域，别名无旁路 | require 返回裸句柄、流丢作用域或别名绕过解析 |
| 域身份（作用域批） | 两 manager 均有 MountId=1、同 backend 多次挂载仍不串缓存；manager 移动不换域 | 仅按 MountId/裸地址比较，或 expired 令牌全部相等 |
| 发布与并发（作用域批） | replace/erase/别名修改推进代次；重叠查询得到一致结果；卸载后新查询失败 | 结果与代次分开读取，忽略别名发布或复用旧挂载身份 |
| 生命周期与回收（作用域批） | 旧引用重新解析失败，已取得字节/流遵守各自保证；死域及活域旧代次均有回收路径 | 卸载销毁仍被使用的内容，非快照流冒充快照，或只清 expired 域 |
| 作者引用（如启用生成头） | 从同一清单生成常量，删除符号后消费者编译失败，安装路径不进入头文件 | 保留过期头文件或另维护手写常量清单 |
| 源树只读 | 正常构建不改作者清单或稳定身份文件 | 隐式 scan 写回源树 |
| 精确失败 | 策略/权限/冲突错误不当作缓存过期 | 将所有 validate 错误统一自动修复 |
| 原子产物 | 中途失败不发布新旧混合包且命令失败 | 先替换数据库再失败于外置资源更新 |
| 工具/后端开关 | 默认组合保留；两者关闭组合必须进 CI，编译并运行目录/内存消费者，且无 SQLite 编译/链接或打包专用依赖查找 | 只隐藏 CLI、保留无条件依赖或跳过全部资源测试 |
| 工具依赖闭包 | CI 检查目标/链接/归档输入，纯 nanres 配置不要求 UI 依赖；查询失败与空输入必须红 | 直接或间接链回完整库、漏解析 response file，或忽略工具查询失败 |
| 自定义资源 | 无 SQLite 配置仍可挂载并读取目录/内存/自定义后端 | 启动强制调用 SQLiteBackend |
| 来源策略（启用时） | system 不满足则失败，vendored 离线可用 | system 失败静默回退或混用头文件 |
| 任意 cwd | 从别处启动仍定位同一应用包 | 使用 current_path 代替真实入口 |
| Portable 重定位 | 整个目录移动后运行，源码/build 不可用也成功 | 发布描述残留开发绝对路径 |
| Prefix 与暂存 | 非默认前缀和 DESTDIR 暂存后的最终布局有效 | 将暂存根写入定位描述 |
| 安装时覆盖 | 更新描述或明确拒绝，不保留旧路径 | 只改变安装目的地不改运行规则 |
| 描述启动入口 | 未定位资源前即可找到部署描述 | 把唯一描述放进未知资源目录 |
| 身份与错误隔离 | 错包/旧包/损坏包准确报错，不碰巧找其他副本 | 按文件名猜包或无条件遍历回退 |
| 并行/多配置 | 不同应用与配置不互相覆盖产物 | 共用无区分中间目录或临时文件名 |
| 交叉编译 | 构建机工具执行、目标程序消费，格式可读 | 把目标二进制当成本机工具 |
| 平台 bundle（实现时） | 平台入口找到包，不依赖 Linux HOME/XDG | 将 Linux 定位规则搬到全部平台 |

按实际改动运行主套件、ASan/UBSan、no-RTTI；新增测试声明 unit/integration suite，真实格式
门禁使用固定 clang-format 23.1.1。需要窗口/平台环境的用例不能把跳过算作通过。
纯资源工具及定位逻辑尽量无窗口测试；同时保留至少一条下游应用通过正式 ResourceManager
读取安装资源的集成路径，不能只检查“目录里存在 resources.db”。

### 解耦批的等价基线

在重构前固定测试输入与逻辑期望，或保存可复现运行的基线工具；不能让拆分后的同一实现同时
生成基线和被测结果。比较域限定为单挂载或 ID 不冲突的输入；既有 ID 数值的比较仅覆盖
既有 lock 已登记的资源。输入不仅是 resources.toml，还包括相同源文件及既有 lock 的独立副本。
基线与重构后工具分别写入两个输出根，不互读临时文件或源树中的自动更新结果。

读取产物后先检查重复 key，再按 key 规范排序，以 `(key, media_type, 内容摘要)` 归一并比较
逻辑内容（含外置资源）；不能先放进字典覆盖重复记录。既有 ID、别名到规范目标的映射、存储
选择及 schema 行为独立比较，不能在归一时删掉。重复 key、别名重定向和存储方式改变均须使
比较器失败；打乱读取顺序但保持逻辑记录不变应通过。

解耦批的等价基线记录的是 P2.3a 之前的行为：当时新资源分配随机身份，只验证唯一性与已有
匹配规则。P2.3a 之后新 key 的 ID 由 `package_id + key` 派生，因此**同输入两次独立生成必须
相等**，随机性只残留在既有 lock 的旧条目上（P2.3b 迁移后消失）。扫描排序只在影响公开结果、
身份匹配或冲突诊断时属于语义；单纯交换不影响结果的遍历顺序，不应让集合等价测试失败。需
守住的排序契约用专门断言，不靠 SQLite 行顺序。
两个输出根能检验产物的输出根独立性，但两次顺序构建不能证明并行安全。并行/多配置另有
实际同时构建的用例，检查临时文件与输出互不覆盖，见矩阵对应行。该基线补充既有读写/错误
用例，不替代它们，也不扩大为跨 SQLite 版本的逐字节包一致性承诺。

## 13. 冻结方向与按批次定形的事项

| 编号 | 已冻结方向 | 实施定形 / 闭环条件 | 阻塞范围 |
| --- | --- | --- | --- |
| **D5.1 身份与作用域** | key 为作者身份；管理器带作用域结果 + 弱域令牌 + 后端发布代次；裸 ID 多重命中报歧义；保留包格式 | 作用域批六项落实见下；身份批另定生成算法、包内碰撞与重建契约，不走纯内容 ID/blob 拆表 | 作用域/身份各自交付，不阻塞解耦 |
| **D5.2 构建选项** | 默认工具开/后端开/vendored；关闭组合进 CI | 选项名、合法组合、外部工具入口及每个公开组合验证；system 后续排期 | 首批可选构建 |
| **D5.3 模块/API** | 解耦先行，保持现有包/ID 行为；机械依赖门禁 | 构建目标与公共范围、共享协议归属、无 UI 配置路径、检查器覆盖传递依赖与归档输入 | 首批解耦 |
| **D5.4 声明** | resources.toml 是唯一资源语义入口 | key→路径 schema、命名空间、适配参数边界、package_directory 与错误载荷 | 声明式构建 |
| **D5.5 部署描述** | 安装与定位同源，有确定启动入口 | 编译配置或旁置描述、版本、优先级、安装覆盖规则 | 部署批 |
| **D5.6 定位策略** | portable/prefix 优先，覆盖显式启用 | 固定/相对根、符号链接、可选包错误和用户覆盖政策 | 部署批 |
| **D5.7 增量与发布** | 目录变化独立发现，输出成套发布 | 扫描机制、并行隔离、副产物清理、代次与可复现性层级 | 声明式构建 |
| **D5.8 平台边界** | 本机工具与目标程序分离，Web 不伪装同步可用 | 工具/包兼容测试，bundle 顺序，D4 预加载/异步协议 | 各平台交付，不阻塞本机解耦 |
| **D5.9 作者引用** | 推荐运行时 key + 可选生成常量头 | 符号规则、冲突、编译依赖、失效诊断；类型化访问器是否另排期 | 与 D5.1 联合定形，不阻塞解耦 |

首批只须闭环 D5.2/D5.3 的必要部分；其余按表中依赖逐项确认，不能再把九项捆成一个总开工条件。

作用域批开工前完成以下六项 API 定形，方向不再重选：

1. 管理器/后端结果类型、八个入口签名、旧解析引用的重开入口及直接裸句柄消费边界。
2. 弱域令牌的所有权比较、manager 移动、MountId 不复用与代次溢出诊断。
3. 后端发布控制点、结果/代次同快照协议与并发操作生效点。
4. 歧义错误载荷、ID 变体及缓存调用点迁移；priority 仅裁决 key。
5. 物化/内存流/非快照文件流的类别表示、保活及过期错误。
6. 缓存的作用域失效、广域操作命名、死域和活域旧代次的回收时机。

本次只冻结设计并同步计划、README、模块/构建边界与故事；不修改运行时代码，不启用新构建
路径，不替换 SQLite，不将待实现接口写成入门教程中的可用功能。

## 8. P2 实施契约（2026-10-22 定稿，P2.1a–P2.4c 已实施）

> 本节把 2026-10-22 的评审结论固化为执行契约。P2 代码按此实施；未写入本节的行为不提前承诺。
> 路线图 P2 的子批划分与状态见[开发路线](development_roadmap.md)。

### 8.0 评审决策

| 议题 | 决定 |
| --- | --- |
| 示例应用 | `examples/lab/`，普通 UI 构建默认参与，可显式关闭；单可执行文件、多路由：登录/注册、番茄钟、笔记列表与编辑、图片预览、确认弹窗；本轮不做数据库持久化、富文本编辑、密码掩码或真实认证 |
| 资源开关 | 不增加笼统 `resources`；按 8.2 的 `build_ui` / `resource_sqlite` / `resource_tools` / `examples` 四个选项 |
| API 命名 | `ResolvedResource` / `ResolvedStream`；`ResourceDomainToken` 只是不透明身份，调用方不能取得可保活域的强令牌；歧义走既有错误通道 `ambiguous_resource_id` |
| key 与 ID | 大小写敏感、`/` 分隔、不做 NFC；记录 ID 由包身份 + 逻辑 key 派生；编码/算法版本/截断/迁移按 8.4 冻结 |
| 常量头 | 在 P2.3 完成最小版（只含逻辑 key，删除声明后编译失败） |
| smoke 底线 | 构建检查与运行检查分别记录；环境不可用允许 SKIP；P2.4 最终完成必须有至少一份实际运行与交互验收证据 |

### 8.1 四层目标与依赖图

| 目标 | 职责 | 允许依赖 | 禁止依赖 |
| --- | --- | --- | --- |
| `resource_core` | key/ID、`Resource`、流、错误、`IResourceBackend` 协议、`ResourceManager` | C++ 标准库；仅 logger 等 foundation 的最小部分 | SQLite、OpenSSL、toml++、字体、渲染、场景、窗口 |
| 运行时后端 | memory / builtin / directory；可选 SQLite | `resource_core`；SQLite 后端额外依赖 vendored `sqlite3` 与包格式协议头 | 构建工具侧库、UI 目标 |
| `resource_build` | scanner、manifest、摘要、校验、packager | `resource_core`、toml++、OpenSSL；SQLite 存储时依赖 `sqlite3` + 包格式协议头 | UI 目标、窗口/字体 |
| `nanres` | CLI 解析、诊断、调用 `resource_build` | `resource_build` | 任何 UI / 运行时后端实现（SQLite 运行时后端也不得成为其依赖） |

**包格式协议头**：新增 `nandina/resource/resource_package_format.hpp`，承载读写共享的包常量与
schema 约定；`tools/resource_packager.cpp` 不再包含 `backends/sqlite_backend.hpp`，SQLite 运行时
后端关闭时打包器仍可独立构建。格式常量只定义一次，读写两侧不各自复制。

依赖边只能自上而下；`nanres` 的目标依赖闭包中不得出现 render/text/widget/scene/app 等 UI 目标
（由 8.9 的 checker 机械校验）。

### 8.2 选项真值表

| 选项 | 形状/默认 | 打开 | 关闭 |
| --- | --- | --- | --- |
| `build_ui` | boolean，true | 构建 UI 框架、showcase、examples/lab、窗口/GPU 探针与相关测试 | 不配置 SDL、freetype、harfbuzz、fribidi 等 UI 依赖；`showcase`/窗口探针强制关闭；只构建资源核心、运行时后端（按开关）与工具 |
| `resource_sqlite` | feature，enabled | 构建 SQLite 运行时后端，应用启动可按配置打开包 | 不编译该后端与自动开包路径；应用必须显式注入后端，缺后端时报 `backend_unavailable` |
| `resource_tools` | boolean，true | 构建本机 `nanres`、helper 与模板约定 | 不构建 `nanres`；需要包的应用必须通过外部工具路径提供兼容版本，并在使用前检查版本/schema |
| `examples` | boolean，true | 构建 `examples/lab` 多路由示例（要求 SDL3 与资源工具链） | 不进入构建图；`build_ui=false` 时自动跳过；显式 true 但缺 SDL/工具链时配置期报错 |

**SQLite 引擎是否构建**由“工具侧打包器或运行时后端是否需要”共同决定：`resource_sqlite=disabled`
不取消工具侧打包；`resource_tools=true` 仍需 SQLite 引擎。`auto` 语义：当前 vendored SQLite 始终
可用，`auto` 等价 `enabled`；将来引入系统来源（`sqlite_source`）时再细化三态，不在本批增加
`resource_storage`。

**纯工具配置入口**：`-Dbuild_ui=false -Dresource_tools=true` 必须在没有 SDL/字体/图形依赖的环境
完成配置与构建；这是 P2.1 的机械门禁，不靠“关了 sdl_backend”推断。

### 8.3 启动策略契约

- `NanApplicationConfig::resource_package` 的默认值改为空：只有显式配置包时启动才查找并挂载；
  “默认 resources.db”不再是隐含行为（破坏性变更记入 `sdk_surface.md`）。
- 后端注入发生在第一次字体/图片/主题消费之前；`resource_sqlite` 关闭时包路径代码不参与编译，
  应用改用显式注入或得到 `backend_unavailable` 诊断，不静默跳过。
- 平台定位只在使用包配置时执行；不支持的平台明确报错，不继续强制 HOME/XDG。
- `build_ui=false` 时 `app`/窗口目标不进入构建图，因此启动策略只在 `build_ui=true` 的配置下验收。

### 8.4 生成数据归属、身份与三种 revision

**归属**：源码目录只保存 `resources.toml`、显式 key 映射与别名；生成的 lock、摘要与扫描缓存
放到各自 build 目录。P2.1 保留旧行为（`resources.lock.toml` 写在策略目录），P2.3b 显式迁移，
并保证普通构建不再向源码目录回写生成数据。`nanres` 新增 `--lock <path>`；构建 helper 把 lock
写到 package 输出旁的 `<output>.lock.toml`（例如 `build/resources.lock.toml`），不再在源码树
生成 lock。没有 build 目录上下文的独立 CLI 仍默认读写策略目录的 `resources.lock.toml`。

| 概念 | 决定 | 不稳定因素 |
| --- | --- | --- |
| 记录 ID | 由包身份 + 逻辑 key 派生；内容变化不改变 ID | 算法版本升级需迁移 |
| 构建 revision | 有上一份 lock 时可递增；删除缓存后不承诺恢复历史计数 | 缓存丢失 |
| 运行时发布代次 | 后端生命周期内的缓存一致性身份；不从构建 revision 推导 | 进程/挂载生命周期 |

**碰撞规则**：同一 ID 对应**不同逻辑记录**（key 不同，或同 key 内容与记录不一致）才是碰撞，
构建失败并给出冲突诊断；相同 key 的内容变化是正常更新并推进 revision；不同 key 的相同内容
保留两条独立记录。

**ID 编码冻结**（P2.3a 实施）：输入为
`"nandina.resource.id.v1\0" + u32_be(len(package_id)) + package_id + u32_be(len(key)) + key`，
对输入取 SHA-256 后取前 16 字节（128 bit），小写 UUID 形式输出；固定测试向量见
`tests/resource_tests.cpp`。碰撞时不随机换 ID，而是失败。
旧 lock（随机 ID）需通过显式命令（如 `nanres scan --rewrite-lock`）迁移；普通构建检测到不匹配
时提示迁移，不静默改写。P2.3a 先对**新 key** 使用派生 ID；已有 lock 条目按同 key 保留旧 ID，
迁移与重建确定性留给 P2.3b。

**P2.3b 迁移**：`build_resource_lock` 现在始终写派生 ID。`scan`/`validate`/`pack` 遇到与派生
ID 不一致的 lock 会失败并提示 `nanres scan --rewrite-lock --lock <path>`；只有显式
`--rewrite-lock`（仅 scan）才重写。`--lock`/`--rewrite-lock` 只在 manifest 模式生效：带
`--root`/`--exclude`/`--type` 等原始扫描覆盖时它们是用法错误，不会静默忽略。lock 的
`package_id` 与 policy 不一致时先报包不匹配，不把包错误误导成“旧 ID 需要迁移”。加载 lock 时
同一 ID 对应不同 key、或同一 key 对应不同 ID 会列出冲突项并失败。无变化时 `scan` 不重写
lock，helper 也不重写 `resource-location.json`；独立目录的同策略重建得到逐字节相同的 lock。

### 8.5 一致性分类与缓存资格

| 来源 | 一致性类别 | 可进入共享内容缓存 |
| --- | --- | --- |
| 内存/内置后端 | 不可变快照 | 可 |
| SQLite 内嵌字节 | 包代次内不可变 | 可（键含域/挂载/代次/ID） |
| 目录后端 | 仅显式发布/不可变声明的内容按快照 | 仅快照 |
| 已打开文件流 | 非快照（外部可改） | 不可 |

目录后端的资源结果同样携带一致性类别；相同路径在同一代次返回不同字节不得被缓存复用。
首版不承诺文件监听；非快照内容不得仅凭作用域键进入共享内容缓存。

### 8.6 错误通道

沿用 `ResourceErrorCode` + `expected`，新增：

- `ambiguous_resource_id`：裸 ID 多重命中，消息必须列出冲突挂载（MountId/后端名）；
- `expired_scope`：域已销毁、挂载已卸载或发布代次过期后的重新解析；
- `generation_exhausted`（P2.2a 实施补充）：发布代次已达 64 位上限，后端拒绝发布而不是回绕；
- 既有 `duplicate_id` / `alias_conflict` 保留后端配置语义，不混用为查询歧义。

不引入第二套异常类型；结构化冲突信息进消息字段（后续如需结构化类型再单独加，不阻塞本批）。

### 8.7 缓存回收

首版采用**有限容量淘汰 + expired 域清扫**：缓存条目持域弱令牌与代次；manager 存活但旧挂载/
旧代次的关联由定向失效或容量淘汰清理。暂不实现 manager 向所有缓存广播失效的通知系统。
直接加载未挂载 `ResourceHandle` 的入口绕过共享缓存，不伪造挂载身份。
P2.2d 实施：FontLoader 用 `FontLoaderLimits{max_entries}` + LRU，并在访问时清扫 expired 域/
weak face；TextureCache 在 `begin_frame()`/`trim()` 清扫 expired 域，`invalidate(resolved)`
做定向清理，既有 retained 容量限制负责存活域旧挂载/旧代次。

### 8.8 增量与发布义务分配

| 工作 | 批次 |
| --- | --- |
| 工具关闭时的外部 nanres 入口、版本/schema 兼容检查 | P2.1 |
| `validate` 区分“过期”与“策略错误/损坏”；只有过期自动重扫 | P2.3 |
| 常量头的符号冲突、删除再生成、真实编译依赖测试 | P2.3c（已实施） |
| 目录新增/删除发现；无变化不重写包/定位描述/时间戳 | P2.4a |
| 数据库、外置资源与定位描述成套发布；失败保留旧产物但返回失败 | P2.4a |
| 定向构建应用即资源就绪；portable stage 重定位后可运行 | P2.4a |

### 8.9 门禁、fixture 与跨编译

- **依赖闭包 checker**（新 `tools/check_tool_dependencies.py`）：先用项目固定的 Meson 版本建立
  真实小型 fixture，再解析 `meson introspect --targets`；同时使用外部 `dependencies` 与内部目标
  `depends`，递归闭包，拒绝 UI 目标；查询失败/目标缺失/输入为空必须报错（fail-closed）。
  `ninja -t commands` 只作诊断附加证据。故障注入：直接链入 UI 库、经中间目标间接引入、
  introspection 缺失，三种都必须变红。
- **纯工具构建 job**：无 SDL/字体依赖环境配置并构建 `nanres`；与现有无后端 job 分开。
- **交叉编译**：只正式验收原生构建；`nanres` 是构建期工具，交叉编译时不得执行目标架构的
  `nanres` 作为本机工具。跨编译支持留待独立批次。
- **外部工具兼容**：`nanres` 提供可解析的版本/schema 输出（如 `nanres --version` /
  `nanres schema`），helper 在使用外部工具前检查；不兼容时明确失败。

### 8.10 子批、提交与验收节奏

| 子批 | 内容 | 会红测试/门禁 |
| --- | --- | --- |
| P2.1a | 目标拆分（core/runtime/build/CLI）+ 包格式协议头 + 选项真值表 | 纯工具配置构建；依赖闭包 checker + 三种故障注入 |
| P2.1b | 启动策略（默认不开包、注入时机、SQLite 关闭诊断）+ 外部工具兼容 | 关闭组合运行；无包/缺后端诊断；`sdk_surface` 更新 |
| P2.2a | 后端发布代次与快照一致性 | 并发发布/快照一致性测试 |
| P2.2b | 管理器 8 入口作用域 + 域令牌 + 歧义/过期错误 | 跨挂载/跨 manager/歧义/过期用例 |
| P2.2c | FontLoader 与图片/流消费者迁移 | 缓存身份测试 + ASan |
| P2.2d | 缓存回收与并发 | 淘汰/清扫/重叠卸载用例 |
| P2.3a | 策略 key 映射 + 生成算法 + ID 编码测试向量 | 重命名/大小写/固定向量 |
| P2.3b | lock 迁移、碰撞、revision、无变化不重写 | 碰撞注入/重建确定性/迁移 |
| P2.3c | 最小常量头 + 编译依赖测试（**已完成 2026-10-25**） | 删除声明后编译失败；符号冲突 |
| P2.4a | 示例资源链路与增量/成套发布（**已完成 2026-10-25**） | 干净检出/离线/定向构建/portable stage |
| P2.4b | 示例应用功能（导航/表单/图片/滚动/弹窗，**已完成 2026-10-25**） | 运行 smoke + 人工交互证据 |
| P2.4c | 测试脚本、失败可复现证据、README（**已完成 2026-10-25**） | 构建检查与运行检查分别记录 |

每个可独立验收的子批一个提交；会红测试先在本地证明旧实现失败，再与修复一起提交；公开 API
迁移提交同时迁移消费者，保持可构建、可回滚；完整验证矩阵在子批收口执行，日常修改先跑相关测试。

### 8.11 P2.1a 实施记录（2026-10-22）

目标拆分已实现：`nandina/resource/meson.build` 定义核心、运行时后端和构建核心，`nanres`
只链接 `resource_build_dep`。BuiltinBackend 的字体是内嵌字节，不引入字体引擎；应用专用的
开发定位 JSON 读取仍在 UI 集成侧。共享包格式头为读写两侧提供 application/schema 常量。

四个选项已注册；`examples` 早期仅预留，P2.4 后随普通 UI 构建默认参与，`build_ui=false` 时跳过。`build_ui=false` 强制排除 UI
目标及其配置期依赖。`resource_sqlite=auto` 等价 enabled；SQLite 引擎由工具或运行时任一
消费者启用。工具关闭时不会查找 OpenSSL；运行时与工具都关闭时不编译 SQLite。
P2.1b 的默认不开包、外部工具入口/握手尚未实施，不将它们算作本批交付。

门禁使用 Meson 目标、外部依赖元数据和 Ninja 实际输入图，核对归档成员与对象来源；
不以链接器是否抽取 UI 符号判断依赖。固定 Meson 1.7.2 的真实 fixture 证明正常路径通过，
直接/间接 UI 库、缺失 introspection、缺失 response file、额外归档成员均被拒绝。
旧 nanres 被该 checker 拒绝；旧配置的 `build_ui=false` 因选项不存在失败，均在重构前确认。

逻辑等价基线由重构前保存的 nanres 生成并入库于 `tests/fixtures/resource_equivalence/`；
独立输出根比较既有 ID、别名、存储类别、media type、内嵌/外置内容摘要，拒绝重复 key。
检查器和等价测试都进入 Meson integration suite；release check 增加工具依赖门禁。
CI 增设只预热 toml++/Catch2、只安装工具所需系统开发包的资源配置矩阵。

### 8.12 P2.1b 实施记录（2026-10-22）

- **默认不开包**：`NanApplicationConfig::resource_package` 默认改为空；只有显式配置才创建平台
  定位并自动挂载包。默认启动不再因为可执行文件旁恰好存在 `resources.db` 而改变资源可见性。
- **显式注入**：新增 `NanApplication::mount_resource_backend(backend, priority)`；后端由应用
  保活，要求窗口/字体/图片首次消费前调用。与自动发现可共存，priority 语义沿用管理器。
- **SQLite 关闭组合**：`resource_sqlite=disabled` 时自动开包分支不参与编译，配置了包名会得到
  `backend_unavailable` 诊断而不是静默跳过。
- **外部工具入口**：`resource_tools=false` 时可用 `-Dexternal_nanres=<path>`；配置期执行
  `nanres schema`（必须为 1）与 `nanres --version`，失败即配置失败。`resource_tools=false`
  且未提供外部工具时 `nandina_resource_toolchain` 不存在并给出 warning，消费方会在取变量时
  明确失败。helper 在每次构建前也做 schema 握手，防止外部工具在打包中途才暴露不兼容。
- **验证**：`-Dbuild_ui=false` 的 5 种组合（工具/后端/auto）在固定 Meson 1.7.2 下完成干净
  配置、构建与测试；外部工具的正常/不兼容/已关闭三种路径有 fixture 覆盖；默认启动与注入的
  应用级测试通过；主套件 104/104、无 SQLite 的 UI 构建 81/81、Windows 构建与 unit 通过（复用旧构建目录时需清理一次 stale thin archive，见依赖策略升级说明）。

### 8.13 P2.2a 实施记录（2026-10-23）

- **协议结果**：`IResourceBackend::find()` / `open_stream()` 返回
  `ResourceResult<ResourceSnapshot>` / `ResourceResult<ResourceStreamSnapshot>`。
  快照同时携带 `PublishGeneration`、`ResourceConsistency` 与可选句柄；负结果也是
  “该代次下的未命中”，不是错误。管理器公开八入口仍返回裸句柄，本批只在其内部解包；
  域令牌、MountId 与 `ResolvedResource`/`ResolvedStream` 留给 P2.2b。
- **同一临界区**：MemoryBackend 在持有读锁期间读取结果与代次，在持有写锁期间完成变更与
  代次推进。`insert`（含 replace）、`add_alias`、`erase` 成功才推进；所有校验、代次耗尽与
  资源分配都在修改映射之前完成，错误路径不留下半份状态。代次为 64 位单调值，`next()` 在
  最大值返回空，后端以 `generation_exhausted` 拒绝发布，绝不回绕复用旧身份。
- **一致性分类**：memory/builtin 为 `immutable_snapshot`；SQLite 内嵌字节为
  `package_snapshot`，SQLite 外置文件为 `non_snapshot`；目录后端因外部可改写且无发布入口，
  结果与流一律 `non_snapshot`。非快照结果不得仅凭作用域键进入共享内容缓存。
- **挂载与并发**：管理器继续在自身锁外复制挂载列表再调用后端；同一 backend 多次挂载共享
  同一次发布代次、各自保留独立 MountId。与卸载重叠的查询可固定旧结果，卸载完成后新解析失败。
- **测试**：`resource` 新增代次单调/不回绕、失败不推进、别名/替换/删除发布、三类一致性、
  负结果（资源与流）也携带所属代次与一致性类别、同一 backend 多挂载、并发发布与快照配对、
  阻塞后端证明管理器不在锁内调用后端。并发用例在临时拆开 MemoryBackend 读锁后会稳定失败
  （3/3），恢复后通过；主套件 104/104。ASan 配置下单元套件 **83/83、window-lifecycle SKIP**：
  该测试会打开真实 GPU 窗口，Mesa 驱动在 LeakSanitizer 下报告 256B 泄漏（栈在驱动线程初始化，
  非项目代码，普通构建不受影响），因此 ASan 配置按既有做法禁用窗口测试；其余资源/应用/字体
  用例全绿。临时失败注入不入库。

### 8.14 P2.2b 实施记录（2026-10-23）

- **管理器结果**：八个公开入口改为返回 `ResolvedResource` / `ResolvedStream`，携带
  `ResourceDomainToken`、`ResourceMountId`、`PublishGeneration`、`ResourceConsistency`、
  记录 ID 与句柄；`find`/`open_stream` 仍为 `optional`，未命中和错误通道不变。旧裸句柄
  别名删除；FontLoader、FontFamily、纹理缓存与 Image 只做编译级迁移（缓存身份仍是裸 ID，
  留给 P2.2c）。直接加载未挂载 `ResourceHandle` 的入口本批不改缓存行为，P2.2c 按 §6.2.1
  改为绕过共享缓存或使用独立身份，不伪造挂载身份。
- **域令牌**：`ResourceDomainToken` 只持有域的弱引用，不保活 manager；相等/排序比较控制块
  所有权，而不是 `lock().get()`，因此两个已销毁域仍是不同身份。manager 移动构造/赋值转移
  同一逻辑域。`MountId` 在域内安装顺序递增，卸载/`clear` 后不复用；耗尽时抛
  `std::overflow_error`，不回绕。
- **查询规则**：key/别名沿挂载优先级短路，别名只在所属后端内解析；裸 ID 扫描全部挂载，
  同一后端多次挂载也算多个命中，大于一个即返回 `ambiguous_resource_id`，消息列出
  “mount <id> (<backend>)”。扫描中任一后端错误直接返回，不会被忽略后宣称唯一命中。
- **重新解析**：新增 `resolve(ResolvedResource)` / `resolve(ResolvedStream)`，只使用引用记录的
  domain + MountId + 记录 ID，按 ID 重新查询原挂载，不回退到全树 key/ID 搜索。域不同、
  挂载已卸载或发布代次变化返回 `expired_scope`；同代次下缺失返回 `not_found`。已取得的
  资源/流仍可继续使用。
- **并发**：挂载列表在管理器锁内复制后在锁外查询后端；与卸载重叠的查询可固定旧结果，
  卸载完成后新查询与旧作用域重新解析都失败。ID 扫描的生效点是挂载列表快照。
- **测试**：`resource` 新增结果字段、跨挂载/同后端重复挂载的裸 ID 歧义、错误不被忽略、
  别名优先级、发布/卸载/跨 manager 过期、无回退、域令牌弱引用与所有权比较、manager 移动、
  MountId 不复用，并扩展阻塞后端用例验证固定结果过期。复核补充：多个挂载中只有一个持有
  目标 ID 时仍是唯一命中；`clear()` 后新引用过期；未绑定域的默认引用对活 manager 解析返回
  `expired_scope`。红测注入：ID 扫描短路时歧义用例失败；令牌相等退化为 `lock()` 指针比较
  时“两个已销毁域不同”失败。验证：主套件 104/104、无 SQLite UI 单元 81/81、纯工具套件 7/7、
  ASan 83/83 + window SKIP、release check 8/8、Windows 构建与 unit 通过。完整结果见
  [开发路线](development_roadmap.md) 的 P2.2b 行。

### 8.15 P2.2c 实施记录（2026-10-23）

- **共享身份**：`ResourceScopeIdentity` 固定为 `(域所有权身份, MountId, 发布代次, ResourceId)`；
  `scope_identity(ResolvedResource/ResolvedStream)` 供消费者派生缓存键。
  `ResourceManager::domain()` 暴露弱域令牌，供解析前的 pending/缓存范围区分。
- **FontLoader**：`FontFaceKey` 改为 `(ResourceScopeIdentity, face_index)`；同一挂载的新代次、
  跨挂载同 ID 均产生新 face。`non_snapshot` 来源不进入共享缓存；直接
  `ResourceHandle` 入口绕过缓存、不伪造挂载身份。失效拆为
  `invalidate(const ResolvedResource&)`（同源作用域）与 `invalidate_wide(ResourceId)`
  （广域、只作用于本实例）。
- **TextureCache**：资源来源新增 `SourceKind::resource`，缓存键为
  `(ResourceScopeIdentity, options)`；新增同步 `load_resolved(const ResolvedResource&, options)`。
  异步 `load_resource_async` 仍把 `require` 与解码留在后台，pending 去重按
  `(域, 逻辑 key, options)`，解析完成后才形成最终缓存键；`non_snapshot` 上传后不索引、不进
  保留列表。文本 key 的 `load_memory` 仍只服务调用方显式提供的非资源内存，不参与资源作用域。
- **Image**：同步 res:// 路径改用 `load_resolved`；异步路径保持 API，但缓存决策移到解析后。
- **测试**：先写六条红测并确认旧实现失败：FontLoader 的旧代次/跨挂载同 ID/非快照目录/直接句柄，
  以及 Image 同步旧代次、目录非快照、异步旧代次；实现后转绿。另加作用域失效只影响自身 scope、
  异步不同逻辑 key 不合并；复核补充同步同代次缓存命中、异步非快照上传后不索引/不保留。
- **验证**：主套件 104/104；无 SQLite UI 单元 81/81（+1 SKIP）；纯工具套件 7/7；
  ASan 单元 83/83（+ `window-lifecycle` SKIP，驱动侧 LSan）；no-RTTI 单元 84/84；
  release check 8/8、Windows 构建与 unit 82/82。完整结果见[开发路线](development_roadmap.md) 的 P2.2c 行。

### 8.16 P2.2d 实施记录（2026-10-24）

- **FontLoader 回收**：新增 `FontLoaderLimits{max_entries = 64}`；缓存项记录 `last_used`，
  访问时先清扫 expired 域与已释放的 weak face，再按 LRU 淘汰到上限；淘汰只删除关联，不销毁
  使用者仍持有的 face。新增 `cached_entries()` 供诊断与测试。
- **TextureCache 回收**：`sweep_expired()` 在 `begin_frame()` 与 `trim()` 时删除已销毁域的索引/
  保留关联；`invalidate(const ResolvedResource&)` 定向删除单个作用域。既有
  `max_retained_entries`/`max_retained_bytes` 继续负责存活域内的旧挂载/旧代次；卸载本身不广播、
  不清扫，进行中的异步完成仍可交付（固定旧结果）。
- **异步重复解码**：资源异步加载改为两阶段：后台只解析作用域 → UI 线程按作用域查缓存 →
  未命中才提交后台解码 → UI 上传。缓存命中时不再解码/上传；pending 仍按
  `(域, 逻辑 key, options)` 合并。C5.5 的“UI 线程不读资源、不解码”保持。
- **测试**：FontLoader LRU 淘汰、weak face 清扫；TextureCache expired 域清扫、卸载后由容量
  淘汰、定向失效、异步跨卸载完成、异步缓存命中不触发解码/上传。复核补充：FontLoader 命中
  刷新最近使用时间后淘汰顺序跟随 LRU 而非插入序。红测注入：禁用容量循环、短路
  `sweep_expired`、跳过命中检查分别使对应用例失败。
- **验证**：主套件 104/104；无 SQLite UI 单元 81/81（+1 SKIP）；纯工具套件 7/7；
  ASan 单元 83/83（+ `window-lifecycle` SKIP，驱动侧 LSan）；no-RTTI 单元 84/84；
  release check 8/8、Windows 构建与 unit 82/82。完整结果见[开发路线](development_roadmap.md) 的 P2.2d 行。

### 8.17 P2.3a 实施记录（2026-10-24）

- **显式 key 映射**：`[[resources]]` 支持相对 `path`，与 `key` 一起声明 key→源文件映射；
  文件不必位于扫描根内。显式映射优先于路径派生 key：同一物理路径不会再生成派生记录——
  扫描器通过 `ResourceScanOptions::mapped_paths` 直接跳过这些文件，既不派生 key 也不报诊断，
  因此扫描根内无法形成规范 key 的文件（如 `Fonts/Default.ttf`）也能由映射声明。
  缺失文件、非普通文件、被禁止的 symlink、同一路径映射多个 key、与派生记录重复 key 都返回
  明确错误。`path` 必须是相对路径且不含 `.`/`..`；rooted 形式（Windows 的 `C:foo`、`\foo`）
  与绝对路径同样拒绝。路径身份比较使用 `resource_path_identity(...)`（绝对化 +
  `lexically_normal` 的 generic string；Windows 上再做 ASCII 大小写折叠），所以 NTFS 上大小写
  不同的拼写仍视为同一物理文件。
- **key 生成**：新增 `derive_resource_key(prefix, relative)`，冻结为
  `prefix + "/" + 相对路径`（空前缀直接使用相对路径）；保留大小写、扩展名与 `/` 分隔。
  `ResourceKey` 仍只接受小写规范段，因此大写路径被诊断为 `invalid_resource_key`。扫描器的
  大小写归一化碰撞检查与槽位登记发生在 key 验证**之前**：非规范名也占住自己的归一化槽位，
  所以 `Upper.txt`/`upper.txt` 这类碰撞的归属不取决于目录遍历先看到哪一个。
- **ID 编码**：新增 `derive_resource_id(package_id, key)`，按 §8.4 的版本前缀 + NUL +
  big-endian u32 长度 + package_id + key 取 SHA-256，截断前 128 bit 并格式化为小写 UUID。
  `tests/resource_tests.cpp` 固定三组向量（已用独立实现复算校验）并覆盖同输入稳定、不同
  package/key 不同 ID，以及长度前缀使 `("ab","c")` 与 `("a","bc")` 分离。
- **lock 身份**：`build_resource_lock` 改为按 key 匹配上一份 lock；同 key 的内容变化保留 ID
  并推进 revision，源文件重命名但 key 不变同样保留 ID；新 key 使用派生 ID，内容相同的不同
  key 各自拥有独立 ID，不再按内容哈希继承旧身份。已有 lock 的旧随机 ID 暂按同 key 保留，
  `--rewrite-lock` 与构建目录迁移留给 P2.3b。
- **复核补充（2026-10-24 评审）**：
  - 修复重构引入的一处语义变化：`normalized_keys` 的登记被误移到 key 验证之后，会让
    “大写名先被遍历”时丢失归一化碰撞诊断（诊断随目录遍历顺序漂移）。已恢复“先查碰撞 →
    登记槽位 → 再验证 key”的既有顺序，并新增 `scanner keeps case-insensitive slots for
    non-canonical names` 用例（两个根固定顺序构造该场景；注入旧顺序时断言失败）。
  - `merge_explicit_resources` 复用 `resource_scan_options(policy).type_rules`，不再复制
    media-type 规则提取逻辑，避免 `[[rules]]` 两处漂移。
  - 补充 CLI 边界：映射源在扫描根外（同 resources.toml 目录）、映射源在扫描根内但无法派生
    规范 key、同 key 内容变化保 ID 且 revision 递增、无变化重扫不换 ID/不推进 revision、
    缺失/目录/绝对路径/上跳越界/空路径/同路径多 key/与派生 key 重复/被禁止 symlink 全部报错。
  - 旧 lock 随机 ID 的保留行为有独立用例：伪造一条随机 ID 的 lock 条目后修改内容，`scan`
    仍保留该 ID 且 `revision` 推进到 2（模拟 P2.3b 迁移前的历史 lock）。
  - 追加 `[[rules]]` 的 storage/streaming 对显式映射条目的优先级断言与
    `derive_resource_key` 拒绝 `a/../b`、绝对输入的断言。
- **已知边界**：单目录内同时存在 `Upper.txt` 与 `upper.txt` 时，拒绝成立（`normalized_key_
  collision` 必报），但另一条 `invalid_resource_key` 的归属仍取决于遍历顺序；完整确定性
  留给 P2.3b 的“重建确定性”项。Windows 路径身份只做 ASCII 大小写折叠，未覆盖 NTFS 逐目录
  区分大小写、junction 与 8.3 短名别名；符号链接检查只针对路径最后一段（父目录为 junction
  时未检测）。`[[resources]]` 的 `path` 不支持目录、绝对路径或 `./..`；目录声明与安装布局
  按 P2.4 收口。
- **测试**：先改 CLI 断言并确认旧实现失败（派生重命名仍复用旧 ID、显式 `path` 被忽略）；
  实现后转绿。新增 C++ 固定向量/键派生/长度前缀/大小写槽位测试与上述 Python 边界用例。
- **验证**：主套件 104/104；无 SQLite UI 单元 81/81（+1 SKIP）；纯工具套件 7/7；
  ASan 单元 83/83（+ `window-lifecycle` SKIP，驱动侧 LSan）；no-RTTI 单元 84/84；
  release check 8/8（format 23.1.1、headers、docs 79 篇等）。Windows（`8bb7fec`，bundle
  同步、工作区干净）：reconfigure/构建 0 错误，`--suite unit` 82/82、0 失败；另用定向脚本
  在 Windows 上复验：映射 key 的固定 ID 向量 `c73735d1-…`、扫描根内不可派生名的显式映射、
  `\foo`/`C:foo` 拒绝、NTFS 大小写折叠身份（只产出一条记录）；`3c86927` 起 `nanres-cli` 在
  Windows 全绿，这些 Windows-only 断言随套件执行，不再依赖临时脚本。
- **Windows 打包修复（后续独立批次，见 §8.18）**：原 `nanres-cli` 在 Windows 的 `pack` 步骤
  因数据库语句未 finalize、目录重命名被拒而失败；同时 `unchanged` 指纹读写在 CRLF 上不一致。
  §8.18 已修复并复验，Windows 打包套件现已通过。
- 完整结果见[开发路线](development_roadmap.md) 的 P2.3a 行。

### 8.18 Windows 资源包安装与指纹修复（2026-10-24）

- **安装失败根因**：`pack_resource_package` 在 `sqlite3_close_v2` 时仍有
  `insert_resource`/`insert_alias` prepared statement 存活，SQLite 转为 zombie 连接并保留
  文件句柄；随后 `rename(temporary, output)` 在 Windows 上被拒绝。POSIX 允许带打开句柄重命名
  目录，因此 Linux 未暴露。
- **修复**：数据库写入与两条语句放入内层作用域，语句 finalize 后再显式 `close_database(...)`；
  关闭改用 `sqlite3_close` 并把 `SQLITE_BUSY` 等失败转成错误，不再静默延迟关闭。
- **指纹平台差异**：`.nanres-pack.sha256` 原先文本模式写入、二进制模式读取，Windows 的 CRLF
  会让 `unchanged` 判定失败并反复重打包。现在写入使用二进制模式，读取时兼容并去除行尾 `\r`。
- **测试夹具**：`tests/*.py` 中打开 SQLite 的临时目录改用 `contextlib.closing`，避免 Windows
  上测试结束时连接仍持有 `resources.db` 导致临时目录清理失败。
- **测试**：新增 CRLF 指纹回归（Linux 可运行；撤掉读取归一化时 `unchanged` 断言失败）；Windows
  上 `nanres-cli` 的 `pack`、`unchanged`、外置文件、重建路径全部通过，另跑
  `nanres-build-workflow`、`portable-stage` 与 `--suite unit`。
- **验证**：主套件 104/104；无 SQLite UI 单元 81/81（+1 SKIP）；纯工具 7/7；ASan 83/83
  （+ `window-lifecycle` SKIP）；no-RTTI 84/84；release check 8/8。Windows 在 `3c86927`（bundle
  同步）上构建 0 错误，`--suite unit` 82/82，`nanres-cli` / `nanres-build-workflow` /
  `portable-stage` 3/3 通过；复核在 Linux 侧独立复跑主套件 104/104 与 no-SQLite/tools/ASan/
  no-RTTI 矩阵（计数与基线一致）。

### 8.19 P2.3b 实施记录（2026-10-24）

- **lock 位置**：`nanres` 新增 `--lock <path>`，`scan`/`validate`/`pack` 统一读写该路径；
  独立 CLI 未指定时仍默认策略目录的 `resources.lock.toml`。构建 helper 把 lock 写到
  `<output>.lock.toml`（与 package 目录同级），源码树不再出现生成的 lock。
- **迁移**：`build_resource_lock` 始终生成派生 ID。普通 `scan`/`validate`/`pack` 检测到旧 lock
  的 ID 与派生结果不一致即失败，提示 `nanres scan --rewrite-lock --lock <path>`；只有显式
  `--rewrite-lock`（仅 scan）才重写。迁移保留同 key 的 revision 与内容，别名/存储/记录不变。
- **碰撞**：加载 lock 时同一 ID 对应不同 key、或同一 key 对应不同 ID 会列出冲突项并失败；
  `build_resource_lock` 仍用 `id_owners` 检查派生 ID 碰撞。
- **确定性/无变化**：相同策略与资产的独立目录得到逐字节相同 lock；`scan` 在
  `same_lock(previous, generated)` 为真时跳过写入；helper 只在 `resource-location.json` 内容
  变化时替换描述文件。`pack` 的无变化检测保持。
- **测试**：先加 CLI 迁移/碰撞/确定性断言并确认旧实现失败（旧 lock 被静默保留、无 `--lock`）；
  实现后转绿。新增 `--lock` 源码无回写、lock mtime 不变、独立目录 lock 字节一致、同 ID 不同 key
  碰撞报错、旧随机 ID 迁移与 `--rewrite-lock` 限制；fixture lock/expected ID 迁移为派生值。
- **复核补充（2026-10-25 评审）**：`--lock`/`--rewrite-lock` 在原始扫描模式下原本会被静默
  忽略，现返回用法错误；lock 的 `package_id` 与 policy 不匹配时先报包不匹配再谈迁移。新增
  “同 key 不同 ID”碰撞用例、legacy lock 的 `validate` 迁移提示、manifest-only 标志与包不匹配
  断言，并各自注入失败确认红测成立。
- **验证**：主套件 104/104；无 SQLite UI 单元 81/81（+1 SKIP）；纯工具 7/7；ASan 83/83
  （+ `window-lifecycle` SKIP）；no-RTTI 84/84；release check 8/8。Windows（bundle 同步、工作区
  干净）：构建 0 错误，`nanres-cli`/`nanres-build-workflow`/`portable-stage` 3/3，`--suite unit`
  82/82。完整结果见[开发路线](development_roadmap.md) 的 P2.3b 行。

### 8.20 P2.3c 实施记录（2026-10-25）

- **命令**：`nanres keys --header <path>` 只读取 `resources.toml` 与扫描结果，不读写 lock，
  也不修改源码树；`--root`/`--exclude`/`--type`/`--output`/`--lock` 等 `keys` 不接受的选项
  返回用法错误 2。其它命令携带 `--header` 同样被拒绝。
- **内容**：生成头只含 `inline constexpr std::string_view` 逻辑 key 常量，不写内部 ID、
  `sha256`、源文件路径或安装位置。规范 key 与 `[aliases]` 别名都会生成；值为作者 key 本身。
- **命名**：命名空间为 `nandina::resource_keys::pkg_<package-id>`，包 ID 中的 `.`/`-`/`_`
  分别转义为大写 `DOT`/`DASH`/`UNDERSCORE`，避免不同包折叠到同一命名空间；包含保护由同一标识符
  大写生成。key 分隔符折叠为 `_`，并加 `RESOURCE_` 前缀，例如
  `images/nandina_cover_1.jpg` → `RESOURCE_IMAGES_NANDINA_COVER_1_JPG`。
- **冲突**：两个 key（含别名）折叠成同一符号时生成失败，诊断同时列出两个 key 与目标符号，
  不静默改名，也不写出头文件。
- **无变化**：内容未变时不替换文件，保留 mtime；删除 `[[resources]]` 声明后重新生成会移除
  对应符号，使用该符号的消费者随之编译失败。
- **测试**：新增 `tests/nanres_keys_test.py`（integration）：真实 C++ 编译成功/失败、别名、
  删除声明、符号碰撞、无变化 mtime、不生成 lock、拒绝扫描覆盖。红测注入：移除符号碰撞
  检查后碰撞用例失败；文件已存在时无条件跳过写入后删除声明用例失败；恢复后转绿。
- **复核补充（2026-10-25 评审）**：补充“包名 `.`/`-`/`_` 转义后消费者按命名空间 include 并
  编译通过”的断言、空 policy 生成不含常量的空头、以及头内容不得出现源路径 `assets/` 的断言；
  注入“去掉 `_` 转义”的失败确认新断言会变红。
- **验证**：主套件 105/105（新增 `nanres-keys`）；no-SQLite UI 单元 81/81（+1 SKIP）；
  no-RTTI 单元 84/84 且 `nanres-keys` 通过；纯工具 8/8；ASan 单元 83/83（+ `window-lifecycle`
  SKIP）；release check 8/8（format 23.1.1、headers、docs 79 篇等）。Windows（bundle 同步、
  工作区干净）：构建 0 错误，`--suite unit` 82/82，`nanres-keys`（真实 C++ 编译依赖测试）与
  `nanres-cli`/`nanres-build-workflow`/`portable-stage` 4/4 通过。

### 8.21 P2.4 实施记录（2026-10-25）

- **形态**：一个 `examples/lab` 可执行文件、五个路由（Account 的 Sign in/Register、Workspace
  的 Notes/Focus/Preview）。没有拆成多个 target：多可执行文件会让资源包、定位描述与 portable
  stage 的验收重复多份，P2.4a 的证据反而分散。示例随普通 UI 构建默认参与（`-Dexamples=false`
  可关闭）；构建要求 `build_ui=true`、SDL3 与资源工具链。`buildDir-examples` 只是干净验收
  目录，不是 Meson 配置要求；日常复用 `buildDir` 即可。
- **P2.4a 资源链**：`resources/resources.toml` + 两张本地位图；Meson 同时运行
  `nanres_build_helper.py build`（包与 `resource-location.json`）和 `keys`（生成常量头）。
  `lab_resource_keys` 列入 executable sources，保证编译前生成；`link_depends: lab_resources`
  保证定向 `meson compile ... nandina_lab` 也先产出包。运行期只依赖 exe 旁的 `resources/`。
  新增 `example-lab-incremental`：复制 policy 后新增/删除资源、检查无变化不重写，并注入缺失
  映射确认失败保留旧数据库。
- **P2.4b 功能**：`LabStore` 保存本地会话与笔记；登录/注册表单用 `Signal` 双向绑定、computed
  禁用态与 invalid 反馈，demo 按钮只做本地预填；Notes 用 `for_each` keyed list、TextArea
  长文/滚动、保存/新增/删除与 `AlertDialog`；Focus 是 `NanControl::on_process` 驱动的 25/5
  timer，支持启动/暂停/重置/跳过；Preview 用生成常量拼出 `res://` 加载资源图片。密码字段可见
  且不持久化，README 明确标注。
- **P2.4c 测试/文档**：`tests/example_lab_test.py` 分为 build、run、incremental 三种模式；
  run 模式先用构建树包（`resource-location.json`）运行 Preview，再通过 `portable_stage.py`
  暂存、移动后依次运行 login/notes/focus/preview 四个路由，断言退出码 0 且日志无页面错误、
  resource lookup 或纹理解码失败标记；无 display 时退出 77（Meson SKIP）。`examples/lab/README.md`
  记录构建、路由、测试钩子、portable stage 与边界；showcase 继续承担组件视觉验收，示例不新增
  页面 golden。
- **崩溃回归（2026-10-25）**：首次新建笔记时 `LabStore::next_note_id_` 从 1 起步，与种子笔记
  id 1 冲突；`for_each` 以重复 key 抛出 `std::invalid_argument`，程序 SIGABRT。修复为从当前
  `notes` 模型推导下一个 id（也覆盖调用方直接设置 notes 的情况），并新增 `lab-store` 单测：
  旧实现下种子冲突与外部 id 50 两处断言变红，修复后转绿。
- **复核补强（2026-10-26 评审）**：`example-lab-run` 从单路由 smoke 扩展为“构建树包 + 移动后
  四路由”，并断言日志无 `NanWindow: page error`、`resource lookup failed`、
  `texture decode/upload failed`、`[error]`（注入坏资源 key 时确认会变红）；SQL 查询补
  `ORDER BY` 并改用 `contextlib.closing` 关闭连接（Windows 上目录替换需要句柄先释放）；
  `lab-store` 不再依赖 `-Dexamples=true`，所有 UI 单元配置（含 sanitizer/no-RTTI/no-SQLite）
  都会覆盖；外部 `nanres` 的配置期握手新增 `keys` 子命令探测，缺失时在配置阶段失败；helper
  的相对路径在跨 Windows 盘符时回退为绝对路径；Notes 修复“fallback 选中但 store 未同步”的
  静默保存失败（保存未命中时明确反馈），demo 预填清除 invalid 标记；`PreviewPage` 拆出独立
  头文件，run 测试复用导出的 `stage_helper`。
- **验证**：默认（`examples=true`）主套件 **109/109**；release check 8/8；干净离线配置
  （`--wrap-mode=nodownload -Dexamples=true -Dshowcase=false -Dbuild_tests=false`）从零配置并
  定向构建 `nandina_lab` 通过；删除 `resource_keys.hpp` 与 `resources/` 后
  `meson compile nandina_lab` 重新生成常量头与包；无变化重建保持数据库 mtime；portable stage
  移动后四路由启动、退出码 0 且无错误日志。单元矩阵（`-Dexamples=false` 目录）：no-RTTI
  **85/85**、no-SQLite UI **82/82（+1 SKIP）**、ASan **84/84（+ `window-lifecycle` SKIP）**；
  纯工具 8/8。`lab-store` 随 `build_ui` 始终注册，因此这些配置也覆盖示例 store。完整示例编译
  另在 no-RTTI 目录以 `-Dexamples=true` 验证：7 个 TU 链接 `nandina_lab`，常量头与资源包均
  生成，单元计数不变；no-SQLite 目录是 headless（`sdl_backend=disabled`），按现有配置无法
  构建示例 app，只覆盖 `lab-store`。
- **Windows（2026-10-26，bundle 同步）**：设备恢复后按 AGENTS 流程复跑。`meson setup
  --reconfigure -Dexamples=true` 与 `ninja` 均 0 错误，生成 `nandina_lab.exe`、
  `resource_keys.hpp`（678 B）与 `resources/resources.db`（352256 B）；`--suite unit`
  **83/83**（`lab-store` 81/83 OK）；资源/示例集成 7 项中 **6 通过 + 1 SKIP**：
  `example-lab-build`、`example-lab-incremental`、`nanres-cli`、`nanres-build-workflow`、
  `portable-stage`、`nanres-keys` 全 OK，`example-lab-run` 因无 display 返回 77（SKIP）。
- **交互验收证据**：`docs/images/lab/` 收录 `notes-signed-in.png`（登录后 Notes 会话行与列表）、
  `notes-editor.png`（选中长文 + 保存/预览/删除）、`notes-dialog.png`（删除确认弹窗）、
  `focus-timer.png`（运行中的倒计时）、`preview.png`（资源包图片）。交互由合成指针事件
  （ydotool，本机绝对坐标按 2× 缩放折算）+ 截图采集，覆盖导航、笔记选择、新增/删除与弹窗确认、
  计时器启动/倒计时/跳过、登录→Notes 状态传递与预览加载；键盘文本输入仍未自动化，表单提交走
  demo 预填路径。真实 GPU 会话下运行，不构成页面 golden。
