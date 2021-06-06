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
    float3 norm : NORMAL;
    float2 uv : TEXCOORD0;
    float4 tangent : TANGENT0;
};

float4 vs(vIn i) : SV_Position
{
    float4 wPos = mul(float4(i.pos.xyz, 1.0f), cbObject.ToWorld);
    return mul(wPos, cbCamera.ViewProjection);
}

void ps()
{
}