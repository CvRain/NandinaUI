Texture2D source_texture : register(t0, space2);
SamplerState source_sampler : register(s0, space2);

cbuffer ShapeParameters : register(b0, space3) {
    float4 parameters[PARAMETER_VECTORS];
};

struct FragmentInput {
    float4 position : SV_Position;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

float4 main(FragmentInput input) : SV_Target {
    float4 bounds = parameters[0];
    float4 style = parameters[1];
    float2 offset = abs(input.position.xy - bounds.xy) - bounds.zw + style.x;
    float distance = min(max(offset.x, offset.y), 0.0)
        + length(max(offset, 0.0)) - style.x;
    float antialias_width = max(fwidth(distance), 0.0001);
    if (style.z > 0.5) {
        distance = abs(distance) - style.y;
    }
    float coverage = 1.0 - smoothstep(-antialias_width, antialias_width, distance);
    float4 color = source_texture.Sample(source_sampler, input.uv) * input.color;
    color *= parameters[2] * parameters[(uint)style.w];
    return float4(color.rgb, color.a * coverage);
}
