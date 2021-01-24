#pragma once

#include <array>
#include <exception>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include <wrl.h>

#include "DXrenderer/DXsharedState.h"

#include "DXrenderer/DXswapchain.h"

namespace DXRplayground
{
class DXpipeline
{
public:
    void Init(HWND hwnd, int width, int height);
    void Flush();
    void Resize(int width, int height);

    void Draw();

private:
    void GetHardwareAdapter(IDXGIFactory7* factory, IDXGIAdapter1** adapter);
    bool m_isTearingSupported = false;

    DXsharedState m_sharedState;

    DXswapchain m_swapChain;

    Microsoft::WRL::ComPtr<IDXGIFactory7> m_factory;
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;

    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, DXsharedState::FramesCount> m_commandAllocators; // [a_vorontcov] For each render thread?
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocators[DXsharedState::FramesCount];
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValues[DXsharedState::FramesCount]{};
    UINT64 m_currentFence = 0;
};
}