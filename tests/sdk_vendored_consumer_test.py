#!/usr/bin/env python3
"""SDK → 下游 vendored SDL 消费验收。

为什么单独一个 fixture：资源 fixture 用 `sdl_backend=disabled` 让时长可控，因此它只证明
"SDK 能打包、SDL 源码进了归档"。这里补的是真正的交付闭环：

  解压到全新目录 → 断网（`--wrap-mode=nodownload`）→ 默认启用 SDL 且 `sdl3_source=vendored`
  → 配置 / 编译 / 链接 → 运行 → 确认运行期加载的是构建树里的 SDL3 DSO。

依赖源码缺文件、wrap 无法消费、共享库 rpath 不对，都会让这个 fixture 失败；
而"资源 SDK 能构建"的用例不会覆盖这些。
"""

import json
import pathlib
import shutil
import subprocess
import sys
import tempfile


def run(
    *args: str,
    cwd: pathlib.Path,
    environment: dict[str, str] | None = None,
    capture: bool = False,
) -> str:
    process_environment = None
    if environment is not None:
        process_environment = {**dict(__import__("os").environ), **environment}
    result = subprocess.run(
        args,
        cwd=cwd,
        check=False,
        text=True,
        capture_output=True,
        env=process_environment,
    )
    assert result.returncode == 0, (
        f"{' '.join(args)} failed\n--- stdout ---\n{result.stdout}\n--- stderr ---\n{result.stderr}"
    )
    return result.stdout if capture else ""


def write_consumer(source: pathlib.Path, archive: pathlib.Path, sha256: str) -> None:
    (source / "subprojects").mkdir(parents=True)
    (source / "subprojects" / "nandina.wrap").write_text(
        "[wrap-file]\n"
        "directory = nandina-sdk-0.0.0-test\n"
        "source_url = https://invalid.example/nandina-sdk-0.0.0-test-source.tar.xz\n"
        f"source_filename = {archive.name}\n"
        f"source_hash = {sha256}\n",
        encoding="ascii",
    )
    # 默认后端必须是 SDL3，且来源固定为 SDK 内的 vendored 源码：这不是"能构建就行"，
    # 而是"SDK 交付的默认路径可用"。
    (source / "meson.build").write_text(
        "project('nandina_sdl_consumer', 'cpp')\n"
        "nandina = subproject('nandina', default_options: [\n"
        "  'build_tests=false',\n"
        "  'physics2d=disabled',\n"
        "  'sdl_backend=enabled',\n"
        "  'sdl3_source=vendored',\n"
        "])\n"
        "nandina_dep = nandina.get_variable('nandina_dep')\n"
        "executable('probe', 'main.cpp', dependencies: [nandina_dep])\n",
        encoding="ascii",
    )
    (source / "main.cpp").write_text(
        "#include <nandina/app/window_config.hpp>\n"
        "#include <SDL3/SDL.h>\n"
        "#include <cstdio>\n"
        "int main() {\n"
        "    const auto backend = nandina::app::default_window_backend();\n"
        "    const bool sdl = backend == nandina::app::WindowBackend::sdl3;\n"
        "    const int version = SDL_GetVersion();\n"
        "    std::printf(\"backend=%s sdl=%d.%d.%d\\n\", sdl ? \"sdl3\" : \"raylib\",\n"
        "        SDL_VERSIONNUM_MAJOR(version), SDL_VERSIONNUM_MINOR(version),\n"
        "        SDL_VERSIONNUM_MICRO(version));\n"
        "    return sdl ? 0 : 1;\n"
        "}\n",
        encoding="ascii",
    )


def main() -> int:
    nandina_source = pathlib.Path(sys.argv[1]).resolve()
    bundle_tool = pathlib.Path(sys.argv[2]).resolve()
    with tempfile.TemporaryDirectory(prefix="nandina-sdl-consumer-") as directory:
        root = pathlib.Path(directory)
        release = root / "release"
        run(
            sys.executable,
            str(bundle_tool),
            "--source",
            str(nandina_source),
            "--output",
            str(release),
            "--version",
            "0.0.0-test",
            "--repository",
            "fixture",
            "--allow-dirty",
            cwd=nandina_source,
            environment={"SOURCE_DATE_EPOCH": "315532800"},
        )
        manifest = json.loads((release / "sdk-manifest.json").read_text(encoding="utf-8"))
        dependencies = manifest["dependencies"]
        assert dependencies, "SDK manifest carries no dependency sources"
        # 迁移后依赖不止一个：按名字取，不能假设顺序。
        dependency = next(item for item in dependencies if item["name"] == "SDL3")
        assert all(item["matches_pin"] and not item["dirty"] for item in dependencies), dependencies
        assert dependency["matches_pin"] is True, dependency
        assert dependency["dirty"] is False, dependency
        assert len(dependency["revision"]) == 40, dependency

        tar_item = next(item for item in manifest["artifacts"] if item["format"] == "tar.xz")
        archive = release / tar_item["filename"]

        source = root / "source"
        build = root / "build"
        write_consumer(source, archive, tar_item["sha256"])
        package_cache = source / "subprojects" / "packagecache"
        package_cache.mkdir(parents=True)
        shutil.copy2(archive, package_cache / archive.name)

        # 断网：SDK 必须自带依赖源码，不能靠下载补齐。
        run("meson", "setup", str(build), "--wrap-mode=nodownload", cwd=source)
        run("meson", "compile", "-C", str(build), cwd=source)

        # 依赖报告：来源必须是 vendored，链接方式必须是共享库（静态 SDL3 与 raylib 冲突）。
        # 子项目构建目录名由 Meson 决定（wrap directory 与 subproject 名可能不同），按内容找。
        candidates = sorted(build.glob("subprojects/*/deps.json"))
        assert len(candidates) == 1, candidates
        deps_path = candidates[0]
        sdl3 = json.loads(deps_path.read_text(encoding="utf-8"))["deps"][0]
        assert sdl3["source"] == "vendored", sdl3
        assert sdl3["linkage"] == "shared", sdl3
        assert sdl3["resolved_by"] == "cmake.subproject", sdl3
        assert sdl3["revision"] == dependency["revision"], sdl3

        # 运行：默认后端必须是 SDL3，且运行期解析到 SDK 构建树里的 DSO。
        probe = build / "probe"
        output = run(str(probe), cwd=source, capture=True)
        assert "backend=sdl3" in output, output

        loader = run("ldd", str(probe), cwd=source, capture=True)
        assert "libSDL3" in loader, loader
        resolved = [line for line in loader.splitlines() if "libSDL3" in line]
        assert any(str(build) in line for line in resolved), resolved
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
