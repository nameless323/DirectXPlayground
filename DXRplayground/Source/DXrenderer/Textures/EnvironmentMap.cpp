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

    m_envMapIndex = ctx.TexManager->CreateTexture(ctx, path, true);
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
    ctx.CommandList->SetComputeRootDescriptorTable(UAVTableIndex, ctx.TexManager->GetCubemapUAVHeap()->GetGPUDescriptorHandleForHeapStart());

    ctx.CommandList->Dispatch(m_cubemapWidth / 32, m_cubemapHeight / 32, 6);
}

bool EnvironmentMap::IsConvertedToCubemap() const
{
    return m_converted;
}

void EnvironmentMap::CreateRootSig(RenderContext& ctx)
{
    std::vector<CD3DX12_ROOT_PARAMETER1> params;
    params.emplace_back();
    params.back().InitAsConstantBufferView(0, 0);

    D3D12_DESCRIPTOR_RANGE1 envMap{};
    envMap.NumDescriptors = 1;
    envMap.BaseShaderRegister = 0;
    envMap.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    envMap.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE; // Double check perf
    envMap.RegisterSpace = 0;
    envMap.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    params.emplace_back();
    params.back().InitAsDescriptorTable(1, &envMap, D3D12_SHADER_VISIBILITY_ALL);

    D3D12_DESCRIPTOR_RANGE1 cubeMap{};
    cubeMap.NumDescriptors = 1;
    cubeMap.BaseShaderRegister = 0;
    cubeMap.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    cubeMap.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE; // Double check perf
    cubeMap.RegisterSpace = 0;
    cubeMap.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    params.emplace_back();
    params.back().InitAsDescriptorTable(1, &cubeMap, D3D12_SHADER_VISIBILITY_ALL);

    CD3DX12_STATIC_SAMPLER_DESC linearClamp(
        0,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    std::array<CD3DX12_STATIC_SAMPLER_DESC, 1> staticSamplers = { linearClamp };

    D3D12_FEATURE_DATA_ROOT_SIGNATURE signatureData = {};
    signatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    if (FAILED(ctx.Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &signatureData, sizeof(signatureData))))
        signatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;

    D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(UINT(params.size()), params.data(), UINT(staticSamplers.size()), staticSamplers.data(), flags);

    Microsoft::WRL::ComPtr<ID3DBlob> signature;
    Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureCreationError;
    HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, signatureData.HighestVersion, &signature, &rootSignatureCreationError);
    if (hr != S_OK)
    {
        OutputDebugStringA(reinterpret_cast<char*>(rootSignatureCreationError->GetBufferPointer()));
        return;
    }
    ctx.Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSig));
}

}