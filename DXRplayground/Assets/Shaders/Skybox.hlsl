#include "Lighting.hlsl"

struct CbCamera
{
    float4x4 ViewProjection;
    float4x4 View;
    float4x4 Proj;
    float3 Position;
    float Padding;
};
ConstantBuffer<CbCamera> cbCamera : register(b0);
TextureCube<float4> Cubemaps[100] : register(t10000);
SamplerState LinearClampSampler : register(s0);

struct vIn
{
    float3 pos : POSITION;
};

struct vOut
{
    float4 pos : SV_Position;
    float3 lPos : TEXCOORD0;
};

vOut vs(vIn vin)
{
    vOut o;
    float4x4 camRot = cbCamera.View;
    camRot._m30_m31_m32 = float3(0.0f, 0.0f, 0.0f);

    float4 outPos = mul(float4(vin.pos.xyz, 1.0f), mul(camRot, cbCamera.Proj));
    o.pos = outPos.xyww;
    o.lPos = vin.pos.xyz;
    return o;
}

float4 ps(vOut pin) : SV_Target
{
    float4 col = Cubemaps[0].Sample(LinearClampSampler, normalize(pin.lPos));

    return col;
}