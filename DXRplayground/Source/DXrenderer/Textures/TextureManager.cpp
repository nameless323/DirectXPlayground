#include "TextureManager.h"

#include <filesystem>
#include <vector>
#include <sstream>

#include "External/Dx12Helpers/d3dx12.h"
#include "External/lodepng/lodepng.h"

#define TINYEXR_IMPLEMENTATION
#include "External/TinyEXR/tinyexr.h"

#define STB_IMAGE_IMPLEMENTATION
#include "External/stb/stb_image.h"

#include "Utils/Logger.h"

#include "DXrenderer/DXhelpers.h"

namespace DirectxPlayground
{
namespace
{
static constexpr UINT MaxImguiTexturesCount = 128;
static constexpr UINT MaxResources = 32768;
}

TextureManager::TextureManager(RenderContext& ctx)
{
    m_mipGenerator = new MipGenerator(ctx);
    m_resources.resize(MaxResources);
    CreateSRVHeap(ctx);
    CreateRTVHeap(ctx);
    CreateUAVHeap(ctx);
}

TextureManager::~TextureManager()
{
    SafeDelete(m_mipGenerator);
}

TexResourceData TextureManager::CreateTexture(RenderContext& ctx, const std::string& filename, bool generateMips /*= true*/, bool allowUAV /*= false*/)
{
    std::vector<byte> buffer;

    std::wstring extension{ std::filesystem::path(filename.c_str()).extension().c_str() };

    UINT w = 0, h = 0;
    DXGI_FORMAT textureFormat{};

    if (extension == L".png" || extension == L".PNG") // let's hope there won't be "pNg" or "PnG" etc
    {
        bool success = ParsePNG(filename, buffer, w, h, textureFormat);
    }
    else if (extension == L".exr" || extension == L".EXR")
    {
        bool success = ParseEXR(filename, buffer, w, h, textureFormat);
    }
    else if (extension == L".hdr" || extension == L".HDR")
    {
        bool success = ParseHDR(filename, buffer, w, h, textureFormat);
    }
    else
    {
        assert("Unknown image format for parsing" && false);
    }

    ResourceDX resource{ D3D12_RESOURCE_STATE_COPY_DEST };
    ResourceDX uploadResource{ D3D12_RESOURCE_STATE_GENERIC_READ };

    UINT16 mipLevels = generateMips ? Log2(std::min(w, h)) + 1 : 1;
    D3D12_RESOURCE_FLAGS flags = generateMips ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.MipLevels = mipLevels;
    texDesc.Format = textureFormat;
    texDesc.Width = w; // TODO: to the next pot (h too)
    texDesc.Height = h;
    texDesc.Flags = flags;
    texDesc.DepthOrArraySize = 1;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    ThrowIfFailed(ctx.Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
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

    size_t texSize = buffer.size() * sizeof(byte);
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
    texData.pData = buffer.data();
    texData.RowPitch = size_t(w) * GetPixelSize(textureFormat);
    texData.SlicePitch = texData.RowPitch * h;

    UpdateSubresources(ctx.CommandList, resource.Get(), uploadResource.Get(), 0, 0, 1, &texData);
    resource.Transition(ctx.CommandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
    viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    viewDesc.Format = resource.Get()->GetDesc().Format;
    viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    viewDesc.Texture2D.MipLevels = resource.Get()->GetDesc().MipLevels;
    viewDesc.Texture2D.MostDetailedMip = 0;
    viewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_srvHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(m_currentTexCount * ctx.CbvSrvUavDescriptorSize);
    ctx.Device->CreateShaderResourceView(resource.Get(), &viewDesc, handle);

    m_resources.push_back(resource);
    m_uploadResources.push_back(uploadResource);

    TexResourceData res{};
    res.SRVOffset = m_currentTexCount++;
    res.ResourceIdx = static_cast<UINT>(m_resources.size()) - 1;
    res.Resource = &m_resources.back();

    if (generateMips)
        m_mipGenerator->GenerateMips(ctx, resource.Get());

    return res;
}

DirectxPlayground::TexResourceData TextureManager::CreateTexture(RenderContext& ctx, D3D12_RESOURCE_DESC desc, const std::wstring& name, D3D12_RESOURCE_STATES initialState)
{
    ResourceDX resource{ initialState };

    ThrowIfFailed(ctx.Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
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

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_srvHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(m_currentTexCount * ctx.CbvSrvUavDescriptorSize);
    ctx.Device->CreateShaderResourceView(resource.Get(), &viewDesc, handle);

    res.SRVOffset = m_currentTexCount++;

    m_resources.push_back(resource);
    res.ResourceIdx = static_cast<UINT>(m_resources.size()) - 1;
    res.Resource = &m_resources.back();

    return res;
}

DirectxPlayground::TexResourceData TextureManager::CreateCubemap(RenderContext& ctx, UINT w, UINT h, DXGI_FORMAT format, bool allowUAV /*= false*/, const byte* data /*= nullptr*/)
{
    ResourceDX resource{ D3D12_RESOURCE_STATE_COMMON };

    D3D12_RESOURCE_FLAGS resourceFlags = allowUAV ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.MipLevels = 1;
    texDesc.Format = format;
    texDesc.Width = w;
    texDesc.Height = h;
    texDesc.Flags = resourceFlags;
    texDesc.DepthOrArraySize = 6;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    ThrowIfFailed(ctx.Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
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

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_srvCubeHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(m_currentCubemapsCount * ctx.CbvSrvUavDescriptorSize);
    ctx.Device->CreateShaderResourceView(resource.Get(), &viewDesc, handle);


    TexResourceData res{};
    res.SRVOffset = m_currentCubemapsCount++;
    res.ResourceIdx = static_cast<UINT>(m_resources.size()) - 1;

    if (allowUAV)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = resource.Get()->GetDesc().Format;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        uavDesc.Texture2DArray.ArraySize = 6;
        uavDesc.Texture2DArray.FirstArraySlice = 0;
        uavDesc.Texture2DArray.MipSlice = 0;

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_uavHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(m_currentUAVCount * ctx.CbvSrvUavDescriptorSize);
        ctx.Device->CreateUnorderedAccessView(resource.Get(), nullptr, &uavDesc, handle);
        res.UAVOffset = m_currentUAVCount++;
    }

    m_resources.push_back(resource);
    res.Resource = &m_resources.back();

    return res;
}


UINT TextureManager::CreateDxrOutput(RenderContext& ctx, D3D12_RESOURCE_DESC desc)
{
    static UINT i = 0;
    assert(i == 0);
    i++;

    ThrowIfFailed(ctx.Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &desc,
        m_rtResource.GetCurrentState(),
        nullptr,
        IID_PPV_ARGS(m_rtResource.GetAddressOf())));

#if defined(_DEBUG)
    m_rtResource.SetName("DXR Resource");
#endif
    D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc = {};
    uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    uavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    uavHeapDesc.NumDescriptors = 1;
    ThrowIfFailed(ctx.Device->CreateDescriptorHeap(&uavHeapDesc, IID_PPV_ARGS(&m_rtUavHeap)));
    SetDXobjectName(m_rtUavHeap.Get(), L"RT_UAV_HEAP");

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = m_rtResource.Get()->GetDesc().Format;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;
    uavDesc.Texture2D.PlaneSlice = 0;
    ctx.Device->CreateUnorderedAccessView(m_rtResource.Get(), nullptr, &uavDesc, m_rtUavHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = m_rtResource.Get()->GetDesc().MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_srvHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(m_currentTexCount * ctx.CbvSrvUavDescriptorSize);
    ctx.Device->CreateShaderResourceView(m_rtResource.Get(), &srvDesc, handle);
    return m_currentTexCount++;
}

void TextureManager::FlushMipsQueue(RenderContext& ctx)
{
    m_mipGenerator->Flush(ctx);
}

bool TextureManager::ParsePNG(const std::string& filename, std::vector<byte>& buffer, UINT& w, UINT& h, DXGI_FORMAT& textureFormat)
{
    std::vector<byte> bufferInMemory;
    lodepng::load_file(bufferInMemory, filename);
    UINT error = lodepng::decode(buffer, w, h, bufferInMemory);
    if (error)
    {
        LOG("PNG decoding error ", error, " : ", lodepng_error_text(error), "\n");
        return false;
    }
    textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    return true;
}

bool TextureManager::ParseEXR(const std::string& filename, std::vector<byte>& buffer, UINT& w, UINT& h, DXGI_FORMAT& textureFormat)
{
    int width = 0, height = 0;
    float* out = nullptr;
    const char* err = nullptr;
    int ret = LoadEXR(&out, &width, &height, filename.c_str(), &err);
    if (ret != TINYEXR_SUCCESS)
    {
        if (err != nullptr)
        {
            LOG("EXR decoding error ", err);
            FreeEXRErrorMessage(err);
        }
        return false;
    }
    else
    {
        w = static_cast<UINT>(width);
        h = static_cast<UINT>(height);

        size_t imageSize = size_t(w) * size_t(h) * sizeof(float) * 4U;
        buffer.resize(imageSize);
        memcpy(buffer.data(), out, imageSize);

        textureFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;

        free(out);
    }
    return true;
}

bool TextureManager::ParseHDR(const std::string& filename, std::vector<byte>& buffer, UINT& w, UINT& h, DXGI_FORMAT& textureFormat)
{
    int width = 0;
    int height = 0;
    int nComponents = 0;
    float* data = stbi_loadf(filename.c_str(), &width, &height, &nComponents, 0);
    if (data == nullptr)
    {
        LOG("HDR decoding error");
        return false;
    }
    if (nComponents != 3 && nComponents != 4)
    {
        LOG("HDR decoding error. Invalid number of color channels in the file");
        return false;
    }
    w = static_cast<UINT>(width);
    h = static_cast<UINT>(height);

    size_t imageSize = size_t(w) * size_t(h) * sizeof(float) * 4U;
    buffer.resize(imageSize);
    // [a_vorontcov] Well extension to 4 components because we want to use the texture as uav, and currently R32G32B32 format isn't supported.
    if (nComponents == 4)
    {
        memcpy(buffer.data(), data, imageSize);
    }
    else
    {
        float* bufferAsFloat = reinterpret_cast<float*>(buffer.data());
        for (size_t i = 0; i < size_t(w) * size_t(h); ++i)
        {
            bufferAsFloat[i * 4 + 0] = data[i * 3 + 0];
            bufferAsFloat[i * 4 + 1] = data[i * 3 + 1];
            bufferAsFloat[i * 4 + 2] = data[i * 3 + 2];
            bufferAsFloat[i * 4 + 3] = 1.0f;
        }
    }

    textureFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;

    stbi_image_free(data);

    return true;
}

TexResourceData TextureManager::CreateRT(RenderContext& ctx, D3D12_RESOURCE_DESC desc, const std::wstring& name, D3D12_CLEAR_VALUE* clearValue /*= nullptr*/, bool createSRV /*= true*/, bool allowUAV /*= false*/)
{
    ResourceDX resource{ D3D12_RESOURCE_STATE_RENDER_TARGET };

    ThrowIfFailed(ctx.Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
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

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    rtvHandle.Offset(m_currentRTCount, ctx.RtvDescriptorSize);
    ctx.Device->CreateRenderTargetView(resource.Get(), &rtDesc, rtvHandle);

    TexResourceData res;
    res.RTVOffset = m_currentRTCount++;

    if (createSRV)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
        viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        viewDesc.Format = desc.Format;
        viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MipLevels = resource.Get()->GetDesc().MipLevels;
        viewDesc.Texture2D.MostDetailedMip = 0;
        viewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_srvHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(m_currentTexCount * ctx.CbvSrvUavDescriptorSize);
        ctx.Device->CreateShaderResourceView(resource.Get(), &viewDesc, handle);

        res.SRVOffset = m_currentTexCount++;
    }
    m_resources.push_back(resource);
    res.ResourceIdx = static_cast<UINT>(m_resources.size()) - 1;
    res.Resource = &m_resources.back();

    return res;
}

void TextureManager::CreateSRVHeap(RenderContext& ctx)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = RenderContext::MaxTextures;
    ThrowIfFailed(ctx.Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_srvHeap)));
    AUTO_NAME_D3D12_OBJECT(m_srvHeap);
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

    heapDesc.NumDescriptors = RenderContext::MaxCubemaps;
    ThrowIfFailed(ctx.Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_srvCubeHeap)));
    handle = m_srvCubeHeap->GetCPUDescriptorHandleForHeapStart();
    viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;

    for (UINT i = 0; i < RenderContext::MaxCubemaps; ++i)
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
    AUTO_NAME_D3D12_OBJECT(m_rtvHeap);
}

void TextureManager::CreateUAVHeap(RenderContext& ctx)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = RenderContext::MaxUAVTextures;
    ThrowIfFailed(ctx.Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_uavHeap)));
    AUTO_NAME_D3D12_OBJECT(m_uavHeap);
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_uavHeap->GetCPUDescriptorHandleForHeapStart());

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