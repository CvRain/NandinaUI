#!/usr/bin/env bash
#
# tools/verify_shadercross.sh — 用 SDL_shadercross 实际生成**每个承诺的后端格式**并校验。
#
# 为什么单独一个脚本：契约（docs/references/platform_backend_contract.md §5 的 P1 验收）要求
# 「CI 需对每个承诺格式实际生成并**加载**」。生成这一步可以在一台 Linux 机器上做完，加载需要
# 各自的设备（D3D12 / Metal）。本脚本把「生成 + 能做的校验」固定下来，让 CI 或有对应平台的
# 机器只补最后一步。
#
# 用法：
#   tools/verify_shadercross.sh [shadercross 可执行文件]
# 默认在 playground/shadercross-build/ 下找（见下方 BUILD_HINT）。
#
# 退出码：0 = 全部格式生成成功且校验通过；1 = 有失败项。

set -uo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SHADERCROSS="${1:-${REPO_ROOT}/playground/shadercross-build/shadercross}"
SOURCE_HLSL="${REPO_ROOT}/probe/cross_platform/desktop/sdf.frag.hlsl"
OUT_DIR="${REPO_ROOT}/playground/shadercross-products"

BUILD_HINT="构建方法（需要 SDL3 >= 3.4.0）：
  git clone https://github.com/libsdl-org/SDL_shadercross
  cd SDL_shadercross && git checkout 1ff05bec573988a98ef9e0260b4da44f512b8367
  # DXC 的嵌套子模块：少了它就缺 d3d12shader.h
  git submodule update --init --recursive external/{SPIRV-Cross,SPIRV-Headers,SPIRV-Tools,DirectXShaderCompiler}
  cd external/DirectXShaderCompiler && git submodule update --init --depth 1 external/DirectX-Headers && cd ../..
  # vendored SPIRV-Tools 在 GCC 14 上会撞 -Werror=array-bounds 误报，放宽这一个警告
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DSDLSHADERCROSS_VENDORED=ON \\
    -DSDLSHADERCROSS_SPIRVCROSS_SHARED=OFF -DSDLSHADERCROSS_DXC=ON \\
    -DCMAKE_CXX_FLAGS='-Wno-error=array-bounds -Wno-error=stringop-overflow'
  cmake --build build --target shadercross"

failures=0
report() {
    local status="$1"
    local name="$2"
    local detail="$3"
    printf '%-5s %-26s %s\n' "${status}" "${name}" "${detail}"
    if [ "${status}" = "FAIL" ]; then
        failures=$((failures + 1))
    fi
}

if [ ! -x "${SHADERCROSS}" ]; then
    echo "verify_shadercross: 找不到可执行的 shadercross：${SHADERCROSS}" >&2
    echo "${BUILD_HINT}" >&2
    exit 1
fi
if [ ! -f "${SOURCE_HLSL}" ]; then
    echo "verify_shadercross: 找不到源 shader：${SOURCE_HLSL}" >&2
    exit 1
fi

mkdir -p "${OUT_DIR}"
echo "shadercross: ${SHADERCROSS}"
echo "source     : ${SOURCE_HLSL}"
echo "outputs    : ${OUT_DIR}"
echo

# 1) HLSL → SPIR-V（DXC 前端），再用**独立**的 spirv-val 校验 —— 这是本机能做的最强校验。
if "${SHADERCROSS}" "${SOURCE_HLSL}" -s HLSL -d SPIRV -t fragment \
    -o "${OUT_DIR}/sdf.frag.spv" >/dev/null 2>&1; then
    magic=$(xxd -p -l 4 "${OUT_DIR}/sdf.frag.spv" 2>/dev/null)
    if [ "${magic}" = "03022307" ]; then
        report OK "HLSL -> SPIR-V" "$(stat -c%s "${OUT_DIR}/sdf.frag.spv") B, magic ${magic}"
    else
        report FAIL "HLSL -> SPIR-V" "SPIR-V magic 不对：${magic}"
    fi
else
    report FAIL "HLSL -> SPIR-V" "生成失败"
fi

if command -v spirv-val >/dev/null 2>&1 && [ -f "${OUT_DIR}/sdf.frag.spv" ]; then
    if spirv-val "${OUT_DIR}/sdf.frag.spv" >/dev/null 2>&1; then
        report OK "spirv-val (external)" "shadercross 的 SPIR-V 产物通过独立校验"
    else
        report FAIL "spirv-val (external)" "独立校验拒绝了这个 SPIR-V 产物"
    fi
else
    report SKIP "spirv-val (external)" "系统没有 spirv-val"
fi

# 2) SPIR-V → DXIL（Windows / D3D12 的目标格式）。
if [ -f "${OUT_DIR}/sdf.frag.spv" ] \
    && "${SHADERCROSS}" "${OUT_DIR}/sdf.frag.spv" -o "${OUT_DIR}/sdf.frag.dxil" >/dev/null 2>&1; then
    magic=$(xxd -p -l 4 "${OUT_DIR}/sdf.frag.dxil" 2>/dev/null)
    if [ "${magic}" = "44584243" ]; then # "DXBC"
        report OK "SPIR-V -> DXIL" "$(stat -c%s "${OUT_DIR}/sdf.frag.dxil") B, DXBC container"
    else
        report FAIL "SPIR-V -> DXIL" "DXIL 容器 magic 不对：${magic}"
    fi
else
    report FAIL "SPIR-V -> DXIL" "生成失败"
fi

# 3) SPIR-V → MSL（macOS / Metal 的目标格式）。
if [ -f "${OUT_DIR}/sdf.frag.spv" ] \
    && "${SHADERCROSS}" "${OUT_DIR}/sdf.frag.spv" -o "${OUT_DIR}/sdf.frag.msl" >/dev/null 2>&1; then
    if grep -q "metal_stdlib" "${OUT_DIR}/sdf.frag.msl" 2>/dev/null; then
        report OK "SPIR-V -> MSL" "$(stat -c%s "${OUT_DIR}/sdf.frag.msl") B, metal_stdlib 入口"
    else
        report FAIL "SPIR-V -> MSL" "产物里没有 metal_stdlib"
    fi
else
    report FAIL "SPIR-V -> MSL" "生成失败"
fi

# 4) SPIR-V → 反解 HLSL（用于确认绑定/资源可还原）。
if [ -f "${OUT_DIR}/sdf.frag.spv" ] \
    && "${SHADERCROSS}" "${OUT_DIR}/sdf.frag.spv" -o "${OUT_DIR}/sdf.frag.hlsl" >/dev/null 2>&1; then
    if grep -qE "Texture2D|SamplerState" "${OUT_DIR}/sdf.frag.hlsl" 2>/dev/null; then
        report OK "SPIR-V -> HLSL" "$(stat -c%s "${OUT_DIR}/sdf.frag.hlsl") B, 资源绑定可见"
    else
        report FAIL "SPIR-V -> HLSL" "产物里没有资源绑定声明"
    fi
else
    report FAIL "SPIR-V -> HLSL" "生成失败"
fi

echo
echo "仍未验证：**加载** DXIL / MSL 需要 D3D12 / Metal 设备。"
echo "  - Metal：xcrun -sdk macosx metal -c ${OUT_DIR}/sdf.frag.msl -o /dev/null"
echo "  - DXIL ：dxc -dumpbin -verify ${OUT_DIR}/sdf.frag.dxil"
echo "这两条在 Linux/AMD（本项目当前唯一的验收机器）上跑不了，属于外部阻塞，不是"未做"。"

exit $((failures > 0 ? 1 : 0))
