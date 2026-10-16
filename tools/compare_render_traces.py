#!/usr/bin/env python3
"""tools/compare_render_traces.py — 对比两个后端的逐图元调用序列。

`render/recording_device.hpp` 把「同一份组件代码向设备要了什么」录成一行一次调用的文本，
于是跨后端的对照可以从「像素看起来差不多」升级成「图元构成与顺序是否相同」：

  * **像素相同** 可能是巧合（圆角画成方框时，圆角处的差异可能只有几个像素）；
  * **调用序列相同** 才能说明两边真的走了同一套绘制路径。

用法：
    tools/compare_render_traces.py <trace_a> <trace_b> [--frame-a N] [--frame-b M]

每份 trace 由 begin_frame/end_frame 切成帧；默认取**最后一个含绘制的帧**（稳态，跳过着色器/
图集首次上传）。比较两件事：

1. 图元直方图（每种调用各多少次），差异列出来；
2. 调用**名字序列**是否逐步一致，第一处分歧给出行号与上下文。

几何参数不参与比较：两个宿主的窗口尺寸/DPI 可能不同（Wayland 合成器会自行平铺窗口），
坐标必然不同，但对「用哪些图元画」没有影响。名字序列一致即说明图元层等价。
"""

from __future__ import annotations

import argparse
import sys
from collections import Counter
from pathlib import Path


def read_frames(path: Path) -> list[list[str]]:
    """按 begin_frame/end_frame 切帧；只保留**确实有绘制调用**的帧。

    构建期/首帧可能只清屏不画（页面尚未挂载、字体图集刚上传），把它们当成稳态帧会让
    直方图凭空多出/少掉一整个图元类别，所以这里以「本帧是否出现 draw_*」为准。"""
    frames: list[list[str]] = []
    current: list[str] | None = None
    with path.open("r", encoding="utf-8", errors="replace") as handle:
        for raw in handle:
            line = raw.rstrip("\n")
            if not line:
                continue
            name = line.split(" ", 1)[0]
            if name == "begin_frame":
                current = []
            elif name == "end_frame":
                if current is not None:
                    frames.append(current)
                current = None
            elif current is not None:
                current.append(line)
    # 宿主与场景树各会开一次帧边界，于是同一次循环里可能出现「只清屏、不绘制」的帧。
    # 把它并入紧随其后的绘制帧，两侧才是同一个逻辑帧（否则直方图会凭空多/少一次 clear）。
    merged: list[list[str]] = []
    pending: list[str] = []
    for frame in frames:
        if any(name.startswith("draw_") for name in names(frame)):
            merged.append(pending + frame)
            pending = []
        else:
            pending.extend(frame)
    return merged


def names(frame: list[str]) -> list[str]:
    return [line.split(" ", 1)[0] for line in frame]


def histogram(frame: list[str]) -> Counter[str]:
    return Counter(names(frame))


def first_divergence(a: list[str], b: list[str]) -> int | None:
    for index, (left, right) in enumerate(zip(a, b)):
        if left != right:
            return index
    return None if len(a) == len(b) else min(len(a), len(b))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("trace_a", type=Path)
    parser.add_argument("trace_b", type=Path)
    parser.add_argument("--frame-a", type=int, default=-1, help="取第 N 帧（默认最后一帧）")
    parser.add_argument("--frame-b", type=int, default=-1)
    args = parser.parse_args()

    frames_a = read_frames(args.trace_a)
    frames_b = read_frames(args.trace_b)
    if not frames_a or not frames_b:
        print("compare_render_traces: 至少一份 trace 里没有帧", file=sys.stderr)
        return 1

    frame_a = frames_a[args.frame_a]
    frame_b = frames_b[args.frame_b]
    print(f"A = {args.trace_a}  帧数={len(frames_a)}  本帧调用={len(frame_a)}")
    print(f"B = {args.trace_b}  帧数={len(frames_b)}  本帧调用={len(frame_b)}")
    print()

    hist_a = histogram(frame_a)
    hist_b = histogram(frame_b)
    all_names = sorted(set(hist_a) | set(hist_b))
    print(f"{'调用':<34}{'A':>8}{'B':>8}   差异")
    mismatched_kinds = 0
    for name in all_names:
        left = hist_a.get(name, 0)
        right = hist_b.get(name, 0)
        mark = ""
        if left != right:
            mark = "不同" if (left and right) else ("A 独有" if left else "B 独有")
            mismatched_kinds += 1
        print(f"{name:<34}{left:>8}{right:>8}   {mark}")
    print()

    seq_a = names(frame_a)
    seq_b = names(frame_b)
    if seq_a == seq_b:
        print(f"名字序列：完全一致（{len(seq_a)} 次调用，逐项相同）")
        return 0

    index = first_divergence(seq_a, seq_b)
    assert index is not None
    print(f"名字序列：第 {index} 次调用起分歧（A {len(seq_a)} 次 / B {len(seq_b)} 次）")
    for offset in range(max(0, index - 2), min(index + 3, max(len(seq_a), len(seq_b)))):
        left = seq_a[offset] if offset < len(seq_a) else "-"
        right = seq_b[offset] if offset < len(seq_b) else "-"
        marker = "  <<<" if offset == index else ""
        print(f"  [{offset:>5}] A={left:<30} B={right:<30}{marker}")
    print()
    print(f"调用种类不一致数：{mismatched_kinds}")
    # 图元种类不一致就是等价性缺口；序列分歧但种类一致通常只是布局/尺寸差异。
    return 2 if mismatched_kinds else 0


if __name__ == "__main__":
    raise SystemExit(main())
