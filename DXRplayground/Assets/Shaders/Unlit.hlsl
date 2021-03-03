struct CbCamera
{
    float4x4 ViewProjection;
};
struct CbObject
{
    float4x4 ToWorld;
};

ConstantBuffer<CbCamera> cbCamera : register(b0);
ConstantBuffer<CbObject> cbObject : register(b1);

Texture2D<float4> Textures[10000] : register(t0);
SamplerState LinearClampSampler : register(s0);

struct vIn
{
    float3 pos : POSITION;
    float3 norm : NORMAL;
    float2 uv : TEXCOORD0;
};

struct vOut
{
    float4 pos : SV_Position;
    float3 norm : NORMAL;
    float2 uv : TEXCOORD0;
};

vOut vs(vIn i)
{
    vOut o;
    float4 wPos = mul(float4(i.pos.xyz, 1.0f), cbObject.ToWorld);
    o.pos = mul(wPos, cbCamera.ViewProjection);
    o.norm = i.norm;
    o.uv = i.uv;
    return o;
}

float4 ps(vOut i) : SV_Target
{
    float4 t = Textures[0].Sample(LinearClampSampler, i.uv);
    return t;
    //return float4(i.norm.xyz * 0.5 + 0.5, 1);
}