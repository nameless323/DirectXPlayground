#include "Lighting.hlsl"

struct CbCamera
{
    float4x4 ViewProjection;
    float3 Position;
};
struct CbObject
{
    float4x4 ToWorld;
};

struct CbMaterial
{
    float4 Albedo;
    float Metallness;
    float Roughness;
    float AO;
    float pad;
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

struct CbShadowMap
{
    uint idx;
};

ConstantBuffer<CbCamera> cbCamera : register(b0);
ConstantBuffer<CbObject> cbObject : register(b1);
ConstantBuffer<CbMaterial> cbMaterial : register(b2);
ConstantBuffer<CbLight> cbLight : register(b3);
ConstantBuffer<CbShadowMap> cbShadowMap : register(b4);

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
    float3 norm : NORMAL;
    float2 uv : TEXCOORD0;
    float4 tangent : TANGENT0;
    float4 projPos : TEXCOORD2;
};

vOut vs(vIn i, uint ind : SV_InstanceID)
{
    vOut o;
    float4 wPos = mul(float4(i.pos.xyz, 1.0f), cbObject.ToWorld);
    o.wpos = wPos.xyz;
    o.pos = mul(wPos, cbCamera.ViewProjection);
    o.projPos = o.pos;
    o.norm = mul(float4(i.norm.xyz, 0.0f), cbObject.ToWorld);
    o.tangent = i.tangent;
    o.uv = i.uv;
    return o;
}

float4 ps(vOut pIn) : SV_Target
{
    float2 shadowUv = (pIn.projPos.xy / pIn.projPos.w) * 0.5f + 0.5f;
    shadowUv.y = 1.0f - shadowUv.y;
    float shadow = Textures[cbShadowMap.idx].Sample(LinearWrapSampler, shadowUv).x;

    float4 albedo = cbMaterial.Albedo;
    float metallness = cbMaterial.Metallness;
    float roughness = cbMaterial.Roughness;
    float3 N = normalize(pIn.norm);
    float AO = cbMaterial.AO;

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

        float3 f0 = NormalIncidenceFresnel(albedo.xyz, metallness);
        // float cosTheta = max(dot(H, V), 0.0f);
        float3 F = FresnelSchlick(H, V, f0);

        float NDF = GGXDistribution(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);

        float3 numer = NDF * G * F;
        float denumer = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f);
        float3 spec = numer / max(denumer, 0.0001f);

        float3 ks = F;
        float3 kd = float3(1.0f, 1.0f, 1.0f) - ks;
        kd *= 1.0f - metallness;

        float NdotL = max(dot(N, L), 0.0f);
        Lo += (kd * albedo.xyz / PI + spec) * radiance * NdotL;
    }
    float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo.xyz * AO;
    float3 color = ambient + Lo * shadow;

    return float4(color, 1.0f);
}