#include "DXrenderer/Textures/EnvironmentMap.h"

#include "DXrenderer/Buffers/UploadBuffer.h"
#include "DXrenderer/Shader.h"
#include "DXrenderer/PsoManager.h"
#include "Utils/Helpers.h"

namespace DirectxPlayground
{

EnvironmentMap::EnvironmentMap(RenderContext& ctx, const std::string& path, UINT width, UINT height)
    : m_cubemapWidth(width)
    , m_cubemapHeight(height)
{
    CreateCommonRootSignature(ctx.Device, IID_PPV_ARGS(&m_commonRootSig));
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = GetDefaultComputePsoDescriptor(m_commonRootSig.Get());

    auto shaderPath = ASSETS_DIR_W + std::wstring(L"Shaders//EnvMapConvertion.hlsl");
    ctx.PsoManager->CreatePso(ctx, m_psoName, shaderPath, desc);

    m_envMapIndex = ctx.TexManager->CreateTexture(ctx, path);
    m_cubemapIndex = ctx.TexManager->CreateCubemap(ctx, m_cubemapWidth, m_cubemapHeight, DXGI_FORMAT_R32G32B32A32_FLOAT, true);

    m_texturesIndices.CubemapIndex = m_cubemapIndex.UAVOffset;
    m_texturesIndices.EnvMapIndex = m_envMapIndex.SRVOffset;

    m_indicesBuffer = new UploadBuffer(*ctx.Device, sizeof(m_texturesIndices), true, 1);
    m_indicesBuffer->UploadData(0, m_texturesIndices);
}

EnvironmentMap::~EnvironmentMap()
{
    SafeDelete(m_indicesBuffer);
}

void EnvironmentMap::ConvertToCubemap(RenderContext& ctx)
{
    ctx.CommandList->SetComputeRootSignature(m_commonRootSig.Get());
    ctx.CommandList->SetPipelineState(ctx.PsoManager->GetPso(m_psoName));

    ID3D12DescriptorHeap* descHeaps[] = { ctx.TexManager->GetDescriptorHeap() };
    ctx.CommandList->SetDescriptorHeaps(1, descHeaps);

    ctx.CommandList->SetComputeRootConstantBufferView(GetCBRootParamIndex(0), m_indicesBuffer->GetFrameDataGpuAddress(0));
    ctx.CommandList->SetComputeRootDescriptorTable(TextureTableIndex, ctx.TexManager->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

    descHeaps[0] = { ctx.TexManager->GetCubemapUAVHeap() };
    ctx.CommandList->SetDescriptorHeaps(1, descHeaps);
    ctx.CommandList->SetComputeRootDescriptorTable(UAVCubemapTableIndex, ctx.TexManager->GetCubemapUAVHeap()->GetGPUDescriptorHandleForHeapStart());

    ctx.CommandList->Dispatch(m_cubemapWidth / 32, m_cubemapHeight / 32, 6);
}

bool EnvironmentMap::IsConvertedToCubemap() const
{
    return m_converted;
}

}