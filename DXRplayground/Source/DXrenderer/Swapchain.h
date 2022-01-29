#pragma once

#include <wrl.h>
#include <dxgi1_6.h>
#include "External/Dx12Helpers/d3dx12.h"

#include "DXrenderer/RenderContext.h"
#include "DXrenderer/ResourceDX.h"

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
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSCPUhandle() const;
    UINT GetCurrentBackBufferIndex() const;
    ID3D12Resource* GetCurrentBackBuffer() const;

    DXGI_FORMAT GetBackBufferFormat() const;
    DXGI_FORMAT GetDepthStencilFormat() const;

private:
    const DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    const DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    UINT mWidth = -1;
    UINT mHeight = -1;
    UINT mCurrentFrameIndex = 0;

    bool mIsSwapChainChainInFullScreen = false;
    bool mIsTearingSupported = false;

    Microsoft::WRL::ComPtr<IDXGISwapChain4> mSwapChain;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

    ResourceDX mDsResource;
    ResourceDX mBackBufferResources[RenderContext::FramesCount];
};

inline D3D12_CPU_DESCRIPTOR_HANDLE Swapchain::GetCurrentBackBufferCPUhandle(const RenderContext& state) const
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(GetCurrentBackBufferIndex() * state.RtvDescriptorSize);
    return handle;
}

inline D3D12_CPU_DESCRIPTOR_HANDLE Swapchain::GetDSCPUhandle() const
{
    return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

inline UINT Swapchain::GetCurrentBackBufferIndex() const
{
    return mSwapChain->GetCurrentBackBufferIndex();
}

inline ID3D12Resource* Swapchain::GetCurrentBackBuffer() const
{
    return mBackBufferResources[GetCurrentBackBufferIndex()].Get();
}

inline DXGI_FORMAT Swapchain::GetBackBufferFormat() const
{
    return mBackBufferFormat;
}

inline DXGI_FORMAT Swapchain::GetDepthStencilFormat() const
{
    return mDepthStencilFormat;
}

}