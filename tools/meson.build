# D3 项目级 CLI：声明在 tools 子目录，让构建产物落到 buildDir/tools/，
# 避免与 nandina 库目录（buildDir/nandina/）同名冲突。
nandina_cli_config = configuration_data()
# Windows 的源目录是 `D:\playground\NandinaUI`：直接写进 C 字符串会产生 `\p`/`\N` 这类
# 非法转义。统一成正斜杠（std::filesystem::path 在 Windows 上也接受）。
nandina_cli_config.set_quoted(
    'NANDINA_SOURCE_ROOT',
    nandina_source_root.replace('\\', '/'),
)
configure_file(
    output: 'nandina_cli_config.hpp',
    configuration: nandina_cli_config,
)

nandina = executable(
    'nandina',
    ['nandina_main.cpp'],
    include_directories: include_directories('.'),
)
