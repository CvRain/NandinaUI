// 生产 SDF 图元的 fragment shader（SDL_GPURenderState 路径）。
//
// 参数块是 3 个 float4（48 字节），由 SdlRenderDevice 以"不可变 state"缓存。
// 按 style.z（模式）区分编码，模式值与 sdf_primitive_geometry.hpp 的
// SdfPrimitiveMode 对齐：
//   0 填充 / 1 中心线描边：bounds.xy = 中心、bounds.zw = 半尺寸（目标像素），
//                           style.x = 圆角半径、style.y = 描边半宽
//   2 线段：               bounds.xy = A、bounds.zw = B - A（方向），style.x = 半宽
//   3 圆 ∩ 圆角矩形：      bounds.xy = 裁剪矩形中心、bounds.zw = 半尺寸，
//                           style.x = 裁剪圆角半径、style.y = 圆半径，
//                           extra.xy = 圆心（目标像素）
//   4 软阴影：             bounds.xy = 阴影矩形中心、bounds.zw = 半尺寸，
//                           style.x = 圆角半径、style.y = 软边衰减宽度（目标像素）
//   5 环形扇区：           bounds.xy = 圆心、bounds.zw = (内半径, 外半径)，
//                           style.x = 起始角 + 半张角、style.y = 半张角
// 角度约定与 IRenderDevice::draw_arc 一致：以 +Y 为 0、向 +X 增大。
// 描边语义与 render/backends/sdf_primitive_geometry.hpp 的
// sdf_inner_outline_geometry() 配套：组件边框先换算成"中心线 + 半宽"，这里画中心线描边。
// 颜色走顶点色；白色 1x1 纹理与 SDL 的 geometry 管线保持一致。

Texture2D source_texture : register(t0, space2);
SamplerState source_sampler : register(s0, space2);

cbuffer ShapeParameters : register(b0, space3) {
    float4 bounds;
    float4 style;
    float4 extra;
};

struct FragmentInput {
    float4 position : SV_Position;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

float sdSegment(float2 p, float2 a, float2 ba) {
    float2 pa = p - a;
    float h = saturate(dot(pa, ba) / max(dot(ba, ba), 1e-6));
    return length(pa - ba * h);
}

// 角向距离：把角差环绕到 [-pi, pi] 再减去半张角，乘以半径换算成近似欧氏距离。
// 不能用"两条半平面之交"的 sine 公式：sweep > pi 时它会退化成扇区补集
// （实测 270 度扇区整块画反），而 spinner/进度环经常大于半圈。
// p 为相对圆心向量；角度以 +Y 为 0、向 +X 增大（atan2(p.x, p.y)）。
float sdArcAngle(float2 p, float mid, float half_sweep) {
    float radius = length(p);
    float angle = atan2(p.x, p.y);
    float delta = angle - mid;
    delta = delta - 6.28318530718 * round(delta / 6.28318530718);
    return (abs(delta) - half_sweep) * max(radius, 1e-4);
}

float4 main(FragmentInput input) : SV_Target {
    float2 position = input.position.xy;
    float coverage;
    if (style.z > 4.5) {
        // 环形扇区：径向带 SDF 与角向扇区 SDF 求交。整环（sweep = 2π）没有角向
        // 边界，两条边界线重合会让 max 公式退化为 |sin|，必须跳过角向切割。
        float2 offset = position - bounds.xy;
        float mid = (bounds.z + bounds.w) * 0.5;
        float half_band = max((bounds.w - bounds.z) * 0.5, 0.0);
        float distance = abs(length(offset) - mid) - half_band;
        if (style.y < 3.14159265 - 1e-3) {
            distance = max(distance, sdArcAngle(offset, style.x, style.y));
        }
        float antialias_width = max(fwidth(distance), 1e-4);
        coverage = 1.0 - smoothstep(-antialias_width * 0.5, antialias_width * 0.5, distance);
    }
    else if (style.z > 3.5 && style.z < 4.5) {
        // 软阴影：圆角矩形 SDF，形状内部不透明，外部按 spread 平滑衰减到 0。
        // 没有 fwidth：柔化宽度由 spread 定义，不是 1px 抗锯齿带。
        float2 shadow_offset = abs(position - bounds.xy) - bounds.zw + style.x;
        float shadow_distance =
            min(max(shadow_offset.x, shadow_offset.y), 0.0) + length(max(shadow_offset, 0.0)) - style.x;
        coverage = 1.0 - smoothstep(0.0, max(style.y, 1e-4), shadow_distance);
    }
    else if (style.z > 2.5 && style.z < 3.5) {
        // 圆 ∩ 圆角矩形：两个有符号距离取 max 得到交集。圆角裁剪必须解析计算：
        // 用矩形裁剪近似会把圆角切成方角（ripple 溢出）。
        float2 clip_offset = abs(position - bounds.xy) - bounds.zw + style.x;
        float clip_distance =
            min(max(clip_offset.x, clip_offset.y), 0.0) + length(max(clip_offset, 0.0)) - style.x;
        float circle_distance = length(position - extra.xy) - style.y;
        float distance = max(clip_distance, circle_distance);
        float antialias_width = max(fwidth(distance), 1e-4);
        coverage = 1.0 - smoothstep(-antialias_width * 0.5, antialias_width * 0.5, distance);
    }
    else if (style.z > 1.5 && style.z < 2.5) {
        // 线段（含圆头）：half width 减在距离上，端点自然成为半圆。
        float distance = sdSegment(position, bounds.xy, bounds.zw) - style.x;
        float antialias_width = max(fwidth(distance), 1e-4);
        coverage = 1.0 - smoothstep(-antialias_width * 0.5, antialias_width * 0.5, distance);
    }
    else {
        // 有符号距离用原始填充距离求导：先 abs() 再求导会在轮廓中心线处得到零导数。
        float2 offset = abs(position - bounds.xy) - bounds.zw + style.x;
        float distance =
            min(max(offset.x, offset.y), 0.0) + length(max(offset, 0.0)) - style.x;
        float antialias_width = max(fwidth(distance), 1e-4);
        coverage = 1.0 - smoothstep(-antialias_width * 0.5, antialias_width * 0.5, distance);
        if (style.z > 0.5) {
            coverage = 1.0
                - smoothstep(
                    -antialias_width * 0.5,
                    antialias_width * 0.5,
                    abs(distance) - style.y
                );
        }
    }
    float4 color = input.color * source_texture.Sample(source_sampler, input.uv);
    return float4(color.rgb, color.a * coverage);
}
