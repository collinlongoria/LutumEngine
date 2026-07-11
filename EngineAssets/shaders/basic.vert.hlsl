cbuffer ObjectUniforms : register(b0, space1) {
    float4x4 viewProj;
    float4x4 model;
};

struct VSInput {
    float3 position : TEXCOORD0;
    float3 normal   : TEXCOORD1;
    float2 uv       : TEXCOORD2;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 uv       : TEXCOORD1;
};

VSOutput main(VSInput input) {
    VSOutput o;
    o.position = mul(viewProj, mul(model, float4(input.position, 1.0)));
    o.uv = input.uv;
    return o;
}