#ifndef INSTANCING_HLSL
#define INSTANCING_HLSL

#ifdef INSTANCING

static const uint INSTANCE_COUNT = 2;

struct CbInstance
{
    float4x4 ToWorld[INSTANCE_COUNT];
};

#define SV_INSTANCE , uint instanceId : SV_InstanceID

#else // INSTANCING

struct CbInstance
{
    float4x4 ToWorld;
};

#define SV_INSTANCE 

#endif // # else INSTANCING

ConstantBuffer<CbInstance> cbInstance : register(b1);

#ifdef INSTANCING
#define GET_TO_WORLD cbInstance.ToWorld[instanceId]
#else // INSTANCING
#define GET_TO_WORLD cbInstance.ToWorld;
#endif // # else INSTANCING

#endif