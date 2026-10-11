# SQLite amalgamation（vendored）

本目录是 SQLite 官方 amalgamation 的原样副本。它**不是** git submodule、Meson wrap
或 vcpkg port —— 见下面「为什么放在树里」。

## 为什么放在树里

`resources.db` 是框架自有的资源包格式，读写两侧都需要 SQLite。它同时被 Meson 构建
和规划中的 CMake 顶层构建消费，所以依赖来源必须只有一份：

- **Meson wrap** 只服务 Meson，换到 CMake 就要另找一条路；
- **vcpkg port** 只服务 CMake，Meson 侧同样落空；
- **git submodule** 要求每个消费者联网 clone 一个镜像仓（SQLite 上游主仓库是 Fossil
  而非 Git），而且指针语义比"两个源文件"更脆。

amalgamation 是 SQLite 的官方发行形式：`sqlite3.c` + `sqlite3.h`。放进树里，两套构建
系统编译**同一份源码、同一组编译选项**，不需要联网、不需要包管理器，也不需要在两处
各维护一次版本。

SQLite 处于 **public domain（公有领域）**：没有许可证文本、没有署名义务、没有
copyleft 或再分发限制，因此随源码分发不需要附加许可文件。仓库其余部分仍是 MIT。

## 版本与来源

| 项目 | 值 |
| --- | --- |
| SQLite 版本 | 3.54.0（`sqlite3.h` 的 `SQLITE_VERSION`） |
| 官方归档 | `sqlite-amalgamation-3540000.zip` |
| 来源 | <https://www.sqlite.org/2026/sqlite-amalgamation-3540000.zip> |
| 归档字节数 | 2990479 |
| SHA-256 | `68e913b0fe8ec6b4e5f391c594a28872884e80d6cd042a1e50c957be46b5aa4d` |
| SHA3-256 | `7b670a62fdfbd672b75fef004cb703c8a3e87d3a5cc7d675b4a08337004a2d93` |

SHA3-256 与 sqlite.org `download.html` 公布的值一致；该归档与 wrapdb 重新托管的副本
逐字节相同。`sqlite3.c`、`sqlite3.h`、`sqlite3ext.h` 从归档中**原样**取出，未做任何
本地修改；归档里的 `shell.c`（命令行工具）未纳入。`sqlite3ext.h` 只是可加载扩展的
接口头文件，当前编译选项中扩展加载被关闭，保留它是为了完整对应官方发行内容。

## 编译方式

编译选项与目标定义都在仓库根 `meson.build`（`sqlite3_c_args` / `sqlite3_dep`）。
由于 `sqlite3.c` 是 C 源码，根 `project()` 额外启用了 `c` 语言——任何提供 C++ 编译器
的工具链都同时提供 C 编译器，所以这不会增加额外的前置条件。

当前只有两条编译选项，都是刻意的：

- `SQLITE_THREADSAFE=1`：与运行时 `SQLITE_OPEN_FULLMUTEX` 的用法一致；
- `SQLITE_OMIT_LOAD_EXTENSION`：运行时打开的是可能随应用分发到用户磁盘的库文件，
  而框架从不加载扩展，因此在编译期关掉这条路径，而不是依赖运行时开关。

改动这组选项等于改动随包分发的引擎能力，需同时更新本节、根 `meson.build` 与 README
的依赖说明。

## 未来接 CMake 时

CMake 顶层构建落地后，对同一个 `sqlite3.c` 做 `add_library(sqlite3 STATIC ...)` 并施加
同一组 `target_compile_definitions` 即可。**不要**改回 `find_package(SQLite3)` 或
vcpkg port —— 那正是本目录要消除的"两套构建系统来源不同"。

## 升级步骤

1. 从 <https://www.sqlite.org/download.html> 取新的 `sqlite-amalgamation-<版本>.zip`；
2. 用 sqlite.org 公布的 SHA3-256 校验归档（`python3 -c "import hashlib,sys;
   print(hashlib.sha3_256(open(sys.argv[1],'rb').read()).hexdigest())" <zip>`）；
3. 原样替换本目录的 `sqlite3.c` / `sqlite3.h` / `sqlite3ext.h`；
4. 更新上面的版本表，并跑 `meson test -C buildDir`（`resource`、`nanres-cli`、
   `application-resource` 覆盖读写两侧与包完整性检查）。

`PRAGMA application_id` / `PRAGMA user_version` 是资源包的格式闸门（见
`nandina/resource/backends/sqlite_backend.hpp`）。SQLite 升级不改变该格式，但会改变
读写行为，因此升级必须走完整验证，而不是只跑一次编译。
