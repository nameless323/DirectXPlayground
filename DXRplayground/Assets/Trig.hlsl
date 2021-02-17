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

float4 vs(vIn i) : SV_Position
{
    vOut o;
    o.pos = float4(i.pos.xy, 0.0f, 1.0f);
    o.ndcPos = o.pos;
    return o;
}

float4 ps(vOut i) : SV_Target
{
    return float4(i.xy, 0, 1);
}