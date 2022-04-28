#include "Instancing.hlsl"

struct CbCamera
{
    float4x4 ViewProjection;
    float4x4 View;
    float4x4 Proj;
    float3 Position;
    float Padding;
};

ConstantBuffer<CbCamera> cbCamera : register(b0);
ConstantBuffer<CbObject> cbObject : register(b1);

struct vIn
{
    float3 pos : POSITION;
    float3 norm : NORMAL;
    float2 uv : TEXCOORD0;
    float4 tangent : TANGENT0;
};

float4 vs(vIn i, SV_INSTANCE) : SV_Position
{
    float4x4 toWorld = GET_TO_WORLD;
    float4 wPos = mul(float4(i.pos.xyz, 1.0f), GET_TO_WORLD);
    return mul(wPos, cbCamera.ViewProjection);
}

void ps()
{
}