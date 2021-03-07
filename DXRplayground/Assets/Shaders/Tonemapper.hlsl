#include "Lighting.hlsl"

Texture2D<float4> Textures[10000] : register(t0);

SamplerState LinearClampSampler : register(s0);

struct CbTonemapper
{
    uint HdrTexIndex;
    float Exposure;
};
ConstantBuffer<CbTonemapper> cbTonemapper : register(b0);

struct vIn
{
    float3 pos : POSITION;
    float3 norm : NORMAL;
    float2 uv : TEXCOORD0;
    float4 tangent : TANGENT0;
};

struct vOut
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

vOut vs(vIn i)
{
    vOut o;
    o.pos = float4(i.pos.xy, 0.0f, 1.0f);
    o.uv = o.pos.xy * 0.5f + 0.5f;
    o.uv.y = 1.0 - o.uv.y;
    return o;
}

float4 ps(vOut i) : SV_Target
{
    float4 hdrColor = Textures[cbTonemapper.HdrTexIndex].Sample(LinearClampSampler, i.uv);
    float4 mappedColor = float4(float3(1.0f, 1.0f, 1.0f) - exp(-hdrColor.xyz * cbTonemapper.Exposure), hdrColor.a);
    return RGBtosRGB(mappedColor);
}