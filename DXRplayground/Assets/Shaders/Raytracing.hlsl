struct SceneRtData
{
    float4x4 invViewProj;
    float4 camPosition;
};

RaytracingAccelerationStructure Scene : register(t0);
RWTexture2D<float4> RenderTarget : register(u0);
ConstantBuffer<SceneRtData> SceneCb : register(b1);

typedef BuiltInTriangleIntersectionAttributes Attributes;
struct RayPayload
{
    float4 color;
}

void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
    float2 xy = index + 0.5f;
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0f - 1.0f;

    screenPos.y = -screenPos.y;

    float4 world = mul(float4(screenPos, 0.0f, 1.0f), SceneCb.invViewProj);

    world /= world.w;
    origin = SceneCb.camPosition.xyz;
    direction = normalize(world.xyz - origin);
}

[shader("raygeneration")]
void Raygen()
{
    float3 rayDir;
    float3 origin;

    GenerateCameraRay(DispatchRayIndex().xy, origin, rayDir);

    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = rayDir;

    ray.TMin = 0.0001f;
    ray.TMax = 10000.0f;

    RayPayload payload = { float4(0.0f, 0.0f, 0.0f, 0.0f) };
    TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);

    RenderTarget[DispatchRayIndex().xy] = payload.color;
}

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in Attributes attr)
{
    payload.color = float4(0.0f, 0.0f, 1.0f, 1.0f);
}

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.color = float4(1.0f, 0.0f, 0.0f, 1.0f);
}