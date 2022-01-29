#include "DXrenderer/LightManager.h"

#include "Utils/Helpers.h"
#include "DXrenderer/RenderContext.h"

namespace DirectxPlayground
{

LightManager::LightManager(RenderContext& ctx)
{
    mLightsBuffer = new UploadBuffer(*ctx.Device, sizeof(mLights), true, RenderContext::FramesCount);
}

LightManager::~LightManager()
{
    SafeDelete(mLightsBuffer);
}

void LightManager::UpdateLights(UINT frame)
{
    mLightsBuffer->UploadData(frame, mLights);
}

}