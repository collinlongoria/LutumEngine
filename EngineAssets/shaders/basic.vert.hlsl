cbuffer FrameUniforms : register(b0, space1) {
    float4x4 viewProj;
};

struct VSInput {
    float3 position : TEXCOORD0;
    float2 uv       : TEXCOORD1;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

VSOutput main(VSInput input) {
    VSOutput o;
    o.position = mul(viewProj, float4(input.position, 1.0));
    o.uv = input.uv;
    return o;
}