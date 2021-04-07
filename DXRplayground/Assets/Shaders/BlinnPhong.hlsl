#include "Lighting.hlsl"

float3 BlinnPhong(float3 lightDir, float3 lightColor, float3 eyePos, float3 pos, float3 n)
{
    float3 v = normalize(pos - eyePos);
    float3 h = normalize(-lightDir + v);
    float d = max(0.0f, dot(n, -lightDir));
    float s = pow(max(0.0f, dot(n, h)), 64);

    return lightColor * (d + s) +float3(0.1f, 0.1f, 0.1f);
}

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
    int BaseColorTexture;
    int MetallicRoughnessTexture;
    int NormalTexture;
    int OcclusionTexture;
    float4 BaseColorFactor;
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
struct CbDebug
{
    float Data[128];
};
ConstantBuffer<CbDebug> cbDebug : register(b0, space1);

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
    float3 norm : NORMAL;
    float2 uv : TEXCOORD0;
    float4 tangent : TANGENT0;
};

vOut vs(vIn i)
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

float4 ps(vOut i) : SV_Target
{
    float4 t = Textures[cbMaterial.BaseColorTexture].Sample(LinearWrapSampler, i.uv);
    float3 normal = normalize(i.norm);
    float3 tangent = normalize(i.tangent.xyz);
    float3 bitangent = cross(normal, tangent) * i.tangent.w;
    if (cbDebug.Data[0] > 0.0)
        bitangent *= -1.0f;
    float3x3 tbn = float3x3(tangent, bitangent, normal);

    float3 bumpNorm = Textures[cbMaterial.NormalTexture].Sample(LinearWrapSampler, i.uv).xyz * 2.0f - 1.0f;
    bumpNorm = normalize(mul(bumpNorm, tbn));

    float3 bp = BlinnPhong(cbLight.Lights[0].Direction, cbLight.Lights[0].Color, cbCamera.Position, i.wpos, bumpNorm);
    return sRGBtoRGB(t) * float4(bp, 1.0);
}