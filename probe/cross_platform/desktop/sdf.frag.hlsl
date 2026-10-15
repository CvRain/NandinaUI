Texture2D u_texture : register(t0, space2);
SamplerState u_sampler : register(s0, space2);

struct PSInput {
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

struct PSOutput {
    float4 color : SV_Target;
};

float sd_round_rect(float2 p, float2 half_size, float radius) {
    float2 q = abs(p) - half_size + radius;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - radius;
}

PSOutput main(PSInput input) {
    float4 source = u_texture.Sample(u_sampler, input.uv) * input.color;
    float2 p = input.uv * 2.0 - 1.0;
    float shadow_distance = sd_round_rect(p - float2(0.0, 0.12), float2(0.9, 0.72), 0.18);
    float shape_distance = sd_round_rect(p, float2(0.9, 0.72), 0.18);
    float aa = max(fwidth(shape_distance), 0.001);
    float shadow_alpha = (1.0 - smoothstep(-0.12, 0.12, shadow_distance)) * 0.3;
    float shape_alpha = 1.0 - smoothstep(-aa, aa, shape_distance);

    float4 result = float4(0.08, 0.09, 0.11, shadow_alpha);
    result.rgb = lerp(result.rgb, source.rgb, shape_alpha);
    result.a = max(result.a, shape_alpha * source.a);

    PSOutput output;
    output.color = result;
    return output;
}
