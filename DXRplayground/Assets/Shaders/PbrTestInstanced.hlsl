#include "Lighting.hlsl"

struct CbCamera
{
    float4x4 ViewProjection;
    float4x4 View;
    float3 Position;
    float Padding;
};
struct CbObject
{
    float4x4 ToWorld[100];
};

struct Material
{
    float4 Albedo;
    float Metallic;
    float Roughness;
    float AO;
    float Padding;
};
struct CbMaterial
{
    Material Materials[100];
};
struct Light
{
    float4 Color;
    float3 Position;
    float Padding;
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
    o.norm = mul(float4(normalize(i.norm), 0.0f), cbObject.ToWorld[ind]).xyz;
    o.tangent = i.tangent;
    o.uv = i.uv;
    o.instanceID = ind;
    return o;
}

float4 ps(vOut pIn) : SV_Target
{
    /*float3 tangent = normalize(i.tangent.xyz);
    float3 bitangent = cross(normal, tangent) * i.tangent.w;
    float3x3 tbn = float3x3(tangent, bitangent, normal);*/

    float instanceID = pIn.instanceID;
    float3 normal = normalize(pIn.norm);
    float3 wpos = pIn.wpos;
    float3 V = normalize(cbCamera.Position - wpos);

    float3 Lo = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < cbLight.UsedLights; ++i)
    {
        float3 lightPos = cbLight.Lights[i].Position;
        float3 L = normalize(lightPos - wpos);
        float3 H = normalize(L + V);

        float d = length(lightPos - wpos);
        float atten = 1.0f / (d * d);
        float3 radiance = cbLight.Lights[i].Color.xyz * atten;

        float metalness = cbMaterial.Materials[instanceID].Metallic;
        float3 f0 = NormalIncidenceFresnel(cbMaterial.Materials[instanceID].Albedo.xyz, metalness);
        // float cosTheta = max(dot(H, V), 0.0f);
        float3 F = FresnelSchlick(H, V, f0);
        float roughness = cbMaterial.Materials[instanceID].Roughness;

        float NDF = GGXDistribution(normal, H, roughness);
        float G = GeometrySmith(normal, V, L, roughness);

        float3 numer = NDF * G * F; 
        float denumer = 4.0f * max(dot(normal, V), 0.0f) * max(dot(normal, L), 0.0f);
        float3 spec = numer / max(denumer, 0.0001f);

        float3 ks = F;
        float3 kd = float3(1.0f, 1.0f, 1.0f) - ks;
        kd *= 1.0f - metalness;

        float NdotL = max(dot(normal, L), 0.0f);
        Lo += (kd * cbMaterial.Materials[instanceID].Albedo.xyz / PI + spec) * radiance * NdotL;

    }
    float3 ambient = float3(0.03f, 0.03f, 0.03f) * cbMaterial.Materials[instanceID].Albedo.xyz * cbMaterial.Materials[instanceID].AO;
    float3 color = ambient + Lo;

    return float4(color, 1.0f);
}