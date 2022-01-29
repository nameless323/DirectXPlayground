#pragma once

#include "DXrenderer/Light.h"
#include "DXrenderer/Buffers/UploadBuffer.h"
#include "External/Dx12Helpers/d3dx12.h"

namespace DirectxPlayground
{
class UploadBuffer;
struct RenderContext;


class LightManager
{
public:
    static constexpr UINT MaxLightsCount = 128;

    LightManager(RenderContext& ctx);
    ~LightManager();

    UINT AddLight(const Light& light);
    Light& GetLightRef(UINT ind);
    void SetLight(UINT ind, const Light& light);
    Light* GetLights();

    void UpdateLights(UINT frame);

    D3D12_GPU_VIRTUAL_ADDRESS GetLightsBufferGpuAddress(UINT frame) const;

private:
    struct
    {
        Light Lights[MaxLightsCount] = {};
        UINT UsedLights = 0;
    } mLights;
    UploadBuffer* mLightsBuffer = nullptr;
};

inline D3D12_GPU_VIRTUAL_ADDRESS LightManager::GetLightsBufferGpuAddress(UINT frame) const
{
    return mLightsBuffer->GetFrameDataGpuAddress(frame);
}

inline UINT LightManager::AddLight(const Light& light)
{
    mLights.Lights[mLights.UsedLights] = light;
    return mLights.UsedLights++;
}

inline Light& LightManager::GetLightRef(UINT ind)
{
    return mLights.Lights[ind];
}

inline void LightManager::SetLight(UINT ind, const Light& light)
{
    mLights.Lights[ind] = light;
}


inline Light* LightManager::GetLights()
{
    return mLights.Lights;
}
}
