#!/usr/bin/env bash
#
# 跨平台后端探测的**工具链固定点**：一条命令打印本机实际版本，并校验 P1 门槛要求的那几项。
#
# 为什么需要它：探测依赖三类宿主工具（shader 编译器、SPIR-V 校验器、Emscripten），
# 它们的版本会决定产物格式与诊断质量。P1 的门槛明确要求「host tool 是构建机依赖：
# 固定版本、固定获取方式，不依赖开发机上刚好装好的工具」——所以版本必须写在一处、
# 可被一条命令核对，而不是散在 README 的叙述里。
#
# 用法：
#   probe/cross_platform/tools/verify_toolchain.sh            # 打印 + 校验必需项
#   EMSDK_DIR=<path> probe/cross_platform/tools/verify_toolchain.sh   # 顺带校验 emsdk
#
# 退出码：0 全部通过（可选项缺失只警告）；1 有必需项缺失或版本不符。

set -uo pipefail

# ---- 固定点（与 README、CMake 里的校验保持一致）--------------------------------
PINNED_GLSLC="2026.4"
PINNED_SPIRV_TOOLS="v2026.4"
PINNED_SDL_MIN="3.4.0"
PINNED_EMSCRIPTEN="6.0.10"

failures=0
warnings=0

report() { printf '  %-22s %s\n' "$1" "$2"; }
pass() { printf '  %-22s %s\n' "$1" "$2"; }
fail() {
    printf '  %-22s %s\n' "$1" "$2"
    failures=$((failures + 1))
}
warn() {
    printf '  %-22s %s\n' "$1" "$2"
    warnings=$((warnings + 1))
}

version_matches() { [[ "$1" == *"$2"* ]]; }

echo "== 必需：桌面 shader 链 =="
if command -v glslc >/dev/null 2>&1; then
    glslc_version="$(glslc --version 2>&1 | head -1)"
    if version_matches "$glslc_version" "$PINNED_GLSLC"; then
        pass "glslc" "$glslc_version"
    else
        fail "glslc" "$glslc_version (需要 $PINNED_GLSLC)"
    fi
else
    fail "glslc" "缺失"
fi

if command -v spirv-val >/dev/null 2>&1; then
    spirv_version="$(spirv-val --version 2>&1 | head -1)"
    if version_matches "$spirv_version" "$PINNED_SPIRV_TOOLS"; then
        pass "spirv-val" "$spirv_version"
    else
        fail "spirv-val" "$spirv_version (需要 SPIRV-Tools $PINNED_SPIRV_TOOLS)"
    fi
else
    fail "spirv-val" "缺失"
fi

echo
echo "== 必需：SDL3（pkg-config）=="
# 用 --atleast-version 而不是 `--exists "sdl3>=x"`：后者在本机 pkg-config 上恒为失败。
if command -v pkg-config >/dev/null 2>&1 && pkg-config --atleast-version="$PINNED_SDL_MIN" sdl3; then
    pass "sdl3" "$(pkg-config --modversion sdl3) (最低要求 $PINNED_SDL_MIN)"
else
    fail "sdl3" "缺失或低于 $PINNED_SDL_MIN"
fi

echo
echo "== 必需：构建器 =="
if command -v cmake >/dev/null 2>&1; then
    pass "cmake" "$(cmake --version | head -1)"
else
    fail "cmake" "缺失"
fi
if command -v ninja >/dev/null 2>&1; then
    pass "ninja" "$(ninja --version)"
else
    warn "ninja" "缺失（没有它就用不了 -G Ninja）"
fi

echo
echo "== 可选：Web 路径（emsdk）=="
emsdk_dir="${EMSDK_DIR:-}"
if [[ -z "$emsdk_dir" ]]; then
    for candidate in \
        "$(dirname "$0")/../../../playground/sdl_gpu_probe/emsdk" \
        "$HOME/emsdk" \
        /opt/emsdk; do
        if [[ -f "$candidate/emsdk_env.sh" ]]; then
            emsdk_dir="$candidate"
            break
        fi
    done
fi
if [[ -n "$emsdk_dir" && -f "$emsdk_dir/emsdk_env.sh" ]]; then
    # shellcheck disable=SC1091
    emcc_path="$emsdk_dir/upstream/emscripten/emcc"
    if [[ -x "$emcc_path" ]]; then
        emcc_version="$("$emcc_path" --version 2>&1 | head -1)"
        if version_matches "$emcc_version" "$PINNED_EMSCRIPTEN"; then
            pass "emscripten" "$emcc_version"
        else
            warn "emscripten" "$emcc_version (探测记录写的是 $PINNED_EMSCRIPTEN)"
        fi
        pass "emsdk 路径" "$emsdk_dir"
    else
        warn "emscripten" "$emsdk_dir 下找不到 emcc"
    fi
else
    warn "emsdk" "未找到（Web 路径需要它；设 EMSDK_DIR 指定）"
fi

echo
echo "== 可选：SDL_shadercross（P1 的多后端产物候选）=="
if command -v shadercross >/dev/null 2>&1; then
    pass "shadercross" "$(command -v shadercross)"
else
    warn "shadercross" "未安装：DXIL/MSL 产物仍未验证（P1 门槛项）"
fi

echo
if [[ "$failures" -eq 0 ]]; then
    echo "工具链检查通过（警告 $warnings 项）。"
    exit 0
fi
echo "工具链检查失败：$failures 项必需依赖不满足（警告 $warnings 项）。"
exit 1
