#pragma once

#include <array>
#include <exception>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include <wrl.h>

#include "DXrenderer/RenderContext.h"

#include "DXrenderer/Swapchain.h"

namespace DirectxPlayground
{
class Scene;

class RenderPipeline
{
public:
    void Init(HWND hwnd, int width, int height, Scene* scene);
    void Flush();
    void Resize(int width, int height);

    void Render(Scene* scene);

private:
    void GetHardwareAdapter(IDXGIFactory7* factory, IDXGIAdapter1** adapter);
    bool m_isTearingSupported = false;

    RenderContext m_context;

    Swapchain m_swapChain;

    Microsoft::WRL::ComPtr<IDXGIFactory7> m_factory;
    Microsoft::WRL::ComPtr<ID3D12Device5> m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;

    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, RenderContext::FramesCount> m_commandAllocators;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> m_commandList;
    UINT64 m_fenceValues[RenderContext::FramesCount]{};
    UINT64 m_currentFence = 0;
};
}