#include "DXrenderer/LightManager.h"

#include "Utils/Helpers.h"
#include "DXrenderer/RenderContext.h"

namespace DirectxPlayground
{

LightManager::LightManager(RenderContext& ctx)
{
    m_lightsBuffer = new UploadBuffer(*ctx.Device, sizeof(m_lights), true, RenderContext::FramesCount);
}

LightManager::~LightManager()
{
    SafeDelete(m_lightsBuffer);
}

void LightManager::UpdateLights(UINT frame)
{
    m_lightsBuffer->UploadData(frame, m_lights);
}

}