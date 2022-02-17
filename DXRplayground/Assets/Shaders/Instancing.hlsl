#ifndef INSTANCING_HLSL
#define INSTANCING_HLSL

#ifdef INSTANCING

static const uint INSTANCE_COUNT = 2;

struct CbInstance
{
    float4x4 ToWorld[INSTANCE_COUNT];
};


#else // INSTANCING

struct CbInstance
{
    float4x4 ToWorld;
};

#endif // # else INSTANCING

#define SV_INSTANCE uint instanceId : SV_InstanceID
ConstantBuffer<CbInstance> cbInstance : register(b1);

#ifdef INSTANCING
#define GET_TO_WORLD cbInstance.ToWorld[instanceId]
#else // INSTANCING
#define GET_TO_WORLD cbInstance.ToWorld;
#endif // # else INSTANCING

#endif