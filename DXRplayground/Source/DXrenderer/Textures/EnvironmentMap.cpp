#include "DXrenderer/Textures/EnvironmentMap.h"

#include <numeric>

#include "DXrenderer/Buffers/UploadBuffer.h"
#include "DXrenderer/Shader.h"
#include "DXrenderer/PsoManager.h"
#include "Utils/Helpers.h"

namespace DirectxPlayground
{

EnvironmentMap::EnvironmentMap(RenderContext& ctx, const std::string& path, UINT width, UINT height)
{
    CreateCommonRootSignature(ctx.Device, IID_PPV_ARGS(&m_commonRootSig));
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = GetDefaultComputePsoDescriptor(m_commonRootSig.Get());
    auto shaderPath = ASSETS_DIR_W + std::wstring(L"Shaders//EnvMapConvertion.hlsl");
    ctx.PsoManager->CreatePso(ctx, m_psoName, shaderPath, desc);

    m_buffer = new UnorderedAccessBuffer(ctx.CommandList, *ctx.Device, UINT(100 * sizeof(int)));
}

EnvironmentMap::~EnvironmentMap()
{
    SafeDelete(m_buffer);
}

void EnvironmentMap::ConvertToCubemap(RenderContext& ctx)
{
    ctx.CommandList->SetComputeRootSignature(m_commonRootSig.Get());
    ctx.CommandList->SetPipelineState(ctx.PsoManager->GetPso(m_psoName));
    ctx.CommandList->SetComputeRootUnorderedAccessView(GetUARootParamIndex(0), m_buffer->GetGpuAddress());

    ctx.CommandList->Dispatch(10, 1, 1);
}

bool EnvironmentMap::IsConvertedToCubemap() const
{
    return m_converted;
}

}