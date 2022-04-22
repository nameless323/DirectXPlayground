#include "TextureManager.h"

#include <filesystem>
#include <vector>
#include <sstream>

#include "External/Dx12Helpers/d3dx12.h"

#include "DXrenderer/DXhelpers.h"
#include "DXrenderer/Textures/Texture.h"

namespace DirectxPlayground
{
namespace
{
constexpr UINT MaxImguiTexturesCount = 128;
constexpr UINT MaxResources = 32768;
}

TextureManager::TextureManager(RenderContext& ctx)
{
    mMipGenerator = new MipGenerator(ctx);
    mResources.resize(MaxResources);
    CreateSRVHeap(ctx);
    CreateRTVHeap(ctx);
    CreateUAVHeap(ctx);
}

TextureManager::~TextureManager()
{
    SafeDelete(mMipGenerator);
}

TexResourceData TextureManager::CreateTexture(RenderContext& ctx, const std::string& filename, bool generateMips /*= true*/, bool allowUAV /*= false*/)
{
    Texture tex;
    tex.Parse(filename);

    ResourceDX resource{ D3D12_RESOURCE_STATE_COPY_DEST };
    ResourceDX uploadResource{ D3D12_RESOURCE_STATE_GENERIC_READ };

    UINT16 mipLevels = generateMips ? Log2(std::min(tex.GetWidth(), tex.GetHeight())) + 1 : 1;
    D3D12_RESOURCE_FLAGS flags = generateMips ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.MipLevels = mipLevels;
    texDesc.Format = tex.GetFormat();
    texDesc.Width = tex.GetWidth(); // TODO: to the next pot (h too)
    texDesc.Height = tex.GetHeight();
    texDesc.Flags = flags;
    texDesc.DepthOrArraySize = 1;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(ctx.Device->CreateCommittedResource(&heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        resource.GetCurrentState(),
        nullptr,
        IID_PPV_ARGS(resource.GetAddressOf())));

#if defined(_DEBUG)
    std::wstring s{ filename.begin(), filename.end() };
    SetDXobjectName(resource.Get(), s.c_str());
#endif

    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(resource.Get(), 0, 1);

    size_t texSize = tex.GetData().size() * sizeof(byte);
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    ThrowIfFailed(ctx.Device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        uploadResource.GetCurrentState(),
        nullptr,
        IID_PPV_ARGS(uploadResource.GetAddressOf())));

    D3D12_SUBRESOURCE_DATA texData = {};
    texData.pData = tex.GetData().data();
    texData.RowPitch = size_t(tex.GetWidth()) * GetPixelSize(tex.GetFormat());
    texData.SlicePitch = texData.RowPitch * tex.GetHeight();

    UpdateSubresources(ctx.CommandList, resource.Get(), uploadResource.Get(), 0, 0, 1, &texData);
    resource.Transition(ctx.CommandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
    viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    viewDesc.Format = resource.Get()->GetDesc().Format;
    viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    viewDesc.Texture2D.MipLevels = resource.Get()->GetDesc().MipLevels;
    viewDesc.Texture2D.MostDetailedMip = 0;
    viewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mSrvHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(mCurrentTexCount * ctx.CbvSrvUavDescriptorSize);
    ctx.Device->CreateShaderResourceView(resource.Get(), &viewDesc, handle);

    mResources.push_back(resource);
    mUploadResources.push_back(uploadResource);

    TexResourceData res{};
    res.SRVOffset = mCurrentTexCount++;
    res.ResourceIdx = static_cast<UINT>(mResources.size()) - 1;
    res.Resource = &mResources.back();

    if (generateMips)
        mMipGenerator->GenerateMips(ctx, &mResources.back());

    return res;
}

TexResourceData TextureManager::CreateTexture(RenderContext& ctx, D3D12_RESOURCE_DESC desc, const std::wstring& name, D3D12_RESOURCE_STATES initialState)
{
    ResourceDX resource{ initialState };

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(ctx.Device->CreateCommittedResource(&heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        initialState,
        nullptr,
        IID_PPV_ARGS(resource.GetAddressOf())));

#if defined(_DEBUG)
    SetDXobjectName(resource.Get(), name.c_str());
#endif
    TexResourceData res{};
    
    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
    viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    viewDesc.Format = desc.Format;
    viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    viewDesc.Texture2D.MipLevels = resource.Get()->GetDesc().MipLevels;
    viewDesc.Texture2D.MostDetailedMip = 0;
    viewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mSrvHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(mCurrentTexCount * ctx.CbvSrvUavDescriptorSize);
    ctx.Device->CreateShaderResourceView(resource.Get(), &viewDesc, handle);

    res.SRVOffset = mCurrentTexCount++;

    mResources.push_back(resource);
    res.ResourceIdx = static_cast<UINT>(mResources.size()) - 1;
    res.Resource = &mResources.back();

    return res;
}

TexResourceData TextureManager::CreateCubemap(RenderContext& ctx, UINT size, DXGI_FORMAT format, bool allowUAV /*= false*/, const byte* data /*= nullptr*/)
{
    ResourceDX resource{ D3D12_RESOURCE_STATE_COMMON };

    D3D12_RESOURCE_FLAGS resourceFlags = allowUAV ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.MipLevels = 1;
    texDesc.Format = format;
    texDesc.Width = size;
    texDesc.Height = size;
    texDesc.Flags = resourceFlags;
    texDesc.DepthOrArraySize = 6;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(ctx.Device->CreateCommittedResource(&heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(resource.GetAddressOf())));

#if defined(_DEBUG)
    resource.SetName("cubemap");
#endif

    resource.Transition(ctx.CommandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
    viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    viewDesc.Format = resource.Get()->GetDesc().Format;
    viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    viewDesc.Texture2D.MipLevels = resource.Get()->GetDesc().MipLevels;
    viewDesc.Texture2D.MostDetailedMip = 0;
    viewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mSrvHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset((mCurrentCubemapCount + RenderContext::CubemapsRangeStarts) * ctx.CbvSrvUavDescriptorSize);
    ctx.Device->CreateShaderResourceView(resource.Get(), &viewDesc, handle);

    TexResourceData res{};
    res.SRVOffset = mCurrentCubemapCount;
    res.ResourceIdx = static_cast<UINT>(mResources.size()) - 1;
    ++mCurrentCubemapCount;

    if (allowUAV)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = resource.Get()->GetDesc().Format;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        uavDesc.Texture2DArray.ArraySize = 6;
        uavDesc.Texture2DArray.FirstArraySlice = 0;
        uavDesc.Texture2DArray.MipSlice = 0;

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mUavHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(mCurrentUavCount * ctx.CbvSrvUavDescriptorSize);
        ctx.Device->CreateUnorderedAccessView(resource.Get(), nullptr, &uavDesc, handle);
        res.UAVOffset = mCurrentUavCount++;
    }

    mResources.push_back(resource);
    res.Resource = &mResources.back();

    return res;
}


UINT TextureManager::CreateDxrOutput(RenderContext& ctx, D3D12_RESOURCE_DESC desc)
{
    static UINT i = 0;
    assert(i == 0);
    i++;

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(ctx.Device->CreateCommittedResource(&heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        mRtResource.GetCurrentState(),
        nullptr,
        IID_PPV_ARGS(mRtResource.GetAddressOf())));

#if defined(_DEBUG)
    mRtResource.SetName("DXR Resource");
#endif
    D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc = {};
    uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    uavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    uavHeapDesc.NumDescriptors = 1;
    ThrowIfFailed(ctx.Device->CreateDescriptorHeap(&uavHeapDesc, IID_PPV_ARGS(&mRtUavHeap)));
    SetDXobjectName(mRtUavHeap.Get(), L"RT_UAV_HEAP");

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = mRtResource.Get()->GetDesc().Format;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;
    uavDesc.Texture2D.PlaneSlice = 0;
    ctx.Device->CreateUnorderedAccessView(mRtResource.Get(), nullptr, &uavDesc, mRtUavHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = mRtResource.Get()->GetDesc().MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mSrvHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(mCurrentTexCount * ctx.CbvSrvUavDescriptorSize);
    ctx.Device->CreateShaderResourceView(mRtResource.Get(), &srvDesc, handle);
    return mCurrentTexCount++;
}

void TextureManager::FlushMipsQueue(RenderContext& ctx)
{
    mMipGenerator->Flush(ctx);
}

TexResourceData TextureManager::CreateRT(RenderContext& ctx, D3D12_RESOURCE_DESC desc, const std::wstring& name, D3D12_CLEAR_VALUE* clearValue /*= nullptr*/, bool createSRV /*= true*/, bool allowUAV /*= false*/)
{
    ResourceDX resource{ D3D12_RESOURCE_STATE_RENDER_TARGET };

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(ctx.Device->CreateCommittedResource(&heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        resource.GetCurrentState(),
        clearValue,
        IID_PPV_ARGS(resource.GetAddressOf())));

#if defined(_DEBUG)
    resource.SetName(name.c_str());
#endif
    D3D12_RENDER_TARGET_VIEW_DESC rtDesc = {};
    rtDesc.Format = desc.Format;
    rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtDesc.Texture2D = { 0, 0 };

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    rtvHandle.Offset(mCurrentRtCount, ctx.RtvDescriptorSize);
    ctx.Device->CreateRenderTargetView(resource.Get(), &rtDesc, rtvHandle);

    TexResourceData res;
    res.RTVOffset = mCurrentRtCount++;

    if (createSRV)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
        viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        viewDesc.Format = desc.Format;
        viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MipLevels = resource.Get()->GetDesc().MipLevels;
        viewDesc.Texture2D.MostDetailedMip = 0;
        viewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mSrvHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(mCurrentTexCount * ctx.CbvSrvUavDescriptorSize);
        ctx.Device->CreateShaderResourceView(resource.Get(), &viewDesc, handle);

        res.SRVOffset = mCurrentTexCount++;
    }
    mResources.push_back(resource);
    res.ResourceIdx = static_cast<UINT>(mResources.size()) - 1;
    res.Resource = &mResources.back();

    return res;
}

void TextureManager::CreateSRVHeap(RenderContext& ctx)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = RenderContext::MaxTextures + RenderContext::MaxCubemaps;
    ThrowIfFailed(ctx.Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mSrvHeap)));
    AUTO_NAME_D3D12_OBJECT(mSrvHeap);
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mSrvHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
    viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    viewDesc.Texture2D.MipLevels = 1;
    viewDesc.Texture2D.MostDetailedMip = 0;
    viewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    for (UINT i = 0; i < (RenderContext::MaxTextures + RenderContext::MaxCubemaps); ++i)
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
    ThrowIfFailed(ctx.Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mRtvHeap)));
    AUTO_NAME_D3D12_OBJECT(mRtvHeap);
}

void TextureManager::CreateUAVHeap(RenderContext& ctx)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = RenderContext::MaxUAVTextures;
    ThrowIfFailed(ctx.Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mUavHeap)));
    AUTO_NAME_D3D12_OBJECT(mUavHeap);
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mUavHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
    viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
    viewDesc.Texture2DArray.ArraySize = 6;
    viewDesc.Texture2DArray.FirstArraySlice = 0;
    viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    for (UINT i = 0; i < RenderContext::MaxUAVTextures; ++i)
    {
        ctx.Device->CreateUnorderedAccessView(nullptr, nullptr, &viewDesc, handle);
        handle.Offset(ctx.CbvSrvUavDescriptorSize);
    }
}

//////////////////////////////////////////////////////////////////////////
/// Imgui Texture Manager
//////////////////////////////////////////////////////////////////////////

ImguiTextureManager::ImguiTextureManager(RenderContext* context)
    : m_context(context)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = MaxImguiTexturesCount;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    context->Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_imguiDescriptorHeap));
    SetDXobjectName(m_imguiDescriptorHeap, L"Pipeline Imgui Descriptor Heap");
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_imguiDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
    viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    viewDesc.Texture2D.MipLevels = 1;
    viewDesc.Texture2D.MostDetailedMip = 0;
    viewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    for (UINT i = 0; i < desc.NumDescriptors; ++i)
    {
        context->Device->CreateShaderResourceView(nullptr, &viewDesc, handle);
        handle.Offset(context->CbvSrvUavDescriptorSize);
    }
}

void ImguiTextureManager::AddTexture(ID3D12Resource* tex)
{
    assert(m_textures.count(tex) == 0);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = tex->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = tex->GetDesc().MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_imguiDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(m_currentOffset * m_context->CbvSrvUavDescriptorSize);

    m_context->Device->CreateShaderResourceView(tex, &srvDesc, handle);

    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_imguiDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    gpuHandle.Offset(m_currentOffset * m_context->CbvSrvUavDescriptorSize);

    m_textures.insert({ tex, reinterpret_cast<ImTextureID>(gpuHandle.ptr) });
    ++m_currentOffset;
}
}