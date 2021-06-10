struct SceneRtData
{
    float4x4 invViewProj;
    float4 camPosition;
    float4 lightPosition;
};

RaytracingAccelerationStructure Scene : register(t0);
RWTexture2D<float4> RenderTarget : register(u0);
ConstantBuffer<SceneRtData> SceneCb : register(b0);

typedef BuiltInTriangleIntersectionAttributes Attributes;
struct RayPayload
{
    float4 color;
};

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

    GenerateCameraRay(DispatchRaysIndex().xy, origin, rayDir);

    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = rayDir;

    ray.TMin = 0.0001f;
    ray.TMax = 5000.0f;

    RayPayload payload = { float4(1.0f, 1.0f, 1.0f, 1.0f) };
    TraceRay(Scene, // What bvh
             RAY_FLAG_NONE, // optimization params https://docs.microsoft.com/en-us/windows/win32/direct3d12/ray_flag
             2, // Instance mask. 0xFF to test against everything. See D3D12_RAYTRACING_INSTANCE_DESC when creating blas
             0, 2, 0, // Shaders: HitGrop, NumHitGroups, MissShader. Hit group includes hit shader and geom. So for shadows one, for reflections another, primary rays another one
             ray,
             payload);

    RenderTarget[DispatchRaysIndex().xy] = payload.color;// float4(DispatchRaysIndex().xy, DispatchRaysDimensions().xy);// payload.color;
}

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in Attributes attr)
{
    float3 hitPoint = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 direction = normalize(SceneCb.lightPosition - hitPoint);
    RayDesc ray;
    ray.Origin = hitPoint;
    ray.Direction = direction;
    ray.TMin = 0.0001f;
    ray.TMax = 5000.0f;

    TraceRay(Scene,
             RAY_FLAG_NONE,
             0xFF,
             1, 2, 1,
             ray,
             payload);
}

[shader("anyhit")]
void AnyHit(inout RayPayload payload, in Attributes attr)
{
    payload.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
}

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
}

[shader("closesthit")]
void ShadowClosestHit(inout RayPayload payload, in Attributes attr)
{
    payload.color = float4(0.0f, 0.0f, 0.0f, 1.0f);
}

[shader("miss")]
void ShadowMiss(inout RayPayload payload)
{
    payload.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
}