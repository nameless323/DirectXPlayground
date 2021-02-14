#pragma once

#include <wrl.h>
#include <dxgi1_6.h>
#include "External/Dx12Helpers/d3dx12.h"

#include "DXrenderer/RenderContext.h"

namespace DirectxPlayground
{
class Swapchain
{
public:
    Swapchain() = default;
    Swapchain(const Swapchain&) = delete;
    ~Swapchain() = default;

    Swapchain& operator= (const Swapchain&) = delete;

    void Init(bool isTearingSupported, HWND hwnd, const RenderContext& state,
        IDXGIFactory7* factory, ID3D12Device* device, ID3D12CommandQueue* commandQueue);
    void Resize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const RenderContext& ctx);

    void Present();
    void ProceedToNextFrame();

    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferCPUhandle(const RenderContext& ctx) const;
    UINT GetCurrentBackBufferIndex() const;
    ID3D12Resource* GetCurrentBackBuffer() const;

private:
    const DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    const DXGI_FORMAT m_depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    UINT m_width = -1;
    UINT m_height = -1;
    UINT m_currentFrameIndex = 0;

    bool m_isSwapChainChainInFullScreen = false;
    bool m_isTearingSupported = false;

    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_dsResource;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_backBufferResources[RenderContext::FramesCount];
};

inline D3D12_CPU_DESCRIPTOR_HANDLE Swapchain::GetCurrentBackBufferCPUhandle(const RenderContext& state) const
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(GetCurrentBackBufferIndex() * state.RtvDescriptorSize);
    return handle;
}

inline UINT Swapchain::GetCurrentBackBufferIndex() const
{
    return m_swapChain->GetCurrentBackBufferIndex();
}

inline ID3D12Resource* Swapchain::GetCurrentBackBuffer() const
{
    return m_backBufferResources[GetCurrentBackBufferIndex()].Get();
}
}