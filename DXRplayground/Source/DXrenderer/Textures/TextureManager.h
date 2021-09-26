#pragma once

#include <cassert>
#include <string>
#include <map>

#include <wrl.h>
#include "External/Dx12Helpers/d3dx12.h"
#include "DXrenderer/RenderContext.h"
#include "DXrenderer/Textures/MipGenerator.h"

namespace DirectxPlayground
{
struct RenderContext;

constexpr UINT InvalidOffset = 0xFFFFFFFF;

struct RtvSrvUavResourceIdx
{
    UINT RTVOffset = InvalidOffset;
    UINT SRVOffset = InvalidOffset;
    UINT UAVOffset = InvalidOffset;
    UINT ResourceIdx = InvalidOffset;
};

class TextureManager
{
public:
    TextureManager(RenderContext& ctx);
    ~TextureManager();

    RtvSrvUavResourceIdx CreateTexture(RenderContext& ctx, const std::string& filename, bool generateMips = false, bool allowUAV = false);
    RtvSrvUavResourceIdx CreateTexture(RenderContext& ctx, D3D12_RESOURCE_DESC desc, const std::wstring& name, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    RtvSrvUavResourceIdx CreateRT(RenderContext& ctx, D3D12_RESOURCE_DESC desc, const std::wstring& name, D3D12_CLEAR_VALUE* clearValue = nullptr, bool createSRV = true, bool allowUAV = false);

    RtvSrvUavResourceIdx CreateCubemap(RenderContext& ctx, UINT w, UINT h, DXGI_FORMAT format, bool allowUAV = false, const byte* data = nullptr);

    ID3D12DescriptorHeap* GetDescriptorHeap() const;
    ID3D12DescriptorHeap* GetCubemapHeap() const;
    ID3D12DescriptorHeap* GetCubemapUAVHeap() const;

    ID3D12DescriptorHeap* GetDXRUavHeap() const
    {
        return m_rtUavHeap.Get();
    }
    //ID3D12DescriptorHeap* GetDXRSrvHeap() const
    //{
    //    return m_rtSrvHeap.Get();
    //}
    ID3D12Resource* GetDXRResource() const
    {
        return m_rtResource.Get();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetRtHandle(RenderContext& ctx, UINT index) const;

    ID3D12Resource* GetResource(UINT index) const;

    UINT CreateDxrOutput(RenderContext& ctx, D3D12_RESOURCE_DESC desc);

    void FlushMipsQueue(RenderContext& ctx);

    MipGenerator* GetMipGenerator();

private:
    void CreateSRVHeap(RenderContext& ctx);
    void CreateRTVHeap(RenderContext& ctx);
    void CreateUAVHeap(RenderContext& ctx);

    bool ParsePNG(const std::string& filename, std::vector<byte>& buffer, UINT& w, UINT& h, DXGI_FORMAT& textureFormat);
    bool ParseEXR(const std::string& filename, std::vector<byte>& buffer, UINT& w, UINT& h, DXGI_FORMAT& textureFormat);
    bool ParseHDR(const std::string& filename, std::vector<byte>& buffer, UINT& w, UINT& h, DXGI_FORMAT& textureFormat);

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_resources;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_uploadResources;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap = nullptr;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap = nullptr;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvCubeHeap = nullptr;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_uavHeap = nullptr;

    // DXR
    //Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtSrvHeap = nullptr;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtUavHeap = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_rtResource = nullptr;
    //
    MipGenerator* m_mipGenerator = nullptr;

    UINT m_currentTexCount = 0;
    UINT m_currentCubemapsCount = 0;
    UINT m_currentRTCount = 0;
    UINT m_currentUAVCount = 0;
};

inline ID3D12DescriptorHeap* TextureManager::GetDescriptorHeap() const
{
    return m_srvHeap.Get();
}

inline ID3D12DescriptorHeap* TextureManager::GetCubemapHeap() const
{
    return m_srvCubeHeap.Get();
}

inline ID3D12DescriptorHeap* TextureManager::GetCubemapUAVHeap() const
{
    return m_uavHeap.Get();
}

inline D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::GetRtHandle(RenderContext& ctx, UINT index) const
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(ctx.RtvDescriptorSize * index);
    return handle;
}

inline ID3D12Resource* TextureManager::GetResource(UINT index) const
{
    return m_resources[index].Get();
}

inline MipGenerator* TextureManager::GetMipGenerator()
{
    return m_mipGenerator;
}

//////////////////////////////////////////////////////////////////////////
/// Imgui Texture Manager
//////////////////////////////////////////////////////////////////////////

typedef void* ImTextureID;

class ImguiTextureManager
{
public:
    void AddTexture(ID3D12Resource* tex);
    ImTextureID GetTextureId(ID3D12Resource* tex) const;

private:
    friend class RenderPipeline;

    ImguiTextureManager(RenderContext* context);
    ID3D12DescriptorHeap* GetHeap() const;

    ID3D12DescriptorHeap* m_imguiDescriptorHeap = nullptr;

    UINT m_currentOffset = 1; // IMGUI font texture always goes at the beginning of the heap.
    RenderContext* m_context = nullptr;
    std::map<ID3D12Resource*, ImTextureID> m_textures;
};

inline ID3D12DescriptorHeap* ImguiTextureManager::GetHeap() const
{
    return m_imguiDescriptorHeap;
}

inline ImTextureID ImguiTextureManager::GetTextureId(ID3D12Resource* tex) const
{
    assert(m_textures.count(tex) == 1);
    return m_textures.at(tex);
}

}