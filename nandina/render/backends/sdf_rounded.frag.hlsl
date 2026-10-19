// 生产 SDF 图元的 fragment shader（SDL_GPURenderState 路径）。
//
// 参数块是 2 个 float4（32 字节），由 SdlRenderDevice 以"不可变 state"缓存：
//   bounds.xy = 图元中心、bounds.zw = 半尺寸（目标像素）
//   style.x   = 圆角半径、style.y = 描边半宽、style.z = 模式（0 填充 / 1 中心线描边）
// 描边语义与 render/backends/sdf_primitive_geometry.hpp 的
// sdf_inner_outline_geometry() 配套：组件边框先换算成"中心线 + 半宽"，这里画中心线描边。
// 颜色走顶点色；白色 1x1 纹理与 SDL 的 geometry 管线保持一致。

Texture2D source_texture : register(t0, space2);
SamplerState source_sampler : register(s0, space2);

cbuffer ShapeParameters : register(b0, space3) {
    float4 bounds;
    float4 style;
};

struct FragmentInput {
    float4 position : SV_Position;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

float4 main(FragmentInput input) : SV_Target {
    // 有符号距离用原始填充距离求导：先 abs() 再求导会在轮廓中心线处得到零导数。
    float2 offset = abs(input.position.xy - bounds.xy) - bounds.zw + style.x;
    float distance = min(max(offset.x, offset.y), 0.0)
        + length(max(offset, 0.0)) - style.x;
    float antialias_width = max(fwidth(distance), 1e-4);
    float coverage = 1.0 - smoothstep(-antialias_width * 0.5, antialias_width * 0.5, distance);
    if (style.z > 0.5) {
        coverage = 1.0 - smoothstep(-antialias_width * 0.5, antialias_width * 0.5, abs(distance) - style.y);
    }
    float4 color = input.color * source_texture.Sample(source_sampler, input.uv);
    return float4(color.rgb, color.a * coverage);
}
