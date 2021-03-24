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
    } m_lights;
    UploadBuffer* m_lightsBuffer = nullptr;
};

inline D3D12_GPU_VIRTUAL_ADDRESS LightManager::GetLightsBufferGpuAddress(UINT frame) const
{
    return m_lightsBuffer->GetFrameDataGpuAddress(frame);
}

inline UINT LightManager::AddLight(const Light& light)
{
    m_lights.Lights[m_lights.UsedLights] = light;
    return m_lights.UsedLights++;
}

inline Light& LightManager::GetLightRef(UINT ind)
{
    return m_lights.Lights[ind];
}

inline void LightManager::SetLight(UINT ind, const Light& light)
{
    m_lights.Lights[ind] = light;
}


inline Light* LightManager::GetLights()
{
    return m_lights.Lights;
}
}
