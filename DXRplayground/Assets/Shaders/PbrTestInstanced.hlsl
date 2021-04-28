#include "Lighting.hlsl"

struct CbCamera
{
    float4x4 ViewProjection;
    float3 Position;
};
struct CbObject
{
    float4x4 ToWorld[100];
};

struct Material
{
    float4 DiffuseColor;
};
struct CbMaterial
{
    Material Materials[100];
};
struct Light
{
    float4 Color;
    float3 Direction;
};
struct CbLight
{
    Light Lights[128];
    uint UsedLights;
};

ConstantBuffer<CbCamera> cbCamera : register(b0);
ConstantBuffer<CbObject> cbObject : register(b1);
ConstantBuffer<CbMaterial> cbMaterial : register(b2);
ConstantBuffer<CbLight> cbLight : register(b3);

Texture2D<float4> Textures[10000] : register(t0);

SamplerState LinearClampSampler : register(s0);
SamplerState LinearWrapSampler : register(s1);

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
    float3 wpos : TEXCOORD1;
    nointerpolation float instanceID : TEXCOORD2;
    float3 norm : NORMAL;
    float2 uv : TEXCOORD0;
    float4 tangent : TANGENT0;
};

vOut vs(vIn i, uint ind : SV_InstanceID)
{
    vOut o;
    float4 wPos = mul(float4(i.pos.xyz * 0.5f, 1.0f), cbObject.ToWorld[ind]);
    o.wpos = wPos.xyz;
    o.pos = mul(wPos, cbCamera.ViewProjection);
    o.norm = mul(float4(normalize(i.pos.xyz), 0.0f), cbObject.ToWorld[ind]).xyz;
    o.tangent = i.tangent;
    o.uv = i.uv;
    o.instanceID = ind;
    return o;
}

float4 ps(vOut i) : SV_Target
{
    float3 normal = normalize(i.norm);
    float3 tangent = normalize(i.tangent.xyz);
    float3 bitangent = cross(normal, tangent) * i.tangent.w;
    float3x3 tbn = float3x3(tangent, bitangent, normal);

    return cbMaterial.Materials[i.instanceID].DiffuseColor;//float4(normal * 0.5f + 0.5f, 1.0f);
}