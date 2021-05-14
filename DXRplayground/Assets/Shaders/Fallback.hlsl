struct CbCamera
{
    float4x4 ViewProjection;
    float3 Position;
};
struct CbObject
{
    float4x4 ToWorld;
};

ConstantBuffer<CbCamera> cbCamera : register(b0);
ConstantBuffer<CbObject> cbObject : register(b1);

struct vIn
{
    float3 pos : POSITION;
};

struct vOut
{
    float4 pos : SV_Position;
};

vOut vs(vIn i)
{
    vOut o;
    float4 wPos = mul(float4(i.pos.xyz, 1.0f), cbObject.ToWorld);
    o.pos = mul(wPos, cbCamera.ViewProjection);
    return o;
}

float4 ps(vOut i) : SV_Target
{
    return float4(1, 0, 1, 1);
}

[numthreads(1, 1, 1)]
void cs()
{
}