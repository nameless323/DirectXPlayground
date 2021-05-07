#include "TextureManager.h"

#include <filesystem>
#include <vector>
#include <sstream>

#include "External/Dx12Helpers/d3dx12.h"
#include "External/lodepng/lodepng.h"

#define TINYEXR_IMPLEMENTATION
#include "External/TinyEXR/tinyexr.h"

#include "Utils/Logger.h"

#include "DXhelpers.h"

namespace DirectxPlayground
{

TextureManager::TextureManager(RenderContext& ctx)
{
    CreateSRVHeap(ctx);
    CreateRTVHeap(ctx);
}

RtvSrvResourceIdx TextureManager::CreateTexture(RenderContext& ctx, const std::string& filename)
{
    std::vector<unsigned char> bufferInMemory;
    std::vector<unsigned char> buffer;

    UINT w = 0, h = 0;
    lodepng::load_file(bufferInMemory, filename);
    UINT error = lodepng::decode(buffer, w, h, bufferInMemory);
    if (error)
    {
        LOG("PNG decoding error ", error, " : ", lodepng_error_text(error), "\n");
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadResource;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.Width = w;
    texDesc.Height = h;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    texDesc.DepthOrArraySize = 1;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    ThrowIfFailed(ctx.Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&resource)));

#if defined(_DEBUG)
    std::wstring s{ filename.begin(), filename.end() };
    SetDXobjectName(resource.Get(), s.c_str());
#endif

    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(resource.Get(), 0, 1);

    size_t texSize = buffer.size() * sizeof(unsigned char);
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    ThrowIfFailed(ctx.Device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadResource)));

    D3D12_SUBRESOURCE_DATA texData = {};
    texData.pData = buffer.data();
    texData.RowPitch = w * 4;
    texData.SlicePitch = texData.RowPitch * h;

    UpdateSubresources(ctx.CommandList, resource.Get(), uploadResource.Get(), 0, 0, 1, &texData);

    CD3DX12_RESOURCE_BARRIER toDest = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    ctx.CommandList->ResourceBarrier(1, &toDest);

    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
    viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    viewDesc.Format = resource->GetDesc().Format;
    viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    viewDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
    viewDesc.Texture2D.MostDetailedMip = 0;
    viewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_srvHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(m_currentTexCount * ctx.CbvSrvUavDescriptorSize);
    ctx.Device->CreateShaderResourceView(resource.Get(), &viewDesc, handle);

    m_resources.push_back(resource);
    m_uploadResources.push_back(uploadResource);

    RtvSrvResourceIdx res{};
    res.SRVOffset = m_currentTexCount++;
    res.ResourceIdx = static_cast<UINT>(m_resources.size()) - 1;

    return res;
}

RtvSrvResourceIdx TextureManager::CreateRT(RenderContext& ctx, D3D12_RESOURCE_DESC desc, const std::wstring& name, D3D12_CLEAR_VALUE* clearValue /*= nullptr*/, bool createSRV /*= true*/)
{
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;

    ThrowIfFailed(ctx.Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        clearValue,
        IID_PPV_ARGS(&resource)));

#if defined(_DEBUG)
    SetDXobjectName(resource.Get(), name.c_str());
#endif
    D3D12_RENDER_TARGET_VIEW_DESC rtDesc = {};
    rtDesc.Format = desc.Format;
    rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtDesc.Texture2D = { 0, 0 };

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    rtvHandle.Offset(m_currentRTCount, ctx.RtvDescriptorSize);
    ctx.Device->CreateRenderTargetView(resource.Get(), &rtDesc, rtvHandle);

    RtvSrvResourceIdx res;
    res.RTVOffset = m_currentRTCount++;

    if (createSRV)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
        viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        viewDesc.Format = desc.Format;
        viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
        viewDesc.Texture2D.MostDetailedMip = 0;
        viewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_srvHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(m_currentTexCount * ctx.CbvSrvUavDescriptorSize);
        ctx.Device->CreateShaderResourceView(resource.Get(), &viewDesc, handle);

        res.SRVOffset = m_currentTexCount++;
    }
    m_resources.push_back(resource);
    res.ResourceIdx = static_cast<UINT>(m_resources.size()) - 1;

    return res;
}

void TextureManager::CreateSRVHeap(RenderContext& ctx)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = RenderContext::MaxTextures;
    ThrowIfFailed(ctx.Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_srvHeap)));
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_srvHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
    viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    viewDesc.Texture2D.MipLevels = 1;
    viewDesc.Texture2D.MostDetailedMip = 0;
    viewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    for (UINT i = 0; i < RenderContext::MaxTextures; ++i)
    {
        ctx.Device->CreateShaderResourceView(nullptr, &viewDesc, handle);
        handle.Offset(ctx.CbvSrvUavDescriptorSize);
    }
}

void TextureManager::CreateRTVHeap(RenderContext& ctx)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.NumDescriptors = RenderContext::MaxRT;
    ThrowIfFailed(ctx.Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_rtvHeap)));
}

}