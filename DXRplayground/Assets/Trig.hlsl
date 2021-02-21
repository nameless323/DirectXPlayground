struct CbCamera
{
    float4x4 ViewProjection;
};

ConstantBuffer<CbCamera> cbCamera : register(b0);

struct vIn
{
    float3 pos : POSITION;
    float3 norm : NORMAL;
    float2 uv : TEXCOORD0;
};

struct vOut
{
    float4 pos : SV_Position;
    float3 ndcPos : TEXCOORD0;
};

vOut vs(vIn i)
{
    vOut o;
    o.pos = mul(float4(i.pos.xy, 3.0f, 1.0f), cbCamera.ViewProjection);
    o.ndcPos = o.pos;
    return o;
}

float4 ps(vOut i) : SV_Target
{
    return float4(i.ndcPos.xy * 0.5 + 0.5, 0, 1);
}