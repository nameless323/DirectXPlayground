#pragma once

#include <string>

#include <wrl.h>
#include "External/Dx12Helpers/d3dx12.h"
#include "DXrenderer/RenderContext.h"

namespace DirectxPlayground
{
struct RenderContext;

constexpr UINT InvalidOffset = 0xFFFFFFFF;

struct RtvSrvResourceIdx
{
    UINT RTVOffset = InvalidOffset;
    UINT SRVOffset = InvalidOffset;
    UINT ResourceIdx = InvalidOffset;
};

class TextureManager
{
public:
    TextureManager(RenderContext& ctx);
    RtvSrvResourceIdx CreateTexture(RenderContext& ctx, const std::string& filename);
    RtvSrvResourceIdx CreateRT(RenderContext& ctx, D3D12_RESOURCE_DESC desc, const std::wstring& name, D3D12_CLEAR_VALUE* clearValue = nullptr, bool createSRV = true);

    ID3D12DescriptorHeap* GetDescriptorHeap() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetRtHandle(RenderContext& ctx, UINT index) const;

    ID3D12Resource* GetResource(UINT index) const;

private:
    void CreateSRVHeap(RenderContext& ctx);
    void CreateRTVHeap(RenderContext& ctx);

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_resources;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_uploadResources;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap = nullptr;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap = nullptr;

    UINT m_currentTexCount = 0;
    UINT m_currentRTCount = 0;
};

inline ID3D12DescriptorHeap* TextureManager::GetDescriptorHeap() const
{
    return m_srvHeap.Get();
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

}