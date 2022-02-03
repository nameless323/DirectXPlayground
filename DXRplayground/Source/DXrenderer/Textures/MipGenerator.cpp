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

    CreateCommonRootSignature(ctx.Device, IID_PPV_ARGS(&mCommonRootSig)); // [a_vorontcov] TODO: Move it goddamn
    CreatePso(ctx);

    mConstantBuffers = new UploadBuffer(*ctx.Device, sizeof(MipGenData), true, m_maxTextureMipsInFrame);
    mQueuedMips.reserve(m_maxTextureMipsInFrame);
    mResourcesPtrs.reserve(m_maxTextureMipsInFrame);
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

        mConstantBuffers->UploadData(mQueuedMipsToGenerateNumber++, mipData);
        assert(mQueuedMipsToGenerateNumber < m_maxTextureMipsInFrame);
        mQueuedMips.push_back(std::move(mipData));
    }
    mResourcesPtrs.push_back(resource);
}

void MipGenerator::Flush(RenderContext& ctx)
{
    if (mQueuedMipsToGenerateNumber == 0)
        return;
    ctx.CommandList->SetComputeRootSignature(mCommonRootSig.Get());
    ctx.CommandList->SetPipelineState(ctx.PsoManager->GetPso(mPsoName));

    ID3D12DescriptorHeap* descHeaps[] = { mUavHeap.Get() };
    ctx.CommandList->SetDescriptorHeaps(1, descHeaps);
    ctx.CommandList->SetComputeRootDescriptorTable(UAVTableIndex, mUavHeap->GetGPUDescriptorHandleForHeapStart());

    std::vector<CD3DX12_RESOURCE_BARRIER> barriers;
    barriers.reserve(mResourcesPtrs.size());
    for (size_t i = 0; i < mResourcesPtrs.size(); ++i)
        barriers.push_back(mResourcesPtrs[i]->GetBarrier(D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
    ctx.CommandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    barriers.clear();

    UINT currTexInd = 0;
    ID3D12Resource* currTex = mResourcesPtrs[currTexInd++]->Get();
    UINT dispatchesLeft = currTex->GetDesc().MipLevels - 1;
    for (UINT i = 0; i < mQueuedMipsToGenerateNumber; ++i)
    {
        ctx.CommandList->SetComputeRootConstantBufferView(GetCBRootParamIndex(0), mConstantBuffers->GetFrameDataGpuAddress(i));
        ctx.CommandList->Dispatch(DISPATCH_TG_COUNT_2D(mQueuedMips[i].Width, 32, mQueuedMips[i].Height, 32));
        auto uav = CD3DX12_RESOURCE_BARRIER::UAV(currTex);
        ctx.CommandList->ResourceBarrier(1, &uav);

        --dispatchesLeft;
        if (dispatchesLeft == 0 && i < mQueuedMipsToGenerateNumber - 1)
        {
            currTex = mResourcesPtrs[currTexInd++]->Get();
            dispatchesLeft = currTex->GetDesc().MipLevels - 1;
        }
    }
    for (size_t i = 0; i < mResourcesPtrs.size(); ++i)
        barriers.push_back(mResourcesPtrs[i]->GetBarrier(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
    ctx.CommandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

    mQueuedMipsToGenerateNumber = 0;
    mCurrViewsCount = 0;
    mQueuedMips.clear();
    mResourcesPtrs.clear();
    ctx.Pipeline->Flush();
}

void MipGenerator::CreateUavHeap(RenderContext& ctx)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = RenderContext::MaxUAVTextures;
    ThrowIfFailed(ctx.Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mUavHeap)));
    NAME_D3D12_OBJECT(mUavHeap, L"Mips heap");
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mUavHeap->GetCPUDescriptorHandleForHeapStart());

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
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = GetDefaultComputePsoDescriptor(mCommonRootSig.Get());

    auto shaderPath = ASSETS_DIR_W + std::wstring(L"Shaders//MipGenerator.hlsl");
    ctx.PsoManager->CreatePso(ctx, mPsoName, shaderPath, desc);
}

UINT MipGenerator::CreateMipViews(RenderContext& ctx, ResourceDX* resource)
{
    UINT baseOffset = mCurrViewsCount;
    UINT mipsCount = resource->Get()->GetDesc().MipLevels;
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mUavHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(ctx.CbvSrvUavDescriptorSize * mCurrViewsCount);

    D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc{};
    viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    viewDesc.Format = resource->Get()->GetDesc().Format;

    for (UINT i = 0; i < mipsCount; ++i)
    {
        viewDesc.Texture2D.MipSlice = i;
        ctx.Device->CreateUnorderedAccessView(resource->Get(), nullptr, &viewDesc, handle);

        handle.Offset(ctx.CbvSrvUavDescriptorSize);
        ++mCurrViewsCount;
    }

    return baseOffset;
}

}