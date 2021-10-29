#include "DXrenderer/Textures/MipGenerator.h"

#include "DXrenderer/RenderContext.h"
#include "DXrenderer/DXhelpers.h"
#include "DXrenderer/PsoManager.h"
#include "DXrenderer/Textures/TextureManager.h"
#include "DXrenderer/Buffers/UploadBuffer.h"
#include "DXrenderer/RenderPipeline.h"

#include "Utils/PixProfiler.h"

namespace DirectxPlayground
{
// [a_vorontcov] TODO: create pairing UAV resource, no need for ALLOW_UAV flag for every texture. Also too many descs and buffers.
MipGenerator::MipGenerator(RenderContext& ctx)
{
    CreateUavHeap(ctx);

    CreateCommonRootSignature(ctx.Device, IID_PPV_ARGS(&m_commonRootSig)); // [a_vorontcov] TODO: Move it goddamn
    CreatePso(ctx);

    m_constantBuffers = new UploadBuffer(*ctx.Device, sizeof(MipGenData), true, m_maxTextureMipsInFrame);
    m_queuedMips.reserve(m_maxTextureMipsInFrame);
    m_resourcesPtrs.reserve(m_maxTextureMipsInFrame);
}

void MipGenerator::GenerateMips(RenderContext& ctx, ResourceDX* resource)
{
    UINT baseMip = CreateMipViews(ctx, resource);
    UINT halfWidth = static_cast<UINT>(resource->Get()->GetDesc().Width) / 2;
    UINT halfHeight = resource->Get()->GetDesc().Height / 2;
    for (UINT i = 0; i < resource->Get()->GetDesc().MipLevels - 1U; ++i)
    {
        MipGenData mipData{};
        mipData.BaseMip = baseMip + i;
        mipData.Width = halfWidth;
        mipData.Height = halfHeight;
        halfWidth /= 2;
        halfHeight /= 2;

        m_constantBuffers->UploadData(m_queuedMipsToGenerateNumber++, mipData);
        assert(m_queuedMipsToGenerateNumber < m_maxTextureMipsInFrame);
        m_queuedMips.push_back(std::move(mipData));
    }
    m_resourcesPtrs.push_back(resource);
}

void MipGenerator::Flush(RenderContext& ctx)
{
    if (m_queuedMipsToGenerateNumber == 0)
        return;
    ctx.CommandList->SetComputeRootSignature(m_commonRootSig.Get());
    ctx.CommandList->SetPipelineState(ctx.PsoManager->GetPso(m_psoName));

    ID3D12DescriptorHeap* descHeaps[] = { m_uavHeap.Get() };
    ctx.CommandList->SetDescriptorHeaps(1, descHeaps);
    ctx.CommandList->SetComputeRootDescriptorTable(UAVTableIndex, m_uavHeap->GetGPUDescriptorHandleForHeapStart());

    std::vector<CD3DX12_RESOURCE_BARRIER> barriers;
    barriers.reserve(m_resourcesPtrs.size());
    for (size_t i = 0; i < m_resourcesPtrs.size(); ++i)
        barriers.push_back(m_resourcesPtrs[i]->GetBarrier(D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
    ctx.CommandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    barriers.clear();

    UINT currTexInd = 0;
    ID3D12Resource* currTex = m_resourcesPtrs[currTexInd++]->Get();
    UINT dispatchesLeft = currTex->GetDesc().MipLevels - 1;
    for (UINT i = 0; i < m_queuedMipsToGenerateNumber; ++i)
    {
        ctx.CommandList->SetComputeRootConstantBufferView(GetCBRootParamIndex(0), m_constantBuffers->GetFrameDataGpuAddress(i));
        ctx.CommandList->Dispatch(DISPATCH_TG_COUNT_2D(m_queuedMips[i].Width, 32, m_queuedMips[i].Height, 32));
        ctx.CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(currTex));

        --dispatchesLeft;
        if (dispatchesLeft == 0 && i < m_queuedMipsToGenerateNumber - 1)
        {
            currTex = m_resourcesPtrs[currTexInd++]->Get();
            dispatchesLeft = currTex->GetDesc().MipLevels - 1;
        }
    }
    for (size_t i = 0; i < m_resourcesPtrs.size(); ++i)
        barriers.push_back(m_resourcesPtrs[i]->GetBarrier(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
    ctx.CommandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

    m_queuedMipsToGenerateNumber = 0;
    m_currViewsCount = 0;
    m_queuedMips.clear();
    m_resourcesPtrs.clear();
    ctx.Pipeline->Flush();
}

void MipGenerator::CreateUavHeap(RenderContext& ctx)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = RenderContext::MaxUAVTextures;
    ThrowIfFailed(ctx.Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_uavHeap)));
    NAME_D3D12_OBJECT(m_uavHeap, L"Mips heap");
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_uavHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {}; // TODO: to helpers as GetNullUAV(SRV)ViewDesc or smth
    viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    viewDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

    for (UINT i = 0; i < RenderContext::MaxUAVTextures; ++i)
    {
        ctx.Device->CreateUnorderedAccessView(nullptr, nullptr, &viewDesc, handle);
        handle.Offset(ctx.CbvSrvUavDescriptorSize);
    }
}

void MipGenerator::CreatePso(RenderContext& ctx)
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = GetDefaultComputePsoDescriptor(m_commonRootSig.Get());

    auto shaderPath = ASSETS_DIR_W + std::wstring(L"Shaders//MipGenerator.hlsl");
    ctx.PsoManager->CreatePso(ctx, m_psoName, shaderPath, desc);
}

UINT MipGenerator::CreateMipViews(RenderContext& ctx, ResourceDX* resource)
{
    UINT baseOffset = m_currViewsCount;
    UINT mipsCount = resource->Get()->GetDesc().MipLevels;
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_uavHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(ctx.CbvSrvUavDescriptorSize * m_currViewsCount);

    D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc{};
    viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    viewDesc.Format = resource->Get()->GetDesc().Format;

    for (UINT i = 0; i < mipsCount; ++i)
    {
        viewDesc.Texture2D.MipSlice = i;
        ctx.Device->CreateUnorderedAccessView(resource->Get(), nullptr, &viewDesc, handle);

        handle.Offset(ctx.CbvSrvUavDescriptorSize);
        ++m_currViewsCount;
    }

    return baseOffset;
}

}