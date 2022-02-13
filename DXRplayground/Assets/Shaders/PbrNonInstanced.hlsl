#include "Lighting.hlsl"

struct CbCamera
{
    float4x4 ViewProjection;
    float4x4 View;
    float4x4 Proj;
    float3 Position;
    float Padding;
};
struct CbObject
{
    float4x4 ToWorld;
};

struct CbMaterial
{
    int BaseColorTexture;
    int MetallicRoughnessTexture;
    int NormalTexture;
    int OcclusionTexture;
    float4 BaseColorFactor;
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
struct CbCubemaps
{
    uint CubemapIndex;
    uint IrradianceMapIndex;
    uint pad0;
    uint pad1;
};

ConstantBuffer<CbCamera> cbCamera : register(b0);
ConstantBuffer<CbObject> cbObject : register(b1);
ConstantBuffer<CbMaterial> cbMaterial : register(b2);
ConstantBuffer<CbLight> cbLight : register(b3);
ConstantBuffer<CbCubemaps> cbCubemaps : register(b5);

Texture2D<float4> Textures[10000] : register(t0);
TextureCube<float4> Cubemaps[100] : register(t10000);

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
};

vOut vs(vIn i, uint ind : SV_InstanceID)
{
    vOut o;
    float4 wPos = mul(float4(i.pos.xyz, 1.0f), cbObject.ToWorld);
    o.wpos = wPos.xyz;
    o.pos = mul(wPos, cbCamera.ViewProjection);
    o.norm = i.norm;
    o.tangent = i.tangent;
    o.uv = i.uv;
    return o;
}

float4 ps(vOut pIn) : SV_Target
{
    float4 albedo = Textures[cbMaterial.BaseColorTexture].Sample(LinearWrapSampler, pIn.uv);
    albedo = sRGBtoRGB(albedo);

    float2 metalnessRoughness = Textures[cbMaterial.MetallicRoughnessTexture].Sample(LinearWrapSampler, pIn.uv).xy;
    float AO = Textures[cbMaterial.OcclusionTexture].Sample(LinearWrapSampler, pIn.uv).x;

    float3 normal = normalize(pIn.norm);
    float3 tangent = normalize(pIn.tangent.xyz);
    float3 bitangent = cross(normal, tangent) * pIn.tangent.w;
    float3x3 tbn = float3x3(tangent, bitangent, normal);

    float3 bumpNorm = Textures[cbMaterial.NormalTexture].Sample(LinearWrapSampler, pIn.uv).xyz * 2.0f - 1.0f;
    float3 N = normalize(mul(bumpNorm, tbn));

    float3 wpos = pIn.wpos;
    float3 V = normalize(cbCamera.Position - wpos);

    float3 Lo = float3(0.0f, 0.0f, 0.0f);
    float3 f0 = NormalIncidenceFresnel(albedo.xyz, metalnessRoughness.x);
    for (uint i = 0; i < cbLight.UsedLights; ++i)
    {
        float3 lightPos = cbLight.Lights[i].Position;
        float3 L = normalize(lightPos - wpos);
        float3 H = normalize(L + V);

        float d = length(lightPos - wpos);
        float atten = 1.0f / (d * d);
        float3 radiance = cbLight.Lights[i].Color.xyz * atten;

        // float cosTheta = max(dot(H, V), 0.0f);
        float3 F = FresnelSchlick(H, V, f0);

        float NDF = GGXDistribution(N, H, metalnessRoughness.y);
        float G = GeometrySmith(N, V, L, metalnessRoughness.y);

        float3 numer = NDF * G * F; 
        float denumer = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f);
        float3 spec = numer / max(denumer, 0.0001f);

        float3 ks = F;
        float3 kd = float3(1.0f, 1.0f, 1.0f) - ks;
        kd *= 1.0f - metalnessRoughness.x;

        float NdotL = max(dot(N, L), 0.0f);
        Lo += (kd * albedo.xyz / PI + spec) * radiance * NdotL;

    }
    float3 ks = FresnelSchlick(max(dot(N, V), 0.0), f0);
    float3 kd = 1.0f - ks;
    float3 irr = Cubemaps[cbCubemaps.IrradianceMapIndex].Sample(LinearClampSampler, N).xyz;
    float3 ambient = irr * albedo.xyz * kd * AO;
    float3 color = ambient + Lo;

    return float4(color, 1.0f);
}